//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/DAGInventory.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "UObject/ScriptInterface.h"
#include "DAGPickable.generated.h"

class UDAG_InventoryComponent;
class AActor;

/**  */
UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UDAG_Pickable : public UInterface
{
	GENERATED_BODY()
};

/**  */
class IDAG_Pickable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	virtual FDAG_Inventory& GetPickupInventory() const = 0;
};

/**  */
UCLASS()
class UDAG_PickableStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UDAG_PickableStatics();

public:
	UFUNCTION(BlueprintPure, Category="Dungeon Architect")
	static TScriptInterface<IDAG_Pickable> GetFirstPickableFromActor(AActor* Actor);

	/** Checks if the inventory has space to add the entire pickup inventory in it */
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static bool CanAddPickupToInventory(UDAG_InventoryComponent* InventoryComponent, TScriptInterface<IDAG_Pickable> Pickup);

	/**
	 * Adds the pickup to the inventory.  does not check if the inventory can hold it.
	 * If the inventory cannot hold it, it would be discarded. It's the caller's job to check before adding
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (WorldContext = "InventoryComponent"), Category="Dungeon Architect")
	static void AddPickupToInventory(UDAG_InventoryComponent* InventoryComponent, TScriptInterface<IDAG_Pickable> Pickup);
};

