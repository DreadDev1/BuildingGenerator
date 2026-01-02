// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Generation/RoomGenerationTypes.h"
#include "Engine/DataAsset.h"
#include "RoomData.generated.h"

struct FForcedEmptyRegion;
class UFloorData;
class UWallData;
class UDoorData;
class UCeilingData;


UCLASS()
class BUILDINGGENERATOR_API URoomData : public UDataAsset
{
	GENERATED_BODY()

public:
	
#pragma region Floor Style Data
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Styles")
	TSoftObjectPtr<UFloorData> FloorStyleData;

	UPROPERTY(EditAnywhere, Category = "Designer Overrides|Floor")
	TArray<FIntPoint> ForcedEmptyFloorCells;
	
	// Array of rectangular regions the designer wants to force empty
	// Use this to create L-shapes, T-shapes, courtyards, or irregular room layouts
	UPROPERTY(EditAnywhere, Category = "Designer Overrides|Floor")
	TArray<FForcedEmptyRegion> ForcedEmptyRegions;
	
	// Map of specific meshes to force-place at coordinates (Highest Priority)
	// Key = Grid coordinate (top-left cell), Value = Mesh to place
	UPROPERTY(EditAnywhere, Category = "Designer Overrides|Floor")
	TMap<FIntPoint, FMeshPlacementInfo> ForcedFloorPlacements;
#pragma endregion
	
#pragma region Wall Style Data
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Styles")
	TSoftObjectPtr<UWallData> WallStyleData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Walls")
	TArray<FForcedWallPlacement> ForcedWallPlacements;
	
	// Wall thickness for rooms using this data (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Properties")
	float WallThickness = 25.0f;
#pragma endregion
	
#pragma region Door Style Data
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Styles")
	TSoftObjectPtr<UDoorData> DoorStyleData;
	
	/* Manual doorway placements (designer-specified) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Manual")
	TArray<FFixedDoorLocation> ForcedDoorways;

	/* Enable automatic standard doorway generation (for hallway connectivity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Automatic")
	bool bGenerateStandardDoorway = true;

	/* Standard doorway width in cells (for hallway connections) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Automatic", meta = (ClampMin = "2", ClampMax = "8"))
	int32 StandardDoorwayWidth = 4;

	/* Use fixed edge for standard doorway (true) or pick randomly (false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Automatic")
	bool bSetStandardDoorwayEdge = false;
	
	/* Which edge to place standard doorway (if bSetStandardDoorwayEdge = true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Automatic", meta = (EditCondition = "bSetStandardDoorwayEdge"))
	EWallEdge StandardDoorwayEdge = EWallEdge::North;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Automatic Placement", 
	meta = (EditCondition = "bGenerateStandardDoorway && ! bSetStandardDoorwayEdge"))
	bool bMultipleDoorways = false;
	
	/* Number of automatic doorways to generate (only used if bMultipleDoorways = true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Automatic Placement", 
		meta = (ClampMin = "2", ClampMax = "4", EditCondition = "bMultipleDoorways && bGenerateStandardDoorway && !bSetStandardDoorwayEdge"))
	int32 NumAutomaticDoorways = 2;

	/* Default door data (used for standard doorway if not specified) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Doorways|Defaults")
	UDoorData* DefaultDoorData = nullptr;
#pragma endregion
	
#pragma region Ceiling Style Data
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Styles")
	TSoftObjectPtr<UCeilingData> CeilingStyleData;
	
	/* Array of specific ceiling tiles to force-place at exact coordinates */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Overrides|Ceiling")
	TArray<FForcedCeilingPlacement> ForcedCeilingPlacements;
	
	// --- Interior Mesh Randomization Pool ---

	// Meshes used to fill the interior of the room grid (clutter, furniture, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior Meshes")
	TArray<FMeshPlacementInfo> InteriorMeshPool;
#pragma endregion
};
