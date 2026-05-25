//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/SnapDungeonBuilderBase.h"

#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"

void USnapDungeonBuilderBase::DestroyDungeonImpl(UWorld* World) {
	Super::DestroyDungeonImpl(World);
	
	USnapDungeonModelBase* SnapModelBase = Cast<USnapDungeonModelBase>(DungeonModel);
	
	// Destroy all the spawned connection door actors
	for (const FSnapConnectionInstance& Connection : SnapModelBase->Connections) {
		for (TWeakObjectPtr<AActor> SpawnedDoorActorPtr : Connection.SpawnedDoorActors) {
			if (SpawnedDoorActorPtr.IsValid()) {
				SpawnedDoorActorPtr->Destroy();
			}
		}
	}
}

