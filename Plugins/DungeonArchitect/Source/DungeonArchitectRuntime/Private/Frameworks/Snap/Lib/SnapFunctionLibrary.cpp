//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/SnapFunctionLibrary.h"

#include "Core/Utils/DungeonConstants.h"
#include "Core/Utils/DungeonModelHelper.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"
#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"

DEFINE_LOG_CATEGORY_STATIC(LogSnapDungeonFunctionLibrary, Log, All);


bool USnapDungeonFunctionLibrary::BuildSnapThemedDungeon(ADungeon* Dungeon, UWorld* World, AActor* ParentActor, FName ThemeId, const FString& ConnectionMarkerPrefix) {
	if (!Dungeon) {
		UE_LOG(LogSnapDungeonFunctionLibrary, Log, TEXT("Dungeon actor is NULL"));
		return false;
	}
	
	if (!World) {
		World = Dungeon->GetWorld();
	}
	const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(Dungeon);

	auto SetupSpawnedActor = [&DungeonTag, Dungeon, ParentActor](AActor* InActor) {
		if (InActor) {
			InActor->Tags.Add(DungeonTag);
			InActor->Tags.Add(FDungeonSceneProviderCommand::TagComplexActor);
			InActor->SetActorEnableCollision(false);
			if (InActor->GetRootComponent()) {
				InActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			}
			if (ParentActor) {
				InActor->AttachToActor(ParentActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
			}
			
#if WITH_EDITOR
			if (Dungeon) {
				const FString FolderName = Dungeon->ItemFolderPath.ToString() + FDungeonArchitectConstants::DungeonFolderPathSuffix_SnapInstances;
				InActor->SetFolderPath(FName(FolderName));
			}
#endif // WITH_EDITOR
		}
	};

	TFunction<FTransform(const FTransform& WorldTransform)> GetWorldToTargetTransform;
	{
		const FTransform InverseDungeonTransform = Dungeon->GetTransform().Inverse();
		const FTransform LocalToTargetTransform = ParentActor ? ParentActor->GetTransform() : FTransform::Identity;
		GetWorldToTargetTransform = [=](const FTransform& WorldTransform) {
			return WorldTransform
					* InverseDungeonTransform
					* LocalToTargetTransform;
		};
	}

	USnapDungeonModelBase* Model = Cast<USnapDungeonModelBase>(Dungeon->DungeonModel);
	if (!Model) {
		return false;
	}

	for (const FSnapModuleInstanceSerializedData& ModuleInstance : Model->ModuleInstances) {
        TArray<AActor*> ChunkActors;
        TMap<AActor*, AActor*> TemplateToSpawnedActorMap;

		TSoftObjectPtr<UWorld> LevelToLoad = !ThemeId.IsNone()
			? ModuleInstance.GetThemedLevel(ThemeId)
			: ModuleInstance.Level;

		if (LevelToLoad.IsNull()) {
			LevelToLoad = ModuleInstance.Level;
		}

		const FTransform ChunkTransform = GetWorldToTargetTransform(ModuleInstance.WorldTransform);
        const FGuid AbstractNodeId = ModuleInstance.ModuleInstanceId;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.OverrideLevel = World->PersistentLevel;
		ADungeonStreamingChunkLevelInstance* LevelInstanceActor = World->SpawnActor<ADungeonStreamingChunkLevelInstance>(ADungeonStreamingChunkLevelInstance::StaticClass(), ChunkTransform,  SpawnParams);

		FGuid ChunkNetworkUID = FGuid(); // TODO: Implement me, this needs to combine the theme hash with the module instance id
		LevelInstanceActor->SetNetworkGuid(ChunkNetworkUID);
		LevelInstanceActor->SetWorldAssetImpl(LevelToLoad);
		LevelInstanceActor->LoadLevelInstance();
		SetupSpawnedActor(LevelInstanceActor);
    }

	// Build the doors
	ULevel* PersistentLevel = World->PersistentLevel;
	for (const FSnapConnectionInstance& ConnectionInfo : Model->Connections) {
		FTransform ConnectionTransform = GetWorldToTargetTransform(ConnectionInfo.WorldTransform);
		ASnapConnectionActor* ConnectionActor = World->SpawnActor<ASnapConnectionActor>(ASnapConnectionActor::StaticClass(), ConnectionTransform);
		ConnectionActor->ConnectionComponent->ConnectionInfo = ConnectionInfo.ConnectionInfo.Get();
		ConnectionActor->ConnectionComponent->ConnectionState = ESnapConnectionState::Door;
		ConnectionActor->ConnectionComponent->DoorType = ConnectionInfo.DoorType;
		ConnectionActor->ConnectionComponent->MarkerPrefix = ConnectionMarkerPrefix;
		if (ConnectionInfo.DoorType == ESnapConnectionDoorType::LockedDoor) {
			ConnectionActor->ConnectionComponent->MarkerName = ConnectionInfo.CustomMarkerName;
		}
		ConnectionActor->BuildConnectionInstance(nullptr, ConnectionInfo.ModuleA, ConnectionInfo.ModuleB,  PersistentLevel);
		for (AActor* SpawnedDoorActor : ConnectionActor->GetSpawnedInstances()) {
			SetupSpawnedActor(SpawnedDoorActor);
		}
		SetupSpawnedActor(ConnectionActor);
	} 

	// Build the walls
	for (const FSnapWallInstance& ConnectionInfo : Model->Walls) {
		FTransform ConnectionTransform = GetWorldToTargetTransform(ConnectionInfo.WorldTransform);
		ASnapConnectionActor* ConnectionActor = World->SpawnActor<ASnapConnectionActor>(ASnapConnectionActor::StaticClass(), ConnectionTransform);
		ConnectionActor->ConnectionComponent->ConnectionInfo = ConnectionInfo.ConnectionInfo.Get();
		ConnectionActor->ConnectionComponent->ConnectionState = ESnapConnectionState::Wall;
		ConnectionActor->ConnectionComponent->DoorType = ESnapConnectionDoorType::NotApplicable;
		ConnectionActor->ConnectionComponent->MarkerPrefix = ConnectionMarkerPrefix;
		ConnectionActor->BuildConnectionInstance(nullptr, ConnectionInfo.ModuleId, {}, PersistentLevel);
		for (AActor* SpawnedDoorActor : ConnectionActor->GetSpawnedInstances()) {
			SetupSpawnedActor(SpawnedDoorActor);
		}
		SetupSpawnedActor(ConnectionActor);
	}
	
	return true;
}

