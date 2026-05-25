//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/DAGInventory.h"

#include "Components/ControllerComponent.h"
#include "DAGPlayerInventoryControllerComponent.generated.h"

class UDAG_InventoryComponent;

UCLASS(Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTGAMEPLAY_API UDAG_PlayerInventoryControllerComponent : public UControllerComponent {
	GENERATED_BODY()
public:
	UDAG_PlayerInventoryControllerComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Moves the cursor held items to the given inventory slot
	 */
    UFUNCTION(Server, Reliable, WithValidation)
	void MoveCursorToInventorySlot(UDAG_InventoryComponent* Inventory, int32 SlotIndex, int32 StackCount, bool bAllowSwapping);
	
	/**
	 * Moves the inventory slot items to the cursor slot
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void MoveInventorySlotToCursor(UDAG_InventoryComponent* Inventory, int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void MoveInventoryHalfSlotToCursor(UDAG_InventoryComponent* Inventory, int32 SlotIndex);
	
	/**
	 * Fills up the cursor slot with the item held in the stack. When the max stack is not reached on the cursor
	 * after the transfer, it tries to extract more items from the inventory till the cursor stack is full
	 * This mimics the double click behaviour over an item in the inventory (e.g. in minecraft)
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void MoveInventoryToCursorFullStack(UDAG_InventoryComponent* Inventory, int32 SlotIndex);

	/**
	 * Return all the cursor held items to the inventory, starting with the slot index, when that gets filled,
	 * place it on any other free slot.  When that's filled, the rest of the items are dropped on to the world
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ReturnAllCursorItemsToOwningInventory();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void DropCursorItemsToWorld();

protected:
	void DropItemsToWorld(UDAG_ItemDefinition* ItemDefinition, int32 StackCount);
	void MoveInventorySlotToCursorImpl(UDAG_InventoryComponent* Inventory, int32 SlotIndex, const TFunction<int(int)>& FnCountToRemove);
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	FDAG_InventorySlot CursorSlot;
	
	int32 LastPickupSlotIndex = INDEX_NONE;
	TWeakObjectPtr<UDAG_InventoryComponent> LastPickupInventory;
};

