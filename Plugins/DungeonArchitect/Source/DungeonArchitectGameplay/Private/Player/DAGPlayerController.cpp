//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Player/DAGPlayerController.h"

#include "AbilitySystem/DAGAbilitySystemComponent.h"
#include "DungeonGameplayTags.h"
#include "Player/DAGCheatManager.h"
#include "Player/DAGLocalPlayer.h"
#include "Player/DAGPlayerState.h"
#include "System/DAGLogChannels.h"

#include "AbilitySystemGlobals.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameMapsSettings.h"
#include "Net/UnrealNetwork.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGPlayerController)

namespace DAG
{
	namespace Input
	{
		static int32 ShouldAlwaysPlayForceFeedback = 0;
		static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(TEXT("DAGPC.ShouldAlwaysPlayForceFeedback"),
			ShouldAlwaysPlayForceFeedback,
			TEXT("Should force feedback effects be played, even if the last input device was not a gamepad?"));
	}
}

ADAG_PlayerController::ADAG_PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ADAG_PlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ADAG_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(false);
}

void ADAG_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ADAG_PlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Disable replicating the PC target view as it doesn't work well for replays or client-side spectating.
	// The engine TargetViewRotation is only set in APlayerController::TickActor if the server knows ahead of time that 
	// a specific pawn is being spectated and it only replicates down for COND_OwnerOnly.
	// In client-saved replays, COND_OwnerOnly is never true and the target pawn is not always known at the time of recording.
	// To support client-saved replays, the replication of this was moved to ReplicatedViewRotation and updated in PlayerTick.
	DISABLE_REPLICATED_PROPERTY(APlayerController, TargetViewRotation);
}

void ADAG_PlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void ADAG_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// If we are auto running then add some player input
	if (GetIsAutoRunning())
	{
		if (APawn* CurrentPawn = GetPawn())
		{
			const FRotator MovementRotation(0.0f, GetControlRotation().Yaw, 0.0f);
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			CurrentPawn->AddMovementInput(MovementDirection, 1.0f);	
		}
	}

	ADAG_PlayerState* DAGPlayerState = GetDAGPlayerState();

	if (PlayerCameraManager && DAGPlayerState)
	{
		APawn* TargetPawn = PlayerCameraManager->GetViewTargetPawn();

		if (TargetPawn)
		{
			// Update view rotation on the server so it replicates
			if (HasAuthority() || TargetPawn->IsLocallyControlled())
			{
				DAGPlayerState->SetReplicatedViewRotation(TargetPawn->GetViewRotation());
			}

			// Update the target view rotation if the pawn isn't locally controlled
			if (!TargetPawn->IsLocallyControlled())
			{
				DAGPlayerState = TargetPawn->GetPlayerState<ADAG_PlayerState>();
				if (DAGPlayerState)
				{
					// Get it from the spectated pawn's player state, which may not be the same as the PC's playerstate
					TargetViewRotation = DAGPlayerState->GetReplicatedViewRotation();
				}
			}
		}
	}
}

ADAG_PlayerState* ADAG_PlayerController::GetDAGPlayerState() const
{
	return CastChecked<ADAG_PlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UDAG_AbilitySystemComponent* ADAG_PlayerController::GetDAGAbilitySystemComponent() const
{
	const ADAG_PlayerState* DAGPS = GetDAGPlayerState();
	return (DAGPS ? DAGPS->GetDAGAbilitySystemComponent() : nullptr);
}

void ADAG_PlayerController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, DAG_IntegerToGenericTeamId(OldTeam), DAG_IntegerToGenericTeamId(NewTeam));
}

void ADAG_PlayerController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void ADAG_PlayerController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IDAG_TeamAgentInterface* PlayerStateTeamInterface = Cast<IDAG_TeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IDAG_TeamAgentInterface* PlayerStateTeamInterface = Cast<IDAG_TeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}

void ADAG_PlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ADAG_PlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ADAG_PlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void ADAG_PlayerController::AddCheats(bool bForce)
{
	Super::AddCheats(bForce);
#if DAG_USING_CHEAT_MANAGER
	Super::AddCheats(true);
#else //#if DAG_USING_CHEAT_MANAGER
	Super::AddCheats(bForce);
#endif // #else //#if DAG_USING_CHEAT_MANAGER
}

void ADAG_PlayerController::ServerCheat_Implementation(const FString& Msg)
{
#if DAG_USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogDAG, Warning, TEXT("ServerCheat: %s"), *Msg);
		ClientMessage(ConsoleCommand(Msg));
	}
#endif // #if DAG_USING_CHEAT_MANAGER
}

