//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Character/DAGCharacter.h"

#include "AbilitySystem/DAGAbilitySystemComponent.h"
#include "Character/DAGHealthComponent.h"
#include "DungeonGameplayTags.h"
#include "Player/DAGPlayerController.h"
#include "Player/DAGPlayerState.h"
#include "System/DAGLogChannels.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGCharacter)

class AActor;
class FLifetimeProperty;
class IRepChangedPropertyTracker;
class UInputComponent;

static FName NAME_DAGCharacterCollisionProfile_Capsule(TEXT("DAGPawnCapsule"));
static FName NAME_DAGCharacterCollisionProfile_Mesh(TEXT("DAGPawnMesh"));

ADAG_Character::ADAG_Character(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetNetCullDistanceSquared(900000000.0f);

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	CapsuleComp->SetCollisionProfileName(NAME_DAGCharacterCollisionProfile_Capsule);

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Rotate mesh to be X forward since it is exported as Y forward.
	MeshComp->SetCollisionProfileName(NAME_DAGCharacterCollisionProfile_Mesh);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->GravityScale = 1.0f;
	MoveComp->MaxAcceleration = 2400.0f;
	MoveComp->BrakingFrictionFactor = 1.0f;
	MoveComp->BrakingFriction = 6.0f;
	MoveComp->GroundFriction = 8.0f;
	MoveComp->BrakingDecelerationWalking = 1400.0f;
	MoveComp->bUseControllerDesiredRotation = false;
	MoveComp->bOrientRotationToMovement = false;
	MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	MoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	MoveComp->bCanWalkOffLedgesWhenCrouching = true;
	MoveComp->SetCrouchedHalfHeight(65.0f);

	HealthComponent = CreateDefaultSubobject<UDAG_HealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	BaseEyeHeight = 80.0f;
	CrouchedEyeHeight = 50.0f;
}

void ADAG_Character::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ADAG_Character::Reset()
{
	DisableMovementAndCollision();

	K2_OnReset();

	UninitAndDestroy();
}

void ADAG_Character::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MyTeamID)
}

void ADAG_Character::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();

	Super::NotifyControllerChanged();

	// Update our team ID based on the controller
	if (HasAuthority() && (Controller != nullptr))
	{
		if (IDAG_TeamAgentInterface* ControllerWithTeam = Cast<IDAG_TeamAgentInterface>(Controller))
		{
			MyTeamID = ControllerWithTeam->GetGenericTeamId();
			ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamID);
		}
	}
}

ADAG_PlayerController* ADAG_Character::GetDAGPlayerController() const
{
	return CastChecked<ADAG_PlayerController>(Controller, ECastCheckedType::NullAllowed);
}

ADAG_PlayerState* ADAG_Character::GetDAGPlayerState() const
{
	return CastChecked<ADAG_PlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UDAG_AbilitySystemComponent* ADAG_Character::GetDAGAbilitySystemComponent() const
{
	return Cast<UDAG_AbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* ADAG_Character::GetAbilitySystemComponent() const
{
	if (ADAG_PlayerState* PS = GetPlayerState<ADAG_PlayerState>()) {
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

void ADAG_Character::OnAbilitySystemInitialized()
{
	check(GetDAGAbilitySystemComponent());

	HealthComponent->InitializeWithAbilitySystem(GetDAGAbilitySystemComponent());
}

void ADAG_Character::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void ADAG_Character::InitializeAbilitySystemComponent()
{
	if (ADAG_PlayerState* PS = GetPlayerState<ADAG_PlayerState>()) {
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			ASC->InitAbilityActorInfo(PS, this);
			OnAbilitySystemInitialized();
		}
	}
}

void ADAG_Character::UninitializeAbilitySystemComponent()
{
	if (ADAG_PlayerState* PS = GetPlayerState<ADAG_PlayerState>()) {
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent()) {
			OnAbilitySystemUninitialized();
			ASC->ClearActorInfo();
		}
	}
}
void ADAG_Character::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = MyTeamID;

	Super::PossessedBy(NewController);

	// Initialize the ASC on the server
	InitializeAbilitySystemComponent();
	
	// Grab the current team ID and listen for future changes
	if (IDAG_TeamAgentInterface* ControllerAsTeamProvider = Cast<IDAG_TeamAgentInterface>(NewController))
	{
		MyTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
	}
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ADAG_Character::OnRep_PlayerState() {
	if (PrevPlayerState && PrevPlayerState != GetPlayerState<ADAG_PlayerState>()) {
		UninitializeAbilitySystemComponent();
	}
	
	Super::OnRep_PlayerState();
	
	PrevPlayerState = GetPlayerState<ADAG_PlayerState>();
	
	// Initialize the ASC on the client
	InitializeAbilitySystemComponent();
}

void ADAG_Character::UnPossessed() {
	UninitializeAbilitySystemComponent();

	AController* const OldController = Controller;
	// Stop listening for changes from the old controller
	const FGenericTeamId OldTeamID = MyTeamID;
	if (IDAG_TeamAgentInterface* ControllerAsTeamProvider = Cast<IDAG_TeamAgentInterface>(OldController))
	{
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	Super::UnPossessed();
	
	// Determine what the new team ID should be afterwards
	MyTeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ADAG_Character::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void ADAG_Character::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		ASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool ADAG_Character::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool ADAG_Character::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		return ASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool ADAG_Character::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		return ASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void ADAG_Character::FellOutOfWorld(const class UDamageType& dmgType)
{
	HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
}

void ADAG_Character::OnDeathStarted(AActor*)
{
	DisableMovementAndCollision();
}

void ADAG_Character::OnDeathFinished(AActor*)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}


void ADAG_Character::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
}

void ADAG_Character::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}


void ADAG_Character::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	if (UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		if (ASC->GetAvatarActor() == this)
		{
			UninitializeAbilitySystemComponent();
		}
	}

	SetActorHiddenInGame(true);
}

void ADAG_Character::ToggleCrouch()
{
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (bIsCrouched || MoveComp->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (MoveComp->IsMovingOnGround())
	{
		Crouch();
	}
}

void ADAG_Character::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(DAG_Tags::Status_Crouching, 1);
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void ADAG_Character::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UDAG_AbilitySystemComponent* ASC = GetDAGAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(DAG_Tags::Status_Crouching, 0);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool ADAG_Character::CanJumpInternal_Implementation() const
{
	// same as ACharacter's implementation but without the crouch check
	return JumpIsAllowedInternal();
}


void ADAG_Character::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController() == nullptr)
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamID = MyTeamID;
			MyTeamID = NewTeamID;
			ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
		}
		else
		{
			UE_LOG(LogDAGTeams, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetPathNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogDAGTeams, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
	}
}

FGenericTeamId ADAG_Character::GetGenericTeamId() const
{
	return MyTeamID;
}

FDAG_OnTeamIndexChangedDelegate* ADAG_Character::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}


void ADAG_Character::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId MyOldTeamID = MyTeamID;
	MyTeamID = DAG_IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, MyTeamID);
}

void ADAG_Character::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

