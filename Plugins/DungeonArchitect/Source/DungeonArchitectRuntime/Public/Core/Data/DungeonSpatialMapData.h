//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonSpatialMapData.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonSpatialMapPoint {
	GENERATED_BODY()

	UPROPERTY()
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY()
	float Value = 0;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonSpatialMapData : public UObject {
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FDungeonSpatialMapPoint> Points;

	UPROPERTY()
	FTransform WorldTransform;

	UPROPERTY()
	FBox LocalBounds = FBox(ForceInit);
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonSpatialSurfaceMapData : public UDungeonSpatialMapData {
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Width = 0;
	
	UPROPERTY()
	int32 Height = 0;

	UPROPERTY()
	FVector SamplingResolution = FVector::One();
};


