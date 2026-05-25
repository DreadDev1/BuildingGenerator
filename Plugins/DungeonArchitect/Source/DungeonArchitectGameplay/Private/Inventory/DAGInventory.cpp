//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/DAGInventory.h"

#include "Item/DAGItemDefinition.h"

/////////////////// FDAG_InventorySlot ///////////////////

bool FDAG_InventorySlot::IsEmpty() const {
	return ItemDefinition == nullptr || StackCount == 0;
}

void FDAG_InventorySlot::Clear() {
	ItemDefinition = nullptr;
	StackCount = 0;
}


/////////////////// FDAG_Inventory ///////////////////

void FDAG_Inventory::Initialize(int32 InNumSlots) {
	if (Slots.Num() != 0) {
		Slots.Reset();
		MarkArrayDirty();
	}
	
	for (int i = 0; i < InNumSlots; i++) {
		FDAG_InventorySlot Item;
		const int32 SlotIndex = Slots.Add(Item);
		MarkSlotDirty(SlotIndex);
	}
}

bool FDAG_Inventory::GetItemAt(int32 Index, FDAG_InventorySlot& OutItem) const {
	if (!Slots.IsValidIndex(Index)) {
		return false;
	}

	OutItem = Slots[Index];
	return true;
}

void FDAG_Inventory::SwapItems(int32 SlotIndexA, int32 SlotIndexB) {
	if (!Slots.IsValidIndex(SlotIndexA) || !Slots.IsValidIndex(SlotIndexB)) {
		return;
	}

	FDAG_InventorySlot TempA = Slots[SlotIndexA];
	Slots[SlotIndexA] = Slots[SlotIndexB];
	Slots[SlotIndexB] = TempA;

	MarkSlotDirty(SlotIndexA);
	MarkSlotDirty(SlotIndexB);
}

int32 FDAG_Inventory::RemoveItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	int32 RemainingToRemove = InCount;
	for (int SlotIdx = 0; SlotIdx < Slots.Num(); SlotIdx++) {
		FDAG_InventorySlot& Slot = Slots[SlotIdx];
		if (!Slot.IsEmpty() && Slot.ItemDefinition == InItemDefinition) {
			int32 NumRemoved = RemoveItemsFromSlot(SlotIdx, InItemDefinition, RemainingToRemove);
			if (NumRemoved > 0) {
				RemainingToRemove -= NumRemoved;
				check(RemainingToRemove >= 0);
				if (RemainingToRemove == 0) {
					break;
				}
			}
		}
	}
	return InCount - RemainingToRemove;
}

int32 FDAG_Inventory::RemoveItemsFromSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	if (!Slots.IsValidIndex(SlotIndex)) {
		return 0;
	}
		
	FDAG_InventorySlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty() || Slot.ItemDefinition != InItemDefinition) {
		return 0;
	}

	int32 NumItemsToRemove = FMath::Min(Slot.StackCount, InCount);
	if (NumItemsToRemove > 0) {
		Slot.StackCount -= NumItemsToRemove;
		check(Slot.StackCount >= 0);
		if (Slot.StackCount == 0) {
			Slot.Clear();
		}
		MarkSlotDirty(SlotIndex);
	}
	return NumItemsToRemove;
}

int32 FDAG_Inventory::AddItemsToSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	if (!IsSlotValid(SlotIndex) || InItemDefinition == nullptr || InCount == 0) {
		return 0;
	}
		
	FDAG_InventorySlot& Slot = Slots[SlotIndex];
	if (!Slot.IsEmpty() && Slot.ItemDefinition != InItemDefinition) {
		// We already have a different item in this slot
		return 0;
	}

	if (Slot.ItemDefinition == nullptr || Slot.StackCount < 0) {
		Slot.StackCount = 0;
	}
		
	int32 StacksToAdd = FMath::Min(InItemDefinition->MaxStackCount - Slot.StackCount, InCount);
	StacksToAdd = FMath::Max(StacksToAdd, 0);

	if (StacksToAdd > 0) {
		Slot.ItemDefinition = InItemDefinition;
		Slot.StackCount += StacksToAdd;
		MarkSlotDirty(SlotIndex);
	}
		
	return StacksToAdd;
}

int32 FDAG_Inventory::AddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) {
	int32 Remaining = InCount;

	TArray<int32> SlotCandidates;
	// Get partially filled slots of the same item type
	for (int SlotIdx = 0; SlotIdx < Slots.Num(); SlotIdx++) {
		FDAG_InventorySlot& Slot = Slots[SlotIdx];
		if (!Slot.IsEmpty() && Slot.ItemDefinition == InItemDefinition && Slot.StackCount < InItemDefinition->MaxStackCount) {
			SlotCandidates.Add(SlotIdx);
		}
	}

	// Get the empty slots
	for (int SlotIdx = 0; SlotIdx < Slots.Num(); SlotIdx++) {
		if (Slots[SlotIdx].IsEmpty()) {
			SlotCandidates.Add(SlotIdx);
		}
	}

	for (int32 SlotIdx : SlotCandidates) {
		int32 NumAdded = AddItemsToSlot(SlotIdx, InItemDefinition, Remaining);
		check(NumAdded <= Remaining && NumAdded > 0);
		Remaining -= NumAdded;

		if (Remaining == 0) {
			break;
		}
	}
		
	return InCount - Remaining;
}

bool FDAG_Inventory::CanAddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) const {
	if (!InItemDefinition) {
		return false;
	}
	if (InCount <= 0) {
		return true;
	}

	int32 Remaining = InCount;
	for (const FDAG_InventorySlot& Slot : Slots) {
		int32 AvailableInSlot{};
		if (Slot.IsEmpty()) {
			AvailableInSlot = InItemDefinition->MaxStackCount;
		}
		else if (Slot.ItemDefinition == InItemDefinition) {
			AvailableInSlot = FMath::Max(0, InItemDefinition->MaxStackCount - Slot.StackCount);
		}

		Remaining -= AvailableInSlot;
		if (Remaining <= 0) {
			break;
		}
	}
	return (Remaining <= 0);
}

int32 FDAG_Inventory::GetNumSlots() const {
	return Slots.Num();
}

void FDAG_Inventory::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize) {
	for (int32 SlotIdx : ChangedIndices) {
		OnSlotChanged.Broadcast(SlotIdx, Slots[SlotIdx]);
	}
}

bool FDAG_Inventory::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms) {
	return FFastArraySerializer::FastArrayDeltaSerialize<FDAG_InventorySlot, FDAG_Inventory>(Slots, DeltaParms, *this);
}

void FDAG_Inventory::MarkSlotDirty(int32 SlotIndex) {
	if (Slots.IsValidIndex(SlotIndex)) {
		MarkItemDirty(Slots[SlotIndex]);
		OnSlotChanged.Broadcast(SlotIndex, Slots[SlotIndex]);
	}
}

