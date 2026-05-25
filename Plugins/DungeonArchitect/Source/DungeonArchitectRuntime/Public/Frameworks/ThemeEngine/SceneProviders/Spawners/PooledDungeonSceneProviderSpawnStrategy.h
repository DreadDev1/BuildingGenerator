//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/Spawners/DungeonSceneProviderSpawnStrategy.h"

#include "GameFramework/Actor.h"

struct FDAVoxelMeshGenerationSettings;

class DUNGEONARCHITECTRUNTIME_API FPooledDungeonSceneProviderSpawnStrategy : public IDungeonSceneProviderSpawnStrategy {
public:
    virtual void Initialize(FDungeonSceneProviderCommandList& InCommandList, ADungeon* InDungeon, const FGuid& InChunkTag) override;
    virtual void Finalize(FDungeonSceneProviderCommandList& InCommandList, ADungeon* InDungeon, const FGuid& InChunkTag) override;
    
    virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) override;
    virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* Mesh, bool bCanAffectNavigation) override;
    virtual void AddLight(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UPointLightComponent* LightTemplate) override;
    virtual void AddParticleSystem(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UParticleSystem* ParticleTemplate) override;
    virtual void AddActorFromTemplate(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UClass* ClassTemplate, bool bCanAffectNavigation) override;
    virtual void AddClonedActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonActorTemplate* ActorTemplate) override;
    virtual void AddGroupActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, const TArray<FName>& ActorNodeIds) override;
    
private:
    void CacheReusableActorPool(FDungeonSceneProviderCommandList& CommandList, const ADungeon* InDungeon, const FGuid& InChunkTag);
    void DestroyUnusedActorsFromPool(FDungeonSceneProviderCommandList& CommandList);
    
protected:
    /** Reuses an object from pool if available. Returns true if an actor was found. If found, it enqueues a reuse command */
    AActor* ReuseFromPool(const FGuid& NodeId, int32 MarkerIndex, const FTransform& InTransform);
    
private:
    struct FReusableItem {
        TWeakObjectPtr<AActor> WeakActor;
        int32 MarkerIndex = INDEX_NONE;
    };

    // Commands to be run in the game thread. Push a command here if we cannot perform a specific tasks in the background thread
    TMap<FGuid, TArray<FReusableItem>> NodeActorPool;
};

