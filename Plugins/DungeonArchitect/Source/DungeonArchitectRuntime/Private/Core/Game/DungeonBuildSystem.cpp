//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Game/DungeonBuildSystem.h"

#include "Builders/SnapGridFlow/SnapGridFlowDungeon.h"
#include "Builders/SnapMap/SnapMapDungeonBuilder.h"
#include "Core/Dungeon.h"
#include "Core/Game/DungeonBuildSystemGameMode.h"
#include "Core/Game/DungeonBuildSystemPlayerController.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonBuildSystem, Log, All);

ADungeon* FDungeonBuildSystemUtils::FindDungeonActor(const UWorld* InWorld) {
	TArray<AActor*> DungeonActors;
	UGameplayStatics::GetAllActorsOfClass(InWorld, ADungeon::StaticClass(), DungeonActors);
	for (AActor* DungeonActor : DungeonActors) {
		if (ADungeon* DungeonCandidate = Cast<ADungeon>(DungeonActor)) {
			if (TObjectPtr<UDungeonBuildSystemDungeonComponent> BuildSystem = DungeonCandidate->GetBuildSystemComponent()) {
				if (BuildSystem->bParticipateInBuildSystem) {
					return DungeonCandidate;
				}
			}
		}
	}
	return nullptr;
}

//////////////////////// UDungeonBuildSystemComponent ////////////////////////
UDungeonBuildSystemDungeonComponent::UDungeonBuildSystemDungeonComponent() {
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bWantsInitializeComponent = true;

	SetIsReplicatedByDefault(true);
}

void UDungeonBuildSystemDungeonComponent::InitializeComponent() {
	Super::InitializeComponent();

	if (ADungeon* Dungeon = Cast<ADungeon>(GetOwner())) {
		// Listen to dungeon build events
		Dungeon->OnDungeonBuildComplete.AddDynamic(this, &UDungeonBuildSystemDungeonComponent::OnLocalDungeonBuildComplete);

		// Check if we use level streaming
		if (Dungeon->Builder) {
			bUsesLevelStreaming = Dungeon->Builder->IsA<USnapMapDungeonBuilder>() || Dungeon->Builder->IsA<USnapGridFlowBuilder>();
			if (bUsesLevelStreaming && Dungeon->LevelStreamingModel) {
				Dungeon->LevelStreamingModel->OnInitialChunksLoaded.AddDynamic(this, &UDungeonBuildSystemDungeonComponent::OnLocalDungeonInitialChunksLoaded);
			}
		}
	}
}

void UDungeonBuildSystemDungeonComponent::BeginPlay() {
	Super::BeginPlay();

	OnGameModePostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &UDungeonBuildSystemDungeonComponent::OnGameModePostLogin);
}

void UDungeonBuildSystemDungeonComponent::BuildLocalDungeon(int32 InSeed) {
	Seed = InSeed;
	bIsLocalDungeonReady = false;
	if (ADungeon* Dungeon = Cast<ADungeon>(GetOwner())) {
		if (UDungeonConfig* DungeonConfig = Dungeon->GetConfig()) {
			DungeonConfig->Seed = InSeed;
			Dungeon->BuildDungeon();
		}
	}
}

void UDungeonBuildSystemDungeonComponent::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer) {
	if (GetOwner()->HasAuthority()) {
		if (ServerDungeonState != EDungeonBuildSystemDungeonState::Waiting) {
			if (UDungeonBuildSystemPlayerControllerComponent* PCBuildSystemComponent = NewPlayer->GetComponentByClass<UDungeonBuildSystemPlayerControllerComponent>()) {
				ServerSetClientBuildState(NewPlayer, EDungeonBuildSystemDungeonState::BuildRequested);
				PCBuildSystemComponent->ServerRequestBuildDungeonOnClient(Seed);
			}
			else {
				UE_LOG(LogDungeonBuildSystem, Warning, TEXT("Player Controller does not have a DungeonBuildSystemPlayerController component for the build system to work"));
			}
		}
	}
}

bool UDungeonBuildSystemDungeonComponent::GetClientBuildState(APlayerController* InController, EDungeonBuildSystemDungeonState& OutState) const {
	if (const EDungeonBuildSystemDungeonState* State = ClientDungeonStates.Find(InController)) {
		OutState = *State;
		return true;
	}
	return false;
}

void UDungeonBuildSystemDungeonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void UDungeonBuildSystemDungeonComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void UDungeonBuildSystemDungeonComponent::InitiateBuild() {
	if (!bParticipateInBuildSystem) {
		return;
	}
	
	if (GetOwner()->HasAuthority() && ServerDungeonState != EDungeonBuildSystemDungeonState::BuildRequested) {
		bWaitForAllClients = false;
		ServerBuildDungeon();
	}
}

void UDungeonBuildSystemDungeonComponent::InitiateBuildAndWaitForPlayers(int32 InExpectedPlayerCount) {
	if (!bParticipateInBuildSystem) {
		return;
	}
	
	if (GetOwner()->HasAuthority() && ServerDungeonState != EDungeonBuildSystemDungeonState::BuildRequested) {
		bWaitForAllClients = true;
		ExpectedPlayerCount = InExpectedPlayerCount;
		ServerBuildDungeon();
	}
}

void UDungeonBuildSystemDungeonComponent::ServerBuildDungeon() {
	if (GetOwner()->HasAuthority()) {
		if (bRandomizeSeedOnBuild) {
			Seed = FMath::Rand();
		}
		else {
			ADungeon* Dungeon = Cast<ADungeon>(GetOwner());
			Seed = Dungeon && Dungeon->GetConfig() ? Dungeon->GetConfig()->Seed : 0;

			// Override if passed it in as a URL parameter during server travel
			if (UWorld* World = GetWorld()) {
				if (AGameModeBase* GameModeBase = World->GetAuthGameMode()) {
					Seed = UGameplayStatics::GetIntOption(GameModeBase->OptionsString, "Seed", Seed);
				}
			}
		}
		
		ServerDungeonState = EDungeonBuildSystemDungeonState::BuildRequested;
		
		BuildLocalDungeon(Seed);


		// Set the client states
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It) {
			if (APlayerController* PlayerController = *It) {
				if (PlayerController->GetLocalRole() == ROLE_Authority) {
					continue;
				}
				ServerSetClientBuildState(PlayerController, EDungeonBuildSystemDungeonState::BuildRequested);
			}
		}
		
		MulticastBuildDungeon(Seed);
	}
}

void UDungeonBuildSystemDungeonComponent::ClientBuildDungeon(int32 InSeed) {
	if (GetOwner()->HasAuthority()) {
		// We don't want to process servers here, they're handled in ServerBuildDungeon
		return;
	}
	
	
	BuildLocalDungeon(InSeed);
}

void UDungeonBuildSystemDungeonComponent::ServerSetClientBuildState(APlayerController* PC, EDungeonBuildSystemDungeonState InClientBuildState) {
	check (GetOwner()->HasAuthority());
	
	EDungeonBuildSystemDungeonState& BuildStateRef = ClientDungeonStates.FindOrAdd(PC);
	BuildStateRef = InClientBuildState;

	if (InClientBuildState == EDungeonBuildSystemDungeonState::BuildComplete) {
		if (bHasNotifiedReady) {
			// Game has started. restart the player
			RestartController(PC);
		}
	}
}

void UDungeonBuildSystemDungeonComponent::RestartController(AController* PC) const {
	if (!PC) {
		return;
	}
	
	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode()) {
		if (APawn* Pawn = PC->GetPawn()) {
			PC->UnPossess();
			Pawn->Destroy();

			GameMode->RestartPlayer(PC);
		}
	}
}

bool UDungeonBuildSystemDungeonComponent::ServerNotifyClientDungeonBuildComplete_Validate(APlayerController* PC, int32 InSeed) {
	return true;
}

void UDungeonBuildSystemDungeonComponent::ServerNotifyClientDungeonBuildComplete_Implementation(APlayerController* PC, int32 InSeed) {
	check(GetOwner()->HasAuthority());

	if (InSeed == Seed) {
		ServerSetClientBuildState(PC, EDungeonBuildSystemDungeonState::BuildComplete);
		CheckAndNotifyReadyToStart();
	}
}

void UDungeonBuildSystemDungeonComponent::MulticastBuildDungeon_Implementation(int32 InSeed) {
	if (GetOwner()->HasAuthority() ) {
		// We don't want to build this on the server. It's already handled from another path
		return;
	}

	ClientBuildDungeon(InSeed);
}

