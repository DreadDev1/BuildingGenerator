//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/Components/DAGPlayerInventoryControllerComponent.h"

#include "Inventory/Components/DAGInventoryComponent.h"
#include "Item/DAGItemDefinition.h"

#include "Net/UnrealNetwork.h"

UDAG_PlayerInventoryControllerComponent::UDAG_PlayerInventoryControllerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}


void UDAG_PlayerInventoryControllerComponent::MoveCursorToInventorySlot_Implementation(UDAG_InventoryComponent* Inventory, int32 SlotIndex, int32 StackCount, bool bAllowSwapping) {
	FDAG_InventorySlot ExistingSlot;
	StackCount = FMath::Min(StackCount, CursorSlot.StackCount);
	if (Inventory->GetItemAt(SlotIndex, ExistingSlot)) {
		if (ExistingSlot.ItemDefinition == nullptr || ExistingSlot.ItemDefinition == CursorSlot.ItemDefinition) {
			// Target slot contains the same item, or is empty. It can be added here
			int32 NumItemsAdded = Inventory->ServerAddItemsToSlot(SlotIndex, CursorSlot.ItemDefinition, StackCount);
			check(NumItemsAdded <= StackCount);
			CursorSlot.StackCount -= NumItemsAdded;
		}
		else if (bAllowSwapping) {
			// Contains a different item.  Swap it with the cursor items
			FDAG_InventorySlot CursorItemsToReplace = CursorSlot;
			CursorSlot.Clear();
			MoveInventorySlotToCursor(Inventory, SlotIndex);	// Pick up everything from the inventory slot on to the cursor

			Inventory->ServerAddItemsToSlot(SlotIndex, CursorItemsToReplace.ItemDefinition, CursorItemsToReplace.StackCount);
		}
	}
}

bool UDAG_PlayerInventoryControllerComponent::MoveCursorToInventorySlot_Validate(UDAG_InventoryComponent* Inventory, int32 SlotIndex, int32 StackCount, bool bAllowSwapping) {
	// TODO: Implement me
	return true;
}

void UDAG_PlayerInventoryControllerComponent::MoveInventorySlotToCursorImpl(UDAG_InventoryComponent* Inventory, int32 SlotIndex, const TFunction<int(int)>& FnCountToRemove) {
	check(GetOwnerRole() == ROLE_Authority);
	check(CursorSlot.IsEmpty());
	
	// Transfer the inventory items from the slot to the cursor's slot
	// TODO: Handle multiplayer
	if (Inventory) {
		FDAG_InventorySlot ExistingSlot;
		if (Inventory->GetItemAt(SlotIndex, ExistingSlot)) {
			CursorSlot.ItemDefinition = ExistingSlot.ItemDefinition;
			CursorSlot.StackCount = Inventory->ServerRemoveItemsFromSlot(SlotIndex, ExistingSlot.ItemDefinition, FnCountToRemove(ExistingSlot.StackCount));
			if (CursorSlot.StackCount == 0) {
				CursorSlot.Clear();
			}
			
			LastPickupSlotIndex = SlotIndex;
			LastPickupInventory = Inventory;
		}
	}
}

void UDAG_PlayerInventoryControllerComponent::MoveInventorySlotToCursor_Implementation(UDAG_InventoryComponent* Inventory, int32 SlotIndex) {
	MoveInventorySlotToCursorImpl(Inventory, SlotIndex, [](int Count){  return Count; });
}

bool UDAG_PlayerInventoryControllerComponent::MoveInventorySlotToCursor_Validate(UDAG_InventoryComponent* Inventory, int32 SlotIndex) {
	// TODO: Implement me
	return true;
}

void UDAG_PlayerInventoryControllerComponent::MoveInventoryHalfSlotToCursor_Implementation(UDAG_InventoryComponent* Inventory, int32 SlotIndex) {
	MoveInventorySlotToCursorImpl(Inventory, SlotIndex, [](int Count){  return Count / 2; });
}

bool UDAG_PlayerInventoryControllerComponent::MoveInventoryHalfSlotToCursor_Validate(UDAG_InventoryComponent* Inventory, int32 SlotIndex) {
	// TODO: Implement me
	return true;
}

