//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/Spawners/DungeonSceneProviderSpawnStrategy.h"
#include "Frameworks/Voxel/Meshing/VoxelMeshGenerationSettings.h"

class DUNGEONARCHITECTRUNTIME_API FDefaultDungeonSceneProviderSpawnStrategy : public IDungeonSceneProviderSpawnStrategy {
public:
	virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) override;
	virtual void AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* Mesh, bool bCanAffectNavigation) override;
	virtual void AddLight(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UPointLightComponent* LightTemplate) override;
	virtual void AddParticleSystem(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UParticleSystem* ParticleTemplate) override;
	virtual void AddActorFromTemplate(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UClass* ClassTemplate, bool bCanAffectNavigation) override;
	virtual void AddClonedActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonActorTemplate* ActorTemplate) override;
	virtual void AddGroupActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, const TArray<FName>& ActorNodeIds) override;
};

