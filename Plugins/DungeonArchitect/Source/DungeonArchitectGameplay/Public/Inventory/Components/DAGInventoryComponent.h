//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/DAGInventory.h"

#include "Components/ActorComponent.h"
#include "DAGInventoryComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTGAMEPLAY_API UDAG_InventoryComponent : public UActorComponent {
	GENERATED_BODY()
	
public:
	UDAG_InventoryComponent();

	virtual void BeginPlay() override;
	

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category="Dungeon Architect Inventory")
	void Initialize(int32 InNumSlots);
	
	/** Swap out the items between two inventory slots */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category="Dungeon Architect Inventory")
	void SwapItems(int32 SlotIndexA, int32 SlotIndexB);

	/**
	 * Remove the items to the inventory.
	 * @return The number of items removed
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category="Dungeon Architect Inventory")
	void RemoveItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/**
	 * Remove the given number of items from the specified slot.
	 * @return the number of items removed
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category="Dungeon Architect Inventory")
	void RemoveItemsFromSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/** Tries to add the item to the slot, up to a max stack count. The target slot should be empty or of
	 * the same item definition type
	 * @return The number of items added to the slot
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category="Dungeon Architect Inventory")
	void AddItemsToSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	/**
	 * Adds the items to the inventory. It will first fill it up on an existing partially filled slots, then move on to the
	 * empty slots, if needed
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category="Dungeon Architect Inventory")
	void AddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect Inventory")
	bool CanAddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect Inventory")
	bool CanAddInventory(const FDAG_Inventory& InventoryToAdd);

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect Inventory")
	bool HasItems(const UDAG_ItemDefinition* InItemDefinition, int32 InCount) const;
	
public:
	void ServerInitialize(int32 InNumSlots);
	void ServerSwapItems(int32 SlotIndexA, int32 SlotIndexB);
	int32 ServerAddItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);
	int32 ServerAddItemsToSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount);
	int32 ServerRemoveItems(UDAG_ItemDefinition* InItemDefinition, int32 InCount);
	int32 ServerRemoveItemsFromSlot(int32 SlotIndex, UDAG_ItemDefinition* InItemDefinition, int32 InCount);
	
public:
	///////
	/** Get the no. slots in the inventory */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon Architect Inventory")
	int32 GetNumSlots() const;
	
	/** Get the item at the desired slot index */
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect Inventory")
	bool GetItemAt(int32 Index, FDAG_InventorySlot& OutItem);


	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FDAG_Inventory Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InitialNumSlots = 1;
};



