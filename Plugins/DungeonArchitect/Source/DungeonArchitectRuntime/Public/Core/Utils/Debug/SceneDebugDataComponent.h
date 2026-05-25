//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SceneDebugDataComponent.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDASceneDebugDataTextEntry {
	GENERATED_BODY()
	
	UPROPERTY()
	FText Text;
	
	UPROPERTY()
	FVector Location = FVector::ZeroVector;
	
	UPROPERTY()
	FLinearColor Color = FLinearColor::White;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDASceneDebugDataSphereEntry {
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Center = FVector::ZeroVector;
	
	UPROPERTY()
	double Radius = 0;
	
	UPROPERTY()
	FLinearColor Color = FLinearColor::White;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDASceneDebugDataBoxEntry {
	GENERATED_BODY()
	
	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	FVector Extent = FVector::ZeroVector;
	
	UPROPERTY()
	FLinearColor Color = FLinearColor::White;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDASceneDebugData {
	GENERATED_BODY()

	UPROPERTY()
	TArray<FDASceneDebugDataTextEntry> TextEntries;

	UPROPERTY()
	TArray<FDASceneDebugDataSphereEntry> SphereEntries;
	
	UPROPERTY()
	TArray<FDASceneDebugDataBoxEntry> BoxEntries;
};


UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTRUNTIME_API UDASceneDebugDataComponent : public UActorComponent {
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FDASceneDebugData Data;

	UPROPERTY()
	FVector RenderScale = FVector::OneVector;

public:
	void ClearDebugData();
};

