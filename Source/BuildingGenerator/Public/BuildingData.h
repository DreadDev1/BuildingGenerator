#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoomPlacement.h"
#include "BuildingData.generated.h"

// Per-floor configuration stored in UBuildingData.
// BuildingManager iterates this array and spawns one AFloorManager per entry,
// positioning each at the correct Z based on the cumulative height of floors below it.
USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FFloorDefinition
{
	GENERATED_BODY()

	// Height of every room on this floor in cm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor", meta = (ClampMin = "100"))
	int32 RoomHeightCm = 300;

	// Bounding grid for this floor in cells. Validation rejects any room whose
	// footprint extends beyond these bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor", meta = (ClampMin = "1"))
	FIntPoint FloorGridSize = FIntPoint(20, 20);

	// All rooms on this floor. Positions (GridOrigin) are in cell units
	// relative to this floor's local grid origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor")
	TArray<FRoomPlacement> RoomPlacements;
};

// Designer-authored building configuration. One asset = one building type.
// Assign to ABuildingManager::BuildingDataAsset and click GenerateBuilding.
UCLASS(BlueprintType)
class BUILDINGGENERATOR_API UBuildingData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingData|Floors")
	TArray<FFloorDefinition> Floors;
};
