//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Teams/DAGTeamAgentInterface.h"

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "DAGCharacter.generated.h"

class AActor;
class AController;
class FLifetimeProperty;
class IRepChangedPropertyTracker;
class UAbilitySystemComponent;
class UInputComponent;
class UDAG_AbilitySystemComponent;
class UDAG_HealthComponent;
class UObject;
struct FFrame;
struct FGameplayTag;
struct FGameplayTagContainer;


/**
 * ADAG_Character
 *
 *	The base character pawn class used by this project.
 *	Responsible for sending events to pawn components.
 *	New behavior should be added via pawn components when possible.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class DUNGEONARCHITECTGAMEPLAY_API ADAG_Character
	: public ACharacter
	, public IAbilitySystemInterface
	, public IGameplayCueInterface
	, public IGameplayTagAssetInterface
	, public IDAG_TeamAgentInterface
{
	GENERATED_BODY()

public:

	ADAG_Character(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "DAG|Character")
	ADAG_PlayerController* GetDAGPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "DAG|Character")
	ADAG_PlayerState* GetDAGPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "DAG|Character")
	UDAG_AbilitySystemComponent* GetDAGAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ToggleCrouch();

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	virtual void OnRep_PlayerState() override;
	//~End of APawn interface

	//~IDAG_TeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FDAG_OnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IDAG_TeamAgentInterface interface

	
protected:

	void InitializeAbilitySystemComponent();
	void UninitializeAbilitySystemComponent();
	
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;

	virtual void  FellOutOfWorld(const class UDamageType& dmgType) override;

	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();

	// Called when the death sequence for the character has completed
	UFUNCTION(BlueprintImplementableEvent, Category = "DAG|Character", meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual bool CanJumpInternal_Implementation() const;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DAG|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDAG_HealthComponent> HealthComponent;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY()
	FDAG_OnTeamIndexChangedDelegate OnTeamChangedDelegate;

	// Track previous player state for ability system cleanup
	UPROPERTY()
	TObjectPtr<ADAG_PlayerState> PrevPlayerState;

protected:
	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

private:
	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);
};

