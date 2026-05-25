//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"

class FDungeonSceneProviderCommandList;
class ADungeon;
class AActor;

struct FDungeonSceneCollectorActorPoolItem {
	TWeakObjectPtr<AActor> WeakActor;
	int32 MarkerIndex = INDEX_NONE;
};

class FDungeonSceneCollectorActorPool {
public:
	AActor* ReuseFromPool(const FGuid& InNodeId, int32 InMarkerIndex, const FTransform& InTransform);

private:
	void AddToPool(const FGuid& InNodeId, int32 InMarkerIndex, AActor* InActor);
	void DestroyPool(FDungeonSceneProviderCommandList& CommandList);
	
private:
    TMap<FGuid, TArray<FDungeonSceneCollectorActorPoolItem>> NodeActorPool;
	friend class FDungeonSceneCollector;
};

class FDungeonSceneCollector {
public:
	void Collect(ADungeon* InDungeon);
	void DestroyUnusedActors(FDungeonSceneProviderCommandList& CommandList);
	void DeregisterChunk(const FGuid& InChunkId);
	TSharedRef<FDungeonSceneCollectorActorPool> GetChunkPool(const FGuid& InChunkId);

private:
	TWeakObjectPtr<ADungeon> Dungeon;
	TMap<FGuid, TSharedPtr<FDungeonSceneCollectorActorPool>> ChunkPools;
};

