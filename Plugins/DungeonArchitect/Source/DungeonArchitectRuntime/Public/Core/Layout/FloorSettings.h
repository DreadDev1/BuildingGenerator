//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "FloorSettings.generated.h"

class UDungeonModel;
class UDungeonConfig;

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonFloorSettings {
	GENERATED_BODY()

	UPROPERTY()
	float FloorHeight = 400;        // Height of a floor

	UPROPERTY()
	float FloorCaptureHeight = 350; // This should not account for the ceiling mesh, or it may overlap with the floor above

	UPROPERTY()
	float GlobalHeightOffset = 0;   // The offset to apply to the dungeon height
	
	UPROPERTY()
	int32 FloorLowestIndex = 0;     // Minimum Floor Index
	
	UPROPERTY()
	int32 FloorHighestIndex = 0;    // Maximum Floor Index
};