bool ADAG_PlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void ADAG_PlayerController::ServerCheatAll_Implementation(const FString& Msg)
{
#if DAG_USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogDAG, Warning, TEXT("ServerCheatAll: %s"), *Msg);
		for (TActorIterator<ADAG_PlayerController> It(GetWorld()); It; ++It)
		{
			ADAG_PlayerController* DAGPC = (*It);
			if (DAGPC)
			{
				DAGPC->ClientMessage(DAGPC->ConsoleCommand(Msg));
			}
		}
	}
#endif // #if DAG_USING_CHEAT_MANAGER
}

bool ADAG_PlayerController::ServerCheatAll_Validate(const FString& Msg)
{
	return true;
}

void ADAG_PlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void ADAG_PlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void ADAG_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SetIsAutoRunning(false);
}

void ADAG_PlayerController::SetIsAutoRunning(const bool bEnabled)
{
	const bool bIsAutoRunning = GetIsAutoRunning();
	if (bEnabled != bIsAutoRunning)
	{
		if (!bEnabled)
		{
			OnEndAutoRun();
		}
		else
		{
			OnStartAutoRun();
		}
	}
}

bool ADAG_PlayerController::GetIsAutoRunning() const
{
	bool bIsAutoRunning = false;
	if (const UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		bIsAutoRunning = ASC->GetTagCount(DAG_Tags::Status_AutoRunning) > 0;
	}
	return bIsAutoRunning;
}

void ADAG_PlayerController::OnStartAutoRun()
{
	if (UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(DAG_Tags::Status_AutoRunning, 1);
		K2_OnStartAutoRun();
	}	
}

void ADAG_PlayerController::OnEndAutoRun()
{
	if (UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(DAG_Tags::Status_AutoRunning, 0);
		K2_OnEndAutoRun();
	}
}

void ADAG_PlayerController::UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId)
{
	if (bForceFeedbackEnabled)
	{
		if (const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
		{
			const ECommonInputType CurrentInputType = CommonInputSubsystem->GetCurrentInputType();
			if (DAG::Input::ShouldAlwaysPlayForceFeedback || CurrentInputType == ECommonInputType::Gamepad || CurrentInputType == ECommonInputType::Touch)
			{
				InputInterface->SetForceFeedbackChannelValues(ControllerId, ForceFeedbackValues);
				return;
			}
		}
	}
	
	InputInterface->SetForceFeedbackChannelValues(ControllerId, FForceFeedbackValues());
}

void ADAG_PlayerController::UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents)
{
	Super::UpdateHiddenComponents(ViewLocation, OutHiddenComponents);

	if (bHideViewTargetPawnNextFrame)
	{
		AActor* const ViewTargetPawn = PlayerCameraManager ? Cast<AActor>(PlayerCameraManager->GetViewTarget()) : nullptr;
		if (ViewTargetPawn)
		{
			// internal helper func to hide all the components
			auto AddToHiddenComponents = [&OutHiddenComponents](const TInlineComponentArray<UPrimitiveComponent*>& InComponents)
			{
				// add every component and all attached children
				for (UPrimitiveComponent* Comp : InComponents)
				{
					if (Comp->IsRegistered())
					{
						OutHiddenComponents.Add(Comp->GetPrimitiveSceneId());

						for (USceneComponent* AttachedChild : Comp->GetAttachChildren())
						{
							static FName NAME_NoParentAutoHide(TEXT("NoParentAutoHide"));
							UPrimitiveComponent* AttachChildPC = Cast<UPrimitiveComponent>(AttachedChild);
							if (AttachChildPC && AttachChildPC->IsRegistered() && !AttachChildPC->ComponentTags.Contains(NAME_NoParentAutoHide))
							{
								OutHiddenComponents.Add(AttachChildPC->GetPrimitiveSceneId());
							}
						}
					}
				}
			};

			//TODO Solve with an interface.  Gather hidden components or something.
			//TODO Hiding isn't awesome, sometimes you want the effect of a fade out over a proximity, needs to bubble up to designers.

			// hide pawn's components
			TInlineComponentArray<UPrimitiveComponent*> PawnComponents;
			ViewTargetPawn->GetComponents(PawnComponents);
			AddToHiddenComponents(PawnComponents);

			//// hide weapon too
			//if (ViewTargetPawn->CurrentWeapon)
			//{
			//	TInlineComponentArray<UPrimitiveComponent*> WeaponComponents;
			//	ViewTargetPawn->CurrentWeapon->GetComponents(WeaponComponents);
			//	AddToHiddenComponents(WeaponComponents);
			//}
		}

		// we consumed it, reset for next frame
		bHideViewTargetPawnNextFrame = false;
	}
}

void ADAG_PlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogDAGTeams, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId ADAG_PlayerController::GetGenericTeamId() const
{
	if (const IDAG_TeamAgentInterface* PSWithTeamInterface = Cast<IDAG_TeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FDAG_OnTeamIndexChangedDelegate* ADAG_PlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ADAG_PlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

