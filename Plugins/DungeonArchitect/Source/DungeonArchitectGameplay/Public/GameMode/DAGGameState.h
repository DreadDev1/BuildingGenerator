//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameStateBase.h"
#include "DAGGameState.generated.h"

struct FDAG_VerbMessage;

class APlayerState;
class UAbilitySystemComponent;
class UDAG_AbilitySystemComponent;
class UObject;
struct FFrame;

/**
 * ADAG_GameState
 *
 *	The base game state class used by this project.
 */
UCLASS(Config = Game)
class DUNGEONARCHITECTGAMEPLAY_API ADAG_GameState : public AGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ADAG_GameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	// Gets the ability system component used for game wide things
	UFUNCTION(BlueprintCallable, Category = "DAG|GameState")
	UDAG_AbilitySystemComponent* GetDAGAbilitySystemComponent() const { return AbilitySystemComponent; }

	// Send a message that all clients will (probably) get
	// (use only for client notifications like eliminations, server join messages, etc... that can handle being lost)
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "DAG|GameState")
	void MulticastMessageToClients(const FDAG_VerbMessage Message);

	// Send a message that all clients will be guaranteed to get
	// (use only for client notifications that cannot handle being lost)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "DAG|GameState")
	void MulticastReliableMessageToClients(const FDAG_VerbMessage Message);

	// Gets the server's FPS, replicated to clients
	float GetServerFPS() const;

private:
	// The ability system component subobject for game-wide things (primarily gameplay cues)
	UPROPERTY(VisibleAnywhere, Category = "DAG|GameState")
	TObjectPtr<UDAG_AbilitySystemComponent> AbilitySystemComponent;

protected:
	UPROPERTY(Replicated)
	float ServerFPS;
};

