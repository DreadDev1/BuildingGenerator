//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Game/DungeonBuildSystem.h"
#include "DungeonBuildSystemGameMode.generated.h"

class ADungeon;

UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTRUNTIME_API UDungeonBuildSystemGameModeComponent : public UActorComponent {
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	
public:
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	bool ReadyToStartMatch();
	
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	AActor* FindPlayerStart(AController* Player, const FString& IncomingName);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	UClass* GetDefaultPawnClassForController(AController* InController);

	/** Initiate dungeon build. Server starts immediately when ready, clients build in background. */
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	void InitiateDungeonBuild();

	/** Initiate dungeon build and wait for all clients to finish before starting.
	 *  Use this for competitive/synchronized starts (e.g., Valorant, Age of Empires style).
	 *  @param ExpectedPlayerCount - Number of players that must complete loading before match starts
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	void InitiateDungeonBuildAndWaitForPlayers(int32 ExpectedPlayerCount);

public:
	/** If set, finds the dungeon with this tag. Otherwise, auto-finds the first dungeon in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FName MainDungeonTag;
	
	/** Fired when the dungeon is built and ready for players */
	UPROPERTY(BlueprintAssignable, Category="Dungeon Architect")
	FOnDungeonBuildSystemReady OnDungeonReady;
	
private:
	void Initialize();
	AActor* FallbackFindPlayerStart(AController* Player, const FString& IncomingName) const;
	
private:
	UPROPERTY()
	TWeakObjectPtr<ADungeon> Dungeon;
	
	UPROPERTY()
	TWeakObjectPtr<UDungeonBuildSystemDungeonComponent> DungeonBuildSystem;
	
private:
	bool bInitialized{};
};

