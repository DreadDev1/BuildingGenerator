//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "GameMode/DAGGameState.h"

#include "AbilitySystem/DAGAbilitySystemComponent.h"
#include "Messages/DAGVerbMessage.h"
#include "Messages/MessageSystem/DAGMessageSubsystem.h"
#include "Player/DAGPlayerState.h"
#include "System/DAGLogChannels.h"

#include "Async/TaskGraphInterfaces.h"
#include "Net/UnrealNetwork.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGGameState)

class APlayerState;
class FLifetimeProperty;

extern ENGINE_API float GAverageFPS;


ADAG_GameState::ADAG_GameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UDAG_AbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	ServerFPS = 0.0f;
}

void ADAG_GameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ADAG_GameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(/*Owner=*/ this, /*Avatar=*/ this);
}

UAbilitySystemComponent* ADAG_GameState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ADAG_GameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ADAG_GameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
}

void ADAG_GameState::RemovePlayerState(APlayerState* PlayerState)
{
	//@TODO: This isn't getting called right now (only the 'rich' AGameMode uses it, not AGameModeBase)
	// Need to at least comment the engine code, and possibly move things around
	Super::RemovePlayerState(PlayerState);
}

void ADAG_GameState::SeamlessTravelTransitionCheckpoint(bool bToTransitionMap)
{
	// Remove inactive and bots
	for (int32 i = PlayerArray.Num() - 1; i >= 0; i--)
	{
		APlayerState* PlayerState = PlayerArray[i];
		if (PlayerState && (PlayerState->IsABot() || PlayerState->IsInactive()))
		{
			RemovePlayerState(PlayerState);
		}
	}
}

void ADAG_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ServerFPS);
}

void ADAG_GameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() == ROLE_Authority)
	{
		ServerFPS = GAverageFPS;
	}
}

void ADAG_GameState::MulticastMessageToClients_Implementation(const FDAG_VerbMessage Message)
{
	if (GetNetMode() == NM_Client)
	{
		UDAG_MessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

void ADAG_GameState::MulticastReliableMessageToClients_Implementation(const FDAG_VerbMessage Message)
{
	MulticastMessageToClients_Implementation(Message);
}

float ADAG_GameState::GetServerFPS() const
{
	return ServerFPS;
}

