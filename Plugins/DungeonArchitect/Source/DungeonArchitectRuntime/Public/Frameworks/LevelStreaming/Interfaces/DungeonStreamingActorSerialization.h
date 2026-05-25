//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "DungeonStreamingActorSerialization.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonStreamingActorDataEntry {
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSubclassOf<AActor> ActorClass;

	UPROPERTY()
	FTransform ActorTransform;

	UPROPERTY()
	FString ActorName;

	UPROPERTY()
	TArray<uint8> ActorData;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonStreamingActorData : public UObject {
	GENERATED_BODY()
public:

	UPROPERTY()
	TArray<FDungeonStreamingActorDataEntry> ActorEntries;

public:
	void SaveLevel(ULevel* InLevel);
	void LoadLevel(ULevel* InLevel);

private:
	void SaveActor(AActor* InActor, FDungeonStreamingActorDataEntry& OutEntry);
	void LoadActor(AActor* InActor, const FDungeonStreamingActorDataEntry& InEntry);

};

UINTERFACE(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UDungeonStreamingActorSerialization : public UInterface {
	GENERATED_BODY()

};

class DUNGEONARCHITECTRUNTIME_API IDungeonStreamingActorSerialization {
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dungeon")
	void OnSerializedDataLoaded();

};

