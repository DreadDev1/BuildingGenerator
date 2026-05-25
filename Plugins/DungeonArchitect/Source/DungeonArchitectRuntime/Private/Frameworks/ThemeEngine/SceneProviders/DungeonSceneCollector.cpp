//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneCollector.h"

#include "Core/Actors/DungeonInstancedMeshActor.h"
#include "Core/Dungeon.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/SceneProviders/Utils/DungeonSceneProviderLib.h"

//////////////////////////////// FDungeonSceneCollectorActorPool ////////////////////////////////
AActor* FDungeonSceneCollectorActorPool::ReuseFromPool(const FGuid& InNodeId, int32 InMarkerIndex, const FTransform& InTransform) {
	// Check if we have a free actor of this type in the pool
	if (NodeActorPool.Contains(InNodeId)) {
		TArray<FDungeonSceneCollectorActorPoolItem>& NodeActorList = NodeActorPool[InNodeId];
		if (NodeActorList.Num() > 0) {
			// A free actor is available in the pool.
            
			// Try to find an actor that shares the same marker index
			for (int i = 0; i < NodeActorList.Num(); i++) {
				TWeakObjectPtr<AActor> Actor = NodeActorList[i].WeakActor;
				if (NodeActorList[i].MarkerIndex == InMarkerIndex) {
					NodeActorList.RemoveAtSwap(i);
					return Actor.Get();
				}
			}
            
			// Next, try to find an actor that shares the same transform
			for (int i = 0; i < NodeActorList.Num(); i++) {
				TWeakObjectPtr<AActor> Actor = NodeActorList[i].WeakActor;
				if (Actor.IsValid() && Actor->GetTransform().Equals(InTransform)) {
					NodeActorList.RemoveAtSwap(i);
					return Actor.Get();
				}
			}

			// We did not find a free actor that has the requested transform or the marker index
			// Grab the first free actor and remove it from the pool
			for (int i = 0; i < NodeActorList.Num(); i++) {
				TWeakObjectPtr<AActor> Actor = NodeActorList[i].WeakActor;
				if (Actor.IsValid()) {
					NodeActorList.RemoveAtSwap(i);
					return Actor.Get();
				}
			}
		}
	}

	// No free objects of this type in the pool
	return nullptr;
}

void FDungeonSceneCollectorActorPool::AddToPool(const FGuid& InNodeId, int32 InMarkerIndex, AActor* InActor) {
	TArray<FDungeonSceneCollectorActorPoolItem>& ActorList = NodeActorPool.FindOrAdd(InNodeId);
	ActorList.Add({ InActor, InMarkerIndex });
}

void FDungeonSceneCollectorActorPool::DestroyPool(FDungeonSceneProviderCommandList& CommandList) {
	for (const auto& Entry : NodeActorPool) {
		const TArray<FDungeonSceneCollectorActorPoolItem>& NodePoolItems = Entry.Value;
		for (const FDungeonSceneCollectorActorPoolItem& PoolItem : NodePoolItems) {
			if (PoolItem.WeakActor.IsValid()) {
				PoolItem.WeakActor->Destroy();
			}
		}
	} 
}

//////////////////////////////// FDungeonSceneCollector ////////////////////////////////
void FDungeonSceneCollector::Collect(ADungeon* InDungeon) {
	Dungeon = InDungeon;
	if (UWorld* World = InDungeon ? InDungeon->GetWorld() : nullptr) {
		ChunkPools.Reset();
		const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(InDungeon);

		// Collect all actors that have a "Dungeon" tag
		for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
			AActor* Actor = *ActorItr;
			if (!IsValid(Actor) || !Actor->ActorHasTag(DungeonTag)) {
				continue;
			}

			UDungeonSpawnDataComponent* SpawnData = Actor->FindComponentByClass<UDungeonSpawnDataComponent>();
			if (!IsValid(SpawnData)) {
				continue;
			}

			if (!SpawnData->OwningDungeon.IsValid() || SpawnData->OwningDungeon != InDungeon) {
				// Does not belong to the same dungeon
				continue;
			}
			
			const FGuid& ChunkId = SpawnData->ChunkId;
		    
			// Get the chunk pool
			TSharedRef<FDungeonSceneCollectorActorPool> ChunkPool = GetChunkPool(ChunkId);

			FGuid NodeId = Actor->IsA<ADungeonInstancedMeshActor>()
					? FDungeonSceneProviderLib::StaticInstancedMeshNodeId
					: SpawnData->ThemeNodeId;
			ChunkPool->AddToPool(NodeId, SpawnData->MarkerIndex, Actor);
		}
	}
}

void FDungeonSceneCollector::DestroyUnusedActors(FDungeonSceneProviderCommandList& CommandList) {
	// Anything remaining in the pool would be marked for destruction
	for (auto& Entry : ChunkPools) {
		TSharedPtr<FDungeonSceneCollectorActorPool>& ChunkPool = Entry.Value;
		if (ChunkPool.IsValid()) {
			ChunkPool->DestroyPool(CommandList);
		}
	}
	ChunkPools = {};
}

void FDungeonSceneCollector::DeregisterChunk(const FGuid& InChunkId) {
	ChunkPools.Remove(InChunkId);
}

TSharedRef<FDungeonSceneCollectorActorPool> FDungeonSceneCollector::GetChunkPool(const FGuid& InChunkId) {
	TSharedPtr<FDungeonSceneCollectorActorPool>& ChunkPool = ChunkPools.FindOrAdd(InChunkId);
	if (!ChunkPool.IsValid()) {
		ChunkPool = MakeShared<FDungeonSceneCollectorActorPool>();
	}
	return ChunkPool.ToSharedRef();
}