bool UDungeonBuildSystemDungeonComponent::CanStartMatch_Implementation() const {
	if (!IsServerDungeonBuilt()) return false;
	return bWaitForAllClients ? AreAllClientDungeonsBuilt() : true;
}

bool UDungeonBuildSystemDungeonComponent::IsServerDungeonBuilt() const {
	return ServerDungeonState == EDungeonBuildSystemDungeonState::BuildComplete;
}

bool UDungeonBuildSystemDungeonComponent::AreAllClientDungeonsBuilt() const {
	const int32 NumPlayersConnected = ServerGetLoggedInPlayersCount();
	if (NumPlayersConnected < ExpectedPlayerCount) {
		// We need at least ExpectedPlayerCount players to login before we can begin
		return false;
	}
	auto ClientStateCopy = ClientDungeonStates;
	for (auto& Entry : ClientStateCopy) {
		TWeakObjectPtr<APlayerController> PlayerController = Entry.Key;
		EDungeonBuildSystemDungeonState BuildState = Entry.Value;
		if (PlayerController.IsValid() && BuildState != EDungeonBuildSystemDungeonState::BuildComplete) {
			return false;
		}
	}
	return true;
}

int32 UDungeonBuildSystemDungeonComponent::ServerGetLoggedInPlayersCount() const {
	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
		APlayerController* PC = Iterator->Get();
		if (PC && PC->PlayerState) {
			++PlayerCount;
		}
	}

	return PlayerCount;
}

void UDungeonBuildSystemDungeonComponent::CheckAndNotifyReadyToStart() {
	bool bHasAuthority = GetOwner()->HasAuthority();
	
	if (CanStartMatch() && !bHasNotifiedReady) {
		bHasNotifiedReady = true;

		if (ADungeon* Dungeon = Cast<ADungeon>(GetOwner())) {
			if (OnDungeonReady.IsBound()) {
				OnDungeonReady.Broadcast(Dungeon);
			}
			
			// Find GameMode component and notify
			if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode()) {
				if (UDungeonBuildSystemGameModeComponent* GameModeComp = GameMode->FindComponentByClass<UDungeonBuildSystemGameModeComponent>()) {
					if (GameModeComp->OnDungeonReady.IsBound()) {
						GameModeComp->OnDungeonReady.Broadcast(Dungeon);
					}
				}
			}
		}

		// Restart all the player controllers on the server
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It) {
			if (APlayerController* PC = *It) {
				bool bRestart = PC->HasAuthority();
				if (!bRestart) {
					if (EDungeonBuildSystemDungeonState* BuildState = ClientDungeonStates.Find(PC)) {
						if (*BuildState == EDungeonBuildSystemDungeonState::BuildComplete) {
							bRestart = true;
						}
					}
				}
				if (bRestart) {
					RestartController(PC);
				}
			}
		}
	}
}

void UDungeonBuildSystemDungeonComponent::OnLocalDungeonInitialChunksLoaded(ADungeon* InDungeon) {
	ADungeon* Dungeon = Cast<ADungeon>(GetOwner());
	if (InDungeon == Dungeon) {
		if (bUsesLevelStreaming) {
			HandleLocalDungeonReady();
		}
	}
}

void UDungeonBuildSystemDungeonComponent::OnLocalDungeonBuildComplete(ADungeon* InDungeon, bool bSuccess) {
	ADungeon* Dungeon = Cast<ADungeon>(GetOwner());
	if (InDungeon == Dungeon) {
		if (!bUsesLevelStreaming) {
			HandleLocalDungeonReady();
		}
	}
}

void UDungeonBuildSystemDungeonComponent::HandleLocalDungeonReady() {
	bIsLocalDungeonReady = true;

	if (GetOwner()->HasAuthority()) {
		ServerDungeonState = EDungeonBuildSystemDungeonState::BuildComplete;
		CheckAndNotifyReadyToStart();
	}
	else {
		const APlayerController* PC =  UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (UDungeonBuildSystemPlayerControllerComponent* PCBuildSystemComponent = PC->GetComponentByClass<UDungeonBuildSystemPlayerControllerComponent>()) {
			PCBuildSystemComponent->ServerNotifyClientDungeonBuildComplete(this, Seed);
		}
	}
}


