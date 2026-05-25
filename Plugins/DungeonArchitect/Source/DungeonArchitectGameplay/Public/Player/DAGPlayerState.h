//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "System/DAGGameplayTagStack.h"
#include "Teams/DAGTeamAgentInterface.h"

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "DAGPlayerState.generated.h"

struct FDAG_VerbMessage;

class AController;
class ADAG_PlayerController;
class APlayerState;
class FName;
class UAbilitySystemComponent;
class UDAG_AbilitySystemComponent;
class UObject;
struct FFrame;
struct FGameplayTag;

/** Defines the types of client connected */
UENUM()
enum class EDAG_PlayerConnectionType : uint8
{
	// An active player
	Player = 0,

	// Spectator connected to a running game
	LiveSpectator,

	// Spectating a demo recording offline
	ReplaySpectator,

	// A deactivated player (disconnected)
	InactivePlayer
};

/**
 * ADAG_PlayerState
 *
 *	Base player state class used by this project.
 */
UCLASS(Config = Game)
class DUNGEONARCHITECTGAMEPLAY_API ADAG_PlayerState : public APlayerState, public IAbilitySystemInterface, public IDAG_TeamAgentInterface
{
	GENERATED_BODY()

public:
	ADAG_PlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "DAG|PlayerState")
	ADAG_PlayerController* GetDAGPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "DAG|PlayerState")
	UDAG_AbilitySystemComponent* GetDAGAbilitySystemComponent() const { return AbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//~APlayerState interface
	virtual void Reset() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

	//~IDAG_TeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FDAG_OnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IDAG_TeamAgentInterface interface

	static const FName NAME_DAGAbilityReady;

	void SetPlayerConnectionType(EDAG_PlayerConnectionType NewType);
	EDAG_PlayerConnectionType GetPlayerConnectionType() const { return MyPlayerConnectionType; }

	/** Returns the Squad ID of the squad the player belongs to. */
	UFUNCTION(BlueprintCallable, Category = "DAG|PlayerState")
	int32 GetSquadId() const
	{
		return MySquadID;
	}

	/** Returns the Team ID of the team the player belongs to. */
	UFUNCTION(BlueprintCallable, Category = "DAG|PlayerState")
	int32 GetTeamId() const
	{
		return DAG_GenericTeamIdToInteger(MyTeamID);
	}

	void SetSquadID(int32 NewSquadID);

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool HasStatTag(FGameplayTag Tag) const;

	// Send a message to just this player
	// (use only for client notifications like accolades, quest toasts, etc... that can handle being occasionally lost)
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "DAG|PlayerState")
	void ClientBroadcastMessage(const FDAG_VerbMessage Message);

	// Gets the replicated view rotation of this player, used for spectating
	FRotator GetReplicatedViewRotation() const;

	// Sets the replicated view rotation, only valid on the server
	void SetReplicatedViewRotation(const FRotator& NewRotation);

private:

	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "DAG|PlayerState")
	TObjectPtr<UDAG_AbilitySystemComponent> AbilitySystemComponent;

	// Health attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UDAG_HealthAttributeSet> HealthSet;
	// Combat attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UDAG_CombatAttributeSet> CombatSet;

	UPROPERTY(Replicated)
	EDAG_PlayerConnectionType MyPlayerConnectionType;

	UPROPERTY()
	FDAG_OnTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(ReplicatedUsing=OnRep_MySquadID)
	int32 MySquadID;

	UPROPERTY(Replicated)
	FDAG_GameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	FRotator ReplicatedViewRotation;

private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnRep_MySquadID();
};

