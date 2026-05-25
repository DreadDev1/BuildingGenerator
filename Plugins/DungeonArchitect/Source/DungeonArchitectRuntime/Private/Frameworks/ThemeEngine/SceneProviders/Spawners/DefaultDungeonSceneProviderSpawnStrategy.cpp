//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Spawners/DefaultDungeonSceneProviderSpawnStrategy.h"

#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandCore.h"

/////////////////////////////// FDefaultDungeonSceneProviderSpawnStrategy ///////////////////////////////
void FDefaultDungeonSceneProviderSpawnStrategy::AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_CreateMesh>(Context, Mesh));
}

void FDefaultDungeonSceneProviderSpawnStrategy::AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* Mesh, bool bCanAffectNavigation) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_CreateMeshSimple>(Context, Mesh, bCanAffectNavigation));
}

void FDefaultDungeonSceneProviderSpawnStrategy::AddLight(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UPointLightComponent* LightTemplate) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_AddLight>(Context, LightTemplate));
}

void FDefaultDungeonSceneProviderSpawnStrategy::AddParticleSystem(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UParticleSystem* ParticleTemplate) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_AddParticleSystem>(Context, ParticleTemplate));
}

void FDefaultDungeonSceneProviderSpawnStrategy::AddActorFromTemplate(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UClass* ClassTemplate, bool bCanAffectNavigation) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_AddActor>(Context, ClassTemplate, bCanAffectNavigation));
}

void FDefaultDungeonSceneProviderSpawnStrategy::AddClonedActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonActorTemplate* ActorTemplate) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_CloneActor>(Context, ActorTemplate));
}

void FDefaultDungeonSceneProviderSpawnStrategy::AddGroupActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, const TArray<FName>& ActorNodeIds) {
    CommandList.Add(MakeShared<FDungeonSceneProviderCommand_CreateGroupActor>(Context, ActorNodeIds));
}

