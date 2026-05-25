//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "DungeonBuildSystem.generated.h"

class ADungeon;
class AGameModeBase;

UENUM(BlueprintType)
enum class EDungeonBuildSystemDungeonState : uint8 {
	Waiting,
	BuildRequested,
	BuildComplete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonBuildSystemReady, ADungeon*, Dungeon);

class FDungeonBuildSystemUtils {
public:
	static ADungeon* FindDungeonActor(const UWorld* InWorld);
};

UCLASS(Blueprintable, BlueprintType, hidecategories = (Rendering,Input,Actor,Misc,Replication,Collision,LOD,Cooking,HLOD,Physics,Networking,Tags,Activation,AssetUserData,Navigation), meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTRUNTIME_API UDungeonBuildSystemDungeonComponent : public UActorComponent {
	GENERATED_BODY()
public:
	UDungeonBuildSystemDungeonComponent();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent , Category="Dungeon")
	bool CanStartMatch() const;

	/** Initiate dungeon build. Server starts immediately when ready, clients build in background. */
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void InitiateBuild();

	/** Initiate dungeon build and wait for all clients to finish before starting.
	 *  Use this for competitive/synchronized starts (e.g., Valorant, Age of Empires style).
	 *  @param ExpectedPlayerCount - Number of players that must complete loading before match starts
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void InitiateBuildAndWaitForPlayers(int32 ExpectedPlayerCount);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerNotifyClientDungeonBuildComplete(APlayerController* PC, int32 InSeed);
	
	void ClientBuildDungeon(int32 Seed);
	bool GetClientBuildState(APlayerController* InController, EDungeonBuildSystemDungeonState& OutState) const;

private:
	/** Manually invoke the build system on the server */
	void ServerBuildDungeon();
	void ServerSetClientBuildState(APlayerController* PC, EDungeonBuildSystemDungeonState InClientBuildState);
	void RestartController(AController* PC) const;
	
	/** Multicast function to build the dungeon on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastBuildDungeon(int32 InSeed);

	bool IsServerDungeonBuilt() const;
	bool AreAllClientDungeonsBuilt() const;
	int32 ServerGetLoggedInPlayersCount() const;
	void CheckAndNotifyReadyToStart();
	
	UFUNCTION()
	void OnLocalDungeonInitialChunksLoaded(ADungeon* InDungeon);
	
	UFUNCTION()
	void OnLocalDungeonBuildComplete(ADungeon* InDungeon, bool bSuccess);

	void HandleLocalDungeonReady();
	void BuildLocalDungeon(int32 Seed);
	
	virtual void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Build System")
	bool bParticipateInBuildSystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Build System", meta=(EditCondition="bParticipateInBuildSystem"))
	bool bRandomizeSeedOnBuild = false;

	/** Fired when the dungeon is built and ready for players */
	UPROPERTY(BlueprintAssignable, Category="Dungeon Architect")
	FOnDungeonBuildSystemReady OnDungeonReady;

private:
	UPROPERTY(Replicated, Transient)
	EDungeonBuildSystemDungeonState ServerDungeonState = EDungeonBuildSystemDungeonState::Waiting;

	bool bUsesLevelStreaming{};
	bool bIsLocalDungeonReady{};

	bool bHasNotifiedReady{};
	int32 Seed{};
	FDelegateHandle OnGameModePostLoginHandle;

	/** If true, wait for all clients to finish building before starting match */
	bool bWaitForAllClients = false;

	/** Number of players expected to complete loading (set by InitiateBuildAndWaitForPlayers) */
	int32 ExpectedPlayerCount = 0;

	TMap<TWeakObjectPtr<APlayerController>, EDungeonBuildSystemDungeonState> ClientDungeonStates;
};

