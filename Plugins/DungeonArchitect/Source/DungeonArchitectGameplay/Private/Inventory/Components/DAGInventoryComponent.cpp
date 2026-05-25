//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/Components/DAGInventoryComponent.h"

#include "Item/DAGItemDefinition.h"

#include "Net/UnrealNetwork.h"

UDAG_InventoryComponent::UDAG_InventoryComponent() {
	// Enable replication for this component
	SetIsReplicatedByDefault(true);
}

void UDAG_InventoryComponent::BeginPlay() {
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority) {
		ServerInitialize(InitialNumSlots);
	}
}

void UDAG_InventoryComponent::Initialize_Implementation(int32 InNumSlots) {
	ServerInitialize(InNumSlots);
}

bool UDAG_InventoryComponent::Initialize_Validate(int32 InNumSlots) {
	// TODO: Validate me
	return true;
}

void UDAG_InventoryComponent::AddItems_Implementation(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	ServerAddItems(InItemDefinition, InCount);
}

bool UDAG_InventoryComponent::AddItems_Validate(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	// TODO: Validate me
	return true;
}

void UDAG_InventoryComponent::AddItemsToSlot_Implementation(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	ServerAddItemsToSlot(SlotIndex, InItemDefinition, InCount);
}

bool UDAG_InventoryComponent::AddItemsToSlot_Validate(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	// TODO: Validate me
	return true;
}


void UDAG_InventoryComponent::RemoveItems_Implementation(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	ServerRemoveItems(InItemDefinition, InCount);
}

bool UDAG_InventoryComponent::RemoveItems_Validate(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	// TODO: Validate me
    return true;
}

void UDAG_InventoryComponent::RemoveItemsFromSlot_Implementation(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	ServerRemoveItemsFromSlot(SlotIndex, InItemDefinition, InCount);
}

bool UDAG_InventoryComponent::RemoveItemsFromSlot_Validate(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	// TODO: Validate me
	return true;
}

void UDAG_InventoryComponent::SwapItems_Implementation(int32 SlotIndexA, int32 SlotIndexB) {
	ServerSwapItems(SlotIndexA, SlotIndexB);
}

bool UDAG_InventoryComponent::SwapItems_Validate(int32 SlotIndexA, int32 SlotIndexB) {
	// TODO: Validate me
	return true;
}

bool UDAG_InventoryComponent::CanAddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	return Inventory.CanAddItems(InItemDefinition, InCount);
}

bool UDAG_InventoryComponent::CanAddInventory(const FDAG_Inventory& InventoryToAdd) {
	FDAG_Inventory LocalCopy = Inventory;
	for (FDAG_InventorySlot SlotToAdd : InventoryToAdd.GetSlots()) {
		if (!SlotToAdd.IsEmpty()) {
			const int32 NumAdded = LocalCopy.AddItems(SlotToAdd.ItemDefinition, SlotToAdd.StackCount);
			if (NumAdded < SlotToAdd.StackCount) {
				// We were not able to add this item
				return false;
			}
		}
	}
	return true;
}

bool UDAG_InventoryComponent::HasItems(const UDAG_ItemDefinition* InItemDefinition, int32 InCount) const {
	if (!InItemDefinition) {
		return false;
	}
	
	int RequiredAmount = InCount;
	for (int32 SlotIdx = 0; SlotIdx <  Inventory.GetNumSlots(); SlotIdx++) {
		FDAG_InventorySlot Slot;
		if (Inventory.GetItemAt(SlotIdx, Slot)) {
			if (Slot.IsEmpty()) {
				RequiredAmount -= InItemDefinition->MaxStackCount;
			}
			else if (Slot.ItemDefinition == InItemDefinition) {
				RequiredAmount = FMath::Max(0, Slot.StackCount);
			}

			if (RequiredAmount <= 0) {
				break;
			}
		}
	}

	return RequiredAmount <= 0;
}

void UDAG_InventoryComponent::ServerInitialize(int32 InNumSlots) {
	check(GetOwnerRole() == ROLE_Authority);
	Inventory.Initialize(InNumSlots);
}

void UDAG_InventoryComponent::ServerSwapItems(int32 SlotIndexA, int32 SlotIndexB) {
	check(GetOwnerRole() == ROLE_Authority);
	Inventory.SwapItems(SlotIndexA, SlotIndexB);
}

int32 UDAG_InventoryComponent::ServerRemoveItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	check(GetOwnerRole() == ROLE_Authority);
	return Inventory.RemoveItems(InItemDefinition, InCount);
}

int32 UDAG_InventoryComponent::ServerRemoveItemsFromSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	check(GetOwnerRole() == ROLE_Authority);
	return Inventory.RemoveItemsFromSlot(SlotIndex, InItemDefinition, InCount); 
}

int32 UDAG_InventoryComponent::ServerAddItemsToSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	check(GetOwnerRole() == ROLE_Authority);
	return Inventory.AddItemsToSlot(SlotIndex, InItemDefinition, InCount);
}

int32 UDAG_InventoryComponent::ServerAddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	check(GetOwnerRole() == ROLE_Authority);
	return Inventory.AddItems(InItemDefinition, InCount);
}

bool UDAG_InventoryComponent::GetItemAt(int32 Index, FDAG_InventorySlot& OutItem) {
	return Inventory.GetItemAt(Index, OutItem);
}

int32 UDAG_InventoryComponent::GetNumSlots() const {
	return Inventory.GetNumSlots();
}

void UDAG_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDAG_InventoryComponent, Inventory);
}

