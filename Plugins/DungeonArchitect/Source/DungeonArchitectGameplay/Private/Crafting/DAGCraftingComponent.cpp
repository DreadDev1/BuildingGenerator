//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Crafting/DAGCraftingComponent.h"

#include "Crafting/DAGCraftingRecipe.h"
#include "Inventory/Components/DAGInventoryComponent.h"

#include "Net/UnrealNetwork.h"

UDAG_CraftingSystemComponent::UDAG_CraftingSystemComponent() {
    PrimaryComponentTick.bCanEverTick = true;
    SetCraftingState(EDAG_CraftingSystemState::NotCrafting);
}

void UDAG_CraftingSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (HasAuthority()) {
        if (CraftingState == EDAG_CraftingSystemState::NotCrafting && bAutoCraftWhenInputReady) {
            // We we want to autocraft, pause this and wait for input.   It wil automatically craft when the intpu is availble
            SetCraftingState(EDAG_CraftingSystemState::Paused);
            PauseReason = EDAG_CraftingSystemPauseReason::WaitTillInputAvailable;
        }
    
        if (CraftingState == EDAG_CraftingSystemState::Paused) {
            // Check if we can unpause
            if (PauseReason == EDAG_CraftingSystemPauseReason::WaitTillOutputFree) {
                // Check if the output inventory can hold our items now
                if (OutputInventory.IsValid() && CurrentRecipe) {
                    bool bCanHoldAllOutputItems = true;
                    for (const FDAG_CraftingRecipeItem& ResultItem : CurrentRecipe->ResultItems) {
                        if (!OutputInventory->CanAddItems(ResultItem.ItemDefinition, ResultItem.Amount)) {
                            bCanHoldAllOutputItems = false;
                            break;
                        }
                    }
                    if (bCanHoldAllOutputItems) {
                        // Continue crafting
                        SetCraftingState(EDAG_CraftingSystemState::InProgress);
                    }
                }
            }
            else if (PauseReason == EDAG_CraftingSystemPauseReason::WaitTillInputAvailable) {
                // Check if the input inventory has enough items to craft the recipe
                if (CanCraft(CurrentRecipe, InputInventory.Get())) {
                    // We have enough resources available
                    SetCraftingState(EDAG_CraftingSystemState::InProgress);
                }
            }
        }
    
        if (CraftingState == EDAG_CraftingSystemState::InProgress && CurrentRecipe) {
            CraftingProgress = FMath::Min(TotalCraftingTime, CraftingProgress + DeltaTime);
            NotifyCraftingProgress();
            if (CraftingProgress >= TotalCraftingTime) {
                // we've completed the crafting time.
                // Check if the output inventory is free to hold the result item
                if (CurrentRecipe && OutputInventory.IsValid()) {
                    if (CanAddItemsToInventory(CurrentRecipe->ResultItems, OutputInventory.Get())) {
                        // Complete the crafting
                        ServerCompleteCrafting();
                    }
                    else {
                        // Pause the crafting, till we have space in the output inventory
                        SetCraftingState(EDAG_CraftingSystemState::Paused);
                        PauseReason = EDAG_CraftingSystemPauseReason::WaitTillOutputFree;
                    }
                }
            }
        }
    }
}

bool UDAG_CraftingSystemComponent::CanCraft(UDAG_CraftingRecipe* InRecipe, UDAG_InventoryComponent* InInputInventory) const {
    if (!InRecipe || !InInputInventory) {
        return false;
    }
    // Check if inventory has required items
    {
        // Find the total required count per item type
        TMap<UDAG_ItemDefinition*, int32> TotalRequiredItemCount;
        for (const FDAG_CraftingRecipeItem& RequiredItem : InRecipe->RequiredItems) {
            int32& CountRef = TotalRequiredItemCount.FindOrAdd(RequiredItem.ItemDefinition);
            CountRef += RequiredItem.Amount;
        }
        // Check if we have this amount in the inventory
        for (auto& Entry : TotalRequiredItemCount) {
            const UDAG_ItemDefinition* ItemDef = Entry.Key;
            const int32 ItemCount = Entry.Value;
            if (!InInputInventory->HasItems(ItemDef, ItemCount)) {
                return false;
            }
        }
    }

    return true;
}

