//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/Spawners/PooledDungeonSceneProviderSpawnStrategy.h"

struct FDAInstanceMeshContext;

class DUNGEONARCHITECTRUNTIME_API FInstancedDungeonSceneProviderSpawnStrategy : public FPooledDungeonSceneProviderSpawnStrategy {
    typedef FPooledDungeonSceneProviderSpawnStrategy Super;
public:
    
    virtual void Initialize(FDungeonSceneProviderCommandList& CommandList, ADungeon* InDungeon, const FGuid& InChunkTag) override;
    virtual void Finalize(FDungeonSceneProviderCommandList& CommandList, ADungeon* InDungeon, const FGuid& InChunkTag) override;
    virtual void AddStaticMesh(const FDungeonSceneProviderContext& SceneProviderContext, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) override;
    virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* StaticMesh, bool bCanAffectNavigation) override;

private:
    TSharedPtr<FDAInstanceMeshContext> Context;
};

