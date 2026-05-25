//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonUtils.h"

#include "Core/Utils/DungeonModelHelper.h"

#include "Camera/CameraComponent.h"
#include "EdGraph/EdGraphNode.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

const FName FDungeonUtils::GenericDungeonIdTag("Dungeon");

void FDungeonUtils::DestroyAllDungeonOwnedActors(ADungeon* InDungeon) {
	if (!InDungeon) {
		return;
	}
	UWorld* World = InDungeon->GetWorld();
	for (ULevel* Level : World->GetLevels()) {
		DestroyAllDungeonOwnedActors(InDungeon, Level);
	}
}

void FDungeonUtils::DestroyAllDungeonOwnedActors(ADungeon* InDungeon, ULevel* InLevel) {
	if (!InDungeon || !InLevel) {
		return;
	}

	TArray<TObjectPtr<AActor>> ActorsToDestroy;
	const FName DungeonTag = GetDungeonIdTag(InDungeon);
	for (AActor* Actor : InLevel->Actors) {
		if (Actor) {
			if (Actor->ActorHasTag(DungeonTag) || Actor->ActorHasTag(GenericDungeonIdTag)) {
				ActorsToDestroy.Add(Actor);
			}
		}
	}
	
	for (AActor* ActorToDestroy : ActorsToDestroy) {
		if (ActorToDestroy && ActorToDestroy->IsValidLowLevel() && !ActorToDestroy->IsPendingKillPending()) {
			ActorToDestroy->Destroy();
		}
	}
}

FVector FDungeonUtils::GetPlayerViewportLocation(const UWorld* World) {
	if (!World) {
		return FVector::ZeroVector;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController) {
		return FVector::ZeroVector;
	}

	if (PlayerController->PlayerCameraManager) {
		return PlayerController->PlayerCameraManager->GetCameraLocation();
	}

	if (APawn* Pawn = PlayerController->GetPawn()) {
		if (UCameraComponent* CameraComponent = Pawn->FindComponentByClass<UCameraComponent>()) {
			return CameraComponent->GetComponentLocation();
		}

		return Pawn->GetActorLocation();
	}

	// Fallback: Use the player controller's location
	return PlayerController->GetFocalLocation();
}

bool FDungeonUtils::GetSpawnedActorMarkerInfo(const AActor* InSpawnedActor, const UDungeonModel* InDungeonModel, FDungeonMarkerInstance& OutWorldMarker) {
	if (InSpawnedActor && InDungeonModel) {
		if (UDungeonSpawnDataComponent* SpawnData = InSpawnedActor->GetComponentByClass<UDungeonSpawnDataComponent>()) {
			int32 MarkerIndex = SpawnData->MarkerIndex;
			if (InDungeonModel->WorldMarkers.IsValidIndex(MarkerIndex)) {
				OutWorldMarker = InDungeonModel->WorldMarkers[MarkerIndex];
				return true;
			}
		}
	}
	OutWorldMarker = {};
	return false;
}

FName FDungeonUtils::GetDungeonIdTag(const ADungeon* Dungeon) {
	if (Dungeon) {
		const FString TagString = "Dungeon-" + Dungeon->Uid.ToString();
		return FName(*TagString);
	}
	return GenericDungeonIdTag;
    
}

FName FDungeonUtils::GetNodeID(const UEdGraphNode* InNode) {
	return InNode ? FName(*InNode->NodeGuid.ToString()) : NAME_None;
}

bool FDungeonUtils::ActorBelongsToDungeon(const AActor* InActor, const ADungeon* InDungeon) {
	if (!InActor || !InDungeon) {
		return false;
	}

	FName DungeonTag = GetDungeonIdTag(InDungeon);
	return InActor->Tags.Contains(DungeonTag);
}