void UDAG_CraftingSystemComponent::StartCrafting_Implementation(UDAG_CraftingRecipe* InRecipe, UDAG_InventoryComponent* InInputInventory) {
    if (!HasAuthority() || !InRecipe || !InInputInventory) {
        return;
    }

    if (CraftingState == EDAG_CraftingSystemState::InProgress) {
        return;
    }

    if (!CanCraft(InRecipe, InInputInventory)) {
        if (bAutoCraftWhenInputReady) {
            SetCraftingState(EDAG_CraftingSystemState::Paused);
            PauseReason = EDAG_CraftingSystemPauseReason::WaitTillInputAvailable;
        }
        return;
    }

    InputInventory = InInputInventory;
    
    // Remove required items from inventory
    for (const FDAG_CraftingRecipeItem& RequiredItem : InRecipe->RequiredItems) {
        InInputInventory->RemoveItems(RequiredItem.ItemDefinition, RequiredItem.Amount);
    }

    // Initialize crafting process
    CurrentRecipe = InRecipe;
    CraftingProgress = 0.0f;
    
    TotalCraftingTime = InRecipe->CraftingTime;
    SetCraftingState(EDAG_CraftingSystemState::InProgress);

    // Notify clients that the crafting has started
    if (GetOwnerRole() == ROLE_Authority) {
        NotifyCraftingProgress();
    }
}

bool UDAG_CraftingSystemComponent::StartCrafting_Validate(UDAG_CraftingRecipe* InRecipe, UDAG_InventoryComponent* InInputInventory) {
    // TODO: Validate me
    return true;
}

void UDAG_CraftingSystemComponent::ServerCompleteCrafting() {
    check(HasAuthority());
    if (!CurrentRecipe || !OutputInventory.IsValid()) {
        return;
    }
    
    if (CanAddItemsToInventory(CurrentRecipe->ResultItems, OutputInventory.Get())) {
        for (const FDAG_CraftingRecipeItem& ResultItem : CurrentRecipe->ResultItems) {
            int32 NumAdded = OutputInventory->ServerAddItems(ResultItem.ItemDefinition, ResultItem.Amount);
            check(NumAdded == ResultItem.Amount);
            
        }

        if (bAutoCraftWhenInputReady) {
            SetCraftingState(EDAG_CraftingSystemState::Paused);
            PauseReason = EDAG_CraftingSystemPauseReason::WaitTillInputAvailable;
        }
        else {
            SetCraftingState(EDAG_CraftingSystemState::NotCrafting);
        }
    }

}

void UDAG_CraftingSystemComponent::NotifyCraftingProgress() const {
    OnCraftingProgress.Broadcast(CraftingProgress, TotalCraftingTime);
}

bool UDAG_CraftingSystemComponent::CanAddItemsToInventory(const TArray<FDAG_CraftingRecipeItem>& InItems, UDAG_InventoryComponent* InInventory) {
    
    TMap<UDAG_ItemDefinition*, int32> TotalItemCount;
    for (const FDAG_CraftingRecipeItem& Item : InItems) {
        int32& CountRef = TotalItemCount.FindOrAdd(Item.ItemDefinition);
        CountRef += Item.Amount;
    }
    
    for (const auto& Entry : TotalItemCount) {
        UDAG_ItemDefinition* ItemDef = Entry.Key;
        int32 Count = Entry.Value;
        if (!InInventory->CanAddItems(ItemDef, Count)) {
            return false;
        }
    }

    return true;
}

void UDAG_CraftingSystemComponent::CancelCrafting() {
    SetCraftingState(EDAG_CraftingSystemState::NotCrafting);
}

EDAG_CraftingSystemPauseReason UDAG_CraftingSystemComponent::GetCraftingPauseReason() const {
    return CraftingState == EDAG_CraftingSystemState::Paused
        ? PauseReason
        : EDAG_CraftingSystemPauseReason::None;
}

bool UDAG_CraftingSystemComponent::HasAuthority() const {
    return GetOwnerRole() == ROLE_Authority;
}

void UDAG_CraftingSystemComponent::OnRep_CraftingProgress() {
    NotifyCraftingProgress();
}

void UDAG_CraftingSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UDAG_CraftingSystemComponent, CraftingProgress);
    DOREPLIFETIME(UDAG_CraftingSystemComponent, CraftingState);
    DOREPLIFETIME(UDAG_CraftingSystemComponent, PauseReason);
    DOREPLIFETIME(UDAG_CraftingSystemComponent, TotalCraftingTime);
}

void UDAG_CraftingSystemComponent::SetCraftingState(EDAG_CraftingSystemState InState) {
    CraftingState = InState;

    if (CraftingState == EDAG_CraftingSystemState::NotCrafting) {
        CraftingProgress = 0.0f;
        CurrentRecipe = nullptr;
        OutputInventory = nullptr;
    }
}

