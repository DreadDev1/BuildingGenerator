//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandInstanced.h"

#include "Core/Actors/DungeonInstancedMeshActor.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"
#include "Frameworks/ThemeEngine/SceneProviders/Utils/DungeonSceneProviderLib.h"

FDungeonSceneProviderCommand_InstanceMeshBase::FDungeonSceneProviderCommand_InstanceMeshBase(
	const TSharedPtr<FDAInstanceMeshContext>& InISMContext, const FDungeonSceneProviderContext& SceneContext): FDungeonSceneProviderCommand(SceneContext), ISMContext(InISMContext) {
}

bool FDungeonSceneProviderCommand_InstanceMeshBase::IsValid() const {
	return (ISMContext.IsValid() && ISMContext->InstancedActor.IsValid());
}

FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor::FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor(
	const TSharedPtr<FDAInstanceMeshContext>& InContext, const FDungeonSceneProviderContext& SceneContext): FDungeonSceneProviderCommand_InstanceMeshBase(InContext, SceneContext) {
}

void FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor::ExecuteImpl(UWorld* World) {
	// Search for an existing actor that holds the instanced static meshes
	const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(Context.Dungeon.Get());
	ADungeonInstancedMeshActor* InstancedActor = nullptr;
	for (TObjectIterator<AActor> ActorItr; ActorItr; ++ActorItr) {
		if (!ActorItr->IsValidLowLevel() || !::IsValid(*ActorItr)) continue;
		if (ActorItr->ActorHasTag(DungeonTag) && ActorItr->ActorHasTag(FDungeonThemeEngineUtils::CreateNodeTagFromId(FDungeonSceneProviderLib::StaticInstancedMeshNodeId))) {
			ADungeonInstancedMeshActor* InstancedActorPtr = Cast<ADungeonInstancedMeshActor>(*ActorItr);
			if (InstancedActorPtr && ::IsValid(InstancedActorPtr)) {
				InstancedActor = InstancedActorPtr;
				break;
			}
		}
	}

	if (!InstancedActor) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.OverrideLevel = Context.LevelOverride.Get();
		InstancedActor = World->SpawnActor<ADungeonInstancedMeshActor>(SpawnParams);
		PostInitializeActor(InstancedActor);
	}


	ISMContext->InstancedActor = InstancedActor;

}

void FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor::UpdateExecutionPriority(const FVector& BuildPosition) {
	ExecutionPriority = -MAX_int32;
}

FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor::FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor(
	const TSharedPtr<FDAInstanceMeshContext>& InContext, const FDungeonSceneProviderContext& SceneContext): FDungeonSceneProviderCommand_InstanceMeshBase(InContext, SceneContext) {
}

void FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor::ExecuteImpl(UWorld* World) {
	// Search for an existing actor that holds the instanced static meshes
	ADungeonInstancedMeshActor* InstancedActor = nullptr;
        
	if (!InstancedActor) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.OverrideLevel = Context.LevelOverride.Get();
		InstancedActor = World->SpawnActor<ADungeonInstancedMeshActor>(SpawnParams);
		PostInitializeActor(InstancedActor);
	}

	ISMContext->InstancedActor = InstancedActor;
}

void FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor::UpdateExecutionPriority(const FVector& BuildPosition) {
	ExecutionPriority = -MAX_int32;
}

FDungeonSceneProviderCommand_InstanceMesh_OnStart::FDungeonSceneProviderCommand_InstanceMesh_OnStart(
	TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext): FDungeonSceneProviderCommand_InstanceMeshBase(InISMContext, SceneContext) {
}

void FDungeonSceneProviderCommand_InstanceMesh_OnStart::ExecuteImpl(UWorld* World) {
	if (!IsValid()) { return; }
	ISMContext->InstancedActor->OnBuildStart();
}

void FDungeonSceneProviderCommand_InstanceMesh_OnStart::UpdateExecutionPriority(const FVector& BuildPosition) {
	ExecutionPriority = -MAX_int32 + 1;
}

FDungeonSceneProviderCommand_InstanceMesh_OnStop::FDungeonSceneProviderCommand_InstanceMesh_OnStop(
	TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext): FDungeonSceneProviderCommand_InstanceMeshBase(InISMContext, SceneContext) {
}

void FDungeonSceneProviderCommand_InstanceMesh_OnStop::ExecuteImpl(UWorld* World) {
	if (!IsValid()) { return; }
	ISMContext->InstancedActor->OnBuildStop();
}

void FDungeonSceneProviderCommand_InstanceMesh_OnStop::UpdateExecutionPriority(const FVector& BuildPosition) {
	ExecutionPriority = MAX_int32;
}

FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance::FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance(TSharedPtr<FDAInstanceMeshContext> InISMContext,
		const FDungeonSceneProviderContext& SceneContext, UDungeonMesh* InMesh, const FTransform& InTransform)
	: FDungeonSceneProviderCommand_InstanceMeshBase(InISMContext, SceneContext), Mesh(InMesh), Transform(InTransform)
{
}

FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance::FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance(
		TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext,
		UStaticMesh* InStaticMesh, const FTransform& InTransform, bool bCanAffectNavigation)
	: FDungeonSceneProviderCommand_InstanceMeshBase(InISMContext, SceneContext), Transform(InTransform)
{
	Mesh = NewObject<UDungeonMesh>();
	Mesh->StaticMesh = InStaticMesh;
	Mesh->bCanEverAffectNavigation = bCanAffectNavigation;
	Mesh->CalculateHashCode();
}

void FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance::ExecuteImpl(UWorld* World) {
	if (!IsValid() || !Mesh) { return; }
	ISMContext->InstancedActor->AddMeshInstance(Mesh.Get(), Transform);
}

void FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance::UpdateExecutionPriority(const FVector& BuildPosition) {
	UpdateExecutionPriorityByDistance(BuildPosition, Transform);
}

void FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance::AddReferencedObjects(FReferenceCollector& Collector) {
	if (Mesh) {
		Collector.AddReferencedObject(Mesh);
	}
}

FString FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance::GetReferencerName() const {
	static const FString ReferencerName = TEXT("FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance");
	return ReferencerName;
}

