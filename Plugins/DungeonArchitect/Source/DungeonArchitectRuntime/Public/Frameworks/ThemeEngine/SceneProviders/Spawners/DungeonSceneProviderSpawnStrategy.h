//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/VDB/VDBUtils.h"

class UDungeonActorTemplate;
class UPointLightComponent;
class UDungeonMesh;
class UStaticMesh;
class UParticleSystem;
class UClass;
class AActor;
class ADungeon;
class FDungeonSceneProviderCommandList;
struct FDungeonSceneProviderContext;

namespace DA {
	typedef TSharedPtr<struct FDungeonMeshGeometry, ESPMode::ThreadSafe> FDungeonMeshGeometryPtr;
}

class DUNGEONARCHITECTRUNTIME_API IDungeonSceneProviderSpawnStrategy {
public:
	virtual ~IDungeonSceneProviderSpawnStrategy() {}

	virtual void Initialize(FDungeonSceneProviderCommandList& CommandList, ADungeon* InDungeon, const FGuid& InChunkTag) {}
	virtual void Finalize(FDungeonSceneProviderCommandList& CommandList, ADungeon* InDungeon, const FGuid& InChunkTag) {}
    
	virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) = 0;
	virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* Mesh, bool bCanAffectNavigation) = 0;
	virtual void AddLight(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UPointLightComponent* LightTemplate) = 0;
	virtual void AddParticleSystem(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UParticleSystem* ParticleTemplate) = 0;
	virtual void AddActorFromTemplate(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UClass* ClassTemplate, bool bCanAffectNavigation) = 0;
	virtual void AddClonedActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonActorTemplate* ActorTemplate) = 0;
	virtual void AddGroupActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, const TArray<FName>& ActorNodeIds) = 0;
}; 