void UDAG_PlayerInventoryControllerComponent::MoveInventoryToCursorFullStack_Implementation(UDAG_InventoryComponent* Inventory, int32 SlotIndex) {
	FDAG_InventorySlot ExistingSlot;
	// Pick up max stack of items from the entire inventory, starting with the given slot
	if (Inventory->GetItemAt(SlotIndex, ExistingSlot)) {
		if (CursorSlot.IsEmpty() && ExistingSlot.IsEmpty()) {
			return;
		}
		if (!CursorSlot.IsEmpty() && !ExistingSlot.IsEmpty() && CursorSlot.ItemDefinition != ExistingSlot.ItemDefinition) {
			// cursor and target slot contains different items
			return;
		}

		UDAG_ItemDefinition* ItemDef = CursorSlot.ItemDefinition ? CursorSlot.ItemDefinition : ExistingSlot.ItemDefinition;
		if (!ItemDef) {
			return;
		}
				
		{
			CursorSlot.ItemDefinition = ItemDef;
			int32 DesiredRemoval = CursorSlot.ItemDefinition->MaxStackCount - CursorSlot.StackCount;
			if (DesiredRemoval > 0) {
				int32 ActualRemoved = Inventory->ServerRemoveItemsFromSlot(SlotIndex, CursorSlot.ItemDefinition, DesiredRemoval);
				CursorSlot.StackCount += ActualRemoved;
				DesiredRemoval -= ActualRemoved;

				if (DesiredRemoval > 0) {
					ActualRemoved = Inventory->ServerRemoveItems(CursorSlot.ItemDefinition, DesiredRemoval);
					DesiredRemoval -= ActualRemoved;
					CursorSlot.StackCount += ActualRemoved;
				}
			}
		}
	}
}

bool UDAG_PlayerInventoryControllerComponent::MoveInventoryToCursorFullStack_Validate(UDAG_InventoryComponent* Inventory, int32 SlotIndex) {
	// TODO: Implement me
	return true;
}

void UDAG_PlayerInventoryControllerComponent::ReturnAllCursorItemsToOwningInventory_Implementation() {
	if (!CursorSlot.IsEmpty()) {
		// Return this back to the inventory
		if (LastPickupInventory.IsValid()) {
			// Try to place this back into the same slot
			int32 NumItemsPlaced = LastPickupInventory->ServerAddItemsToSlot(LastPickupSlotIndex, CursorSlot.ItemDefinition, CursorSlot.StackCount);
			check(NumItemsPlaced <= CursorSlot.StackCount);

			// The desired slot is full and we are left with a few more.  place them anywhere in inventory, where we can
			CursorSlot.StackCount -= NumItemsPlaced;
			if (CursorSlot.StackCount) {
				NumItemsPlaced = LastPickupInventory->ServerAddItems(CursorSlot.ItemDefinition, CursorSlot.StackCount);
				check(NumItemsPlaced <= CursorSlot.StackCount);

				CursorSlot.StackCount -= NumItemsPlaced;
				if (CursorSlot.StackCount > 0) {
					// The inventory is full, and we have some items left on hand.  drop them on the ground
					DropCursorItemsToWorld();
				}
			}
		}
	}
}

bool UDAG_PlayerInventoryControllerComponent::ReturnAllCursorItemsToOwningInventory_Validate() {
	// TODO: Implement me
	return true;
}

void UDAG_PlayerInventoryControllerComponent::DropCursorItemsToWorld_Implementation() {
	if (!CursorSlot.IsEmpty()) {
		DropItemsToWorld(CursorSlot.ItemDefinition, CursorSlot.StackCount);
		CursorSlot.Clear();
	}
}

bool UDAG_PlayerInventoryControllerComponent::DropCursorItemsToWorld_Validate() {
	// TODO: Implement me
	return true;
}

void UDAG_PlayerInventoryControllerComponent::DropItemsToWorld(UDAG_ItemDefinition* ItemDefinition, int32 StackCount) {
	// TODO: broadcast me
}


void UDAG_PlayerInventoryControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register properties for replication
	DOREPLIFETIME(UDAG_PlayerInventoryControllerComponent, CursorSlot);
}

