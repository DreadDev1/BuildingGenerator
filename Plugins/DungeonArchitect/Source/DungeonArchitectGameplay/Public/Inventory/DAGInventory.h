//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "DAGInventory.generated.h"

class UDAG_ItemDefinition;

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTGAMEPLAY_API FDAG_InventorySlot : public FFastArraySerializerItem {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UDAG_ItemDefinition> ItemDefinition = nullptr;;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 StackCount = 0;
	
	bool IsEmpty() const;
	void Clear();
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDAG_OnInventorySlotChanged, int32, SlotIndex, const FDAG_InventorySlot&, Slot);

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTGAMEPLAY_API FDAG_Inventory : public FFastArraySerializer {
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FDAG_OnInventorySlotChanged OnSlotChanged;

public:
	void Initialize(int32 InNumSlots);
	bool GetItemAt(int32 Index, FDAG_InventorySlot& OutItem) const;

	void SwapItems(int32 SlotIndexA, int32 SlotIndexB);

	/**
	 * Removes the items to the inventory.
	 * @return The number of items removed
	 */
	int32 RemoveItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/**
	 * Removes the given number of items from the specified slot.
	 * @return the number of items removed
	 */
	int32 RemoveItemsFromSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/** Tries to add the item to the slot, up to a max stack count. The target slot should be empty or of
	 * the same item definition type
	 * @return The number of items added to the slot
	 */
	int32 AddItemsToSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/**
	 * Adds the items to the inventory. It will first fill it up on an existing partially filled slots, then move on to the
	 * empty slots, if needed
	 */
	int32 AddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/** Check if the inventory can add the give amount of items */
	bool CanAddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount) const;

	/** Get the inventory slots (read only) */
	FORCEINLINE const TArray<FDAG_InventorySlot>& GetSlots() const { return Slots; }
	
	int32 GetNumSlots() const;
	FORCEINLINE bool IsSlotEmpty(int32 InSlotIndex) const {
		if (IsSlotValid(InSlotIndex)) {
			return Slots[InSlotIndex].IsEmpty();
		}
		return true;
	}

	FORCEINLINE bool IsSlotValid(int32 InSlotIndex) const {
		return Slots.IsValidIndex(InSlotIndex);
	}
	
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize) {}
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize) {}
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
	void MarkSlotDirty(int32 SlotIndex);

private:
	UPROPERTY()
	TArray<FDAG_InventorySlot> Slots;
};

template<>
struct TStructOpsTypeTraits<FDAG_Inventory> : public TStructOpsTypeTraitsBase2<FDAG_Inventory> {
	enum {
		WithNetDeltaSerializer = true,
	};
};

