//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Spawners/InstancedDungeonSceneProviderSpawnStrategy.h"

#include "Core/Actors/DungeonInstancedMeshActor.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandCore.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandInstanced.h"
#include "Frameworks/ThemeEngine/SceneProviders/Utils/DungeonSceneProviderLib.h"

#include "GameFramework/Actor.h"

void FInstancedDungeonSceneProviderSpawnStrategy::Initialize(FDungeonSceneProviderCommandList& CommandList, ADungeon* InDungeon, const FGuid& InChunkTag) {
	Super::Initialize(CommandList, InDungeon, InChunkTag);
        
	Context = MakeShareable(new FDAInstanceMeshContext());
        
	ADungeonInstancedMeshActor* InstancedMeshActor{};
	if (AActor* PoolActor = ReuseFromPool(FDungeonSceneProviderLib::StaticInstancedMeshNodeId, INDEX_NONE, FTransform::Identity)) {
		if (InChunkTag.IsValid()) {
			check(PoolActor->Tags.Contains(FName(InChunkTag.ToString())));
		}
            
		InstancedMeshActor = Cast<ADungeonInstancedMeshActor>(PoolActor);
		if (IsValid(InstancedMeshActor)) {
			Context->InstancedActor = InstancedMeshActor;
		}
		else {
			// We checked it out from the pool but we're not using it. Destroy it for proper cleanup, Also, this actor shouldn't be in the instanced mesh pool
			CommandList.Add(MakeShareable(new FDungeonSceneProviderCommand_DestroyActor(PoolActor)));
		}
	}

	FDungeonSceneProviderContext SceneContext;
	SceneContext.Dungeon = InDungeon;
	SceneContext.ChunkTag = InChunkTag;
        
	if (!IsValid(InstancedMeshActor)) {
		CommandList.Add(MakeShareable(new FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor(Context, SceneContext)));
	}

	CommandList.Add(MakeShared<FDungeonSceneProviderCommand_InstanceMesh_OnStart>(Context, SceneContext));
        
}

void FInstancedDungeonSceneProviderSpawnStrategy::Finalize(FDungeonSceneProviderCommandList& CommandList, ADungeon* InDungeon, const FGuid& InChunkTag) {
	FDungeonSceneProviderContext SceneContext;
	SceneContext.Dungeon = InDungeon;
	SceneContext.ChunkTag = InChunkTag;

	// Insert the starting commands
	//CommandList.Insert(MakeShared<FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor>(Context, SceneContext), 0);
	//CommandList.Insert(MakeShared<FDungeonSceneProviderCommand_InstanceMesh_OnStart>(Context, SceneContext), 1);

	// Notify the creation has ended, as the last command
	CommandList.Add(MakeShareable(new FDungeonSceneProviderCommand_InstanceMesh_OnStop(Context, SceneContext)));

	Super::Finalize(CommandList, InDungeon, InChunkTag);
        
	Context = nullptr;
}

void FInstancedDungeonSceneProviderSpawnStrategy::AddStaticMesh(const FDungeonSceneProviderContext& SceneProviderContext, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) {
	CommandList.Add(MakeShared<FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance>(Context, SceneProviderContext, Mesh, SceneProviderContext.Transform));
}

void FInstancedDungeonSceneProviderSpawnStrategy::AddStaticMesh(const FDungeonSceneProviderContext& SceneProviderContext, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* StaticMesh, bool bCanAffectNavigation) {
	CommandList.Add(MakeShared<FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance>(Context, SceneProviderContext, StaticMesh, SceneProviderContext.Transform, bCanAffectNavigation));
}
