//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Game/DungeonBuildSystemGameMode.h"

#include "Core/Dungeon.h"
#include "Core/Game/DungeonBuildSystem.h"

#include "Engine/PlayerStartPIE.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"

////////////////////////// UGameModeDungeonBuildSystemComponent //////////////////////////

void UDungeonBuildSystemGameModeComponent::BeginPlay() {
	Super::BeginPlay();

	if (!bInitialized) {
		Initialize();
	}
}

void UDungeonBuildSystemGameModeComponent::InitiateDungeonBuild() {
	if (!bInitialized) {
		Initialize();
	}
	if (DungeonBuildSystem.IsValid()) {
		DungeonBuildSystem->InitiateBuild();
	}
}

void UDungeonBuildSystemGameModeComponent::InitiateDungeonBuildAndWaitForPlayers(int32 ExpectedPlayerCount) {
	if (!bInitialized) {
		Initialize();
	}
	if (DungeonBuildSystem.IsValid()) {
		DungeonBuildSystem->InitiateBuildAndWaitForPlayers(ExpectedPlayerCount);
	}
}

void UDungeonBuildSystemGameModeComponent::Initialize() {
	bInitialized = true;
	Dungeon.Reset();
	DungeonBuildSystem.Reset();
	if (!MainDungeonTag.IsNone()) {
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), MainDungeonTag, Found);
		for (AActor* Actor : Found) {
			if (ADungeon* FoundDungeon = Cast<ADungeon>(Actor)) {
				if (TObjectPtr<UDungeonBuildSystemDungeonComponent> BuildSystem = FoundDungeon->GetBuildSystemComponent()) {
					if (BuildSystem->bParticipateInBuildSystem) {
						Dungeon = FoundDungeon;
					}
				}
				break;
			}
		}
	}

	if (!Dungeon.IsValid()) {
		Dungeon = FDungeonBuildSystemUtils::FindDungeonActor(GetWorld());
	}
	
	if (Dungeon.IsValid()) {
		DungeonBuildSystem = Dungeon->GetBuildSystemComponent();
	}
}

bool UDungeonBuildSystemGameModeComponent::ReadyToStartMatch() {
	if (!DungeonBuildSystem.IsValid()) {
		return false;
	}
	if (!bInitialized) {
		Initialize();
	}
	return DungeonBuildSystem->CanStartMatch();
}

AActor* UDungeonBuildSystemGameModeComponent::FindPlayerStart(AController* Player, const FString& IncomingName) {
	if (!bInitialized) {
		Initialize();
	}
	
	if (DungeonBuildSystem.IsValid()) {
		if (DungeonBuildSystem->CanStartMatch()) {
			TArray<AActor*> PlayerStartActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStartActors);
			for (AActor* PlayerStartActor : PlayerStartActors) {
				if (!PlayerStartActor) {
					continue;
				}

				if (PlayerStartActor->IsA<APlayerStartPIE>()) {
					// We don't want to start in PIE locations
					continue;
				}

				// TODO: Handle tagging so we can selectively spawn at different locations
				return PlayerStartActor;
			} 
		}
	}

	return FallbackFindPlayerStart(Player, IncomingName);
}

UClass* UDungeonBuildSystemGameModeComponent::GetDefaultPawnClassForController(AController* InController) {
	if (AGameModeBase* GameMode = Cast<AGameModeBase>(GetOwner())) {
		if (ReadyToStartMatch()) {
			// Check if the player controller is ready
			EDungeonBuildSystemDungeonState ClientBuildState;
			bool bStateTracked = DungeonBuildSystem->GetClientBuildState(Cast<APlayerController>(InController), ClientBuildState);
			if (!bStateTracked || ClientBuildState == EDungeonBuildSystemDungeonState::BuildComplete) {
				return GameMode->DefaultPawnClass;
			}
		}
		return GameMode->SpectatorClass;
	}

	// Component is attached to the wrong component.   return a default pawn
	return ADefaultPawn::StaticClass();
}

AActor* UDungeonBuildSystemGameModeComponent::FallbackFindPlayerStart(AController* Player, const FString& IncomingName) const {
	UWorld* World = GetWorld();

	// If incoming start is specified, then just use it
	if (!IncomingName.IsEmpty())
	{
		const FName IncomingPlayerStartTag = FName(*IncomingName);
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			APlayerStart* Start = *It;
			if (Start && Start->PlayerStartTag == IncomingPlayerStartTag)
			{
				return Start;
			}
		}
	}

	// Always pick StartSpot at start of match
	if (Player != nullptr && Player->StartSpot != nullptr)
	{
		if (AActor* PlayerStartSpot = Player->StartSpot.Get())
		{
			return PlayerStartSpot;
		}
		else
		{
			UE_LOG(LogGameMode, Error, TEXT("FindPlayerStart: ShouldSpawnAtStartSpot returned true but the Player StartSpot was null."));
		}
	}

	AActor* BestStart{};
	if (AGameModeBase* OwningGameModeBase = Cast<AGameModeBase>(GetOwner())) {
		BestStart = OwningGameModeBase->ChoosePlayerStart(Player);
		if (!BestStart) {
			BestStart = World->GetWorldSettings();
		}
	}
	return BestStart;
}
