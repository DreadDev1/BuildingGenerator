//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Interaction/DAGPickable.h"

#include "Inventory/Components/DAGInventoryComponent.h"

#include "GameFramework/Actor.h"

UDAG_PickableStatics::UDAG_PickableStatics()
	: Super(FObjectInitializer::Get())
{
}


TScriptInterface<IDAG_Pickable> UDAG_PickableStatics::GetFirstPickableFromActor(AActor* Actor)
{
	// If the actor is directly pickupable, return that.
	TScriptInterface<IDAG_Pickable> PickupableActor(Actor);
	if (PickupableActor)
	{
		return PickupableActor;
	}

	// If the actor isn't pickupable, it might have a component that has a pickupable interface.
	TArray<UActorComponent*> PickupableComponents = Actor ? Actor->GetComponentsByInterface(UDAG_Pickable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		// Get first pickupable, if the user needs more sophisticated pickup distinction, will need to be solved elsewhere.
		return TScriptInterface<IDAG_Pickable>(PickupableComponents[0]);
	}

	return TScriptInterface<IDAG_Pickable>();
}

void UDAG_PickableStatics::AddPickupToInventory(UDAG_InventoryComponent* InventoryComponent, TScriptInterface<IDAG_Pickable> Pickup) {
	if (InventoryComponent && Pickup) {
		const FDAG_Inventory& PickupInventory = Pickup->GetPickupInventory();
		for (const FDAG_InventorySlot& PickupSlot : PickupInventory.GetSlots()) {
			InventoryComponent->AddItems(PickupSlot.ItemDefinition, PickupSlot.StackCount);
		}
	}
}

bool UDAG_PickableStatics::CanAddPickupToInventory(UDAG_InventoryComponent* InventoryComponent, TScriptInterface<IDAG_Pickable> Pickup) {
	if (!InventoryComponent) {
		return false;
	}
	return InventoryComponent->CanAddInventory(Pickup->GetPickupInventory());
	
}

