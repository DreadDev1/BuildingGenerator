// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RoomGenerator.h"
#include "PresetRoomGenerator.generated.h"

/**
 * 
 */
UCLASS()
class BUILDINGGENERATOR_API UPresetRoomGenerator : public URoomGenerator
{
	GENERATED_BODY()

public:
#pragma region Room Generation Overrides
	/** Generate floor meshes per-region using region-specific floor styles 
	 * Overrides base implementation to respect preset regions */
	virtual bool GenerateFloor() override;

	/** Generate ceiling meshes per-region using region-specific ceiling styles
	 * Overrides base implementation to respect preset regions */
	virtual bool GenerateCeiling() override;
#pragma endregion
	
#pragma region Region Generation Functions
	/** Generate floor for a specific region */
	int32 GenerateFloorForRegion(const FPresetRegion& Region, class UFloorData* FloorStyleData);
	
	/** Fill region with tiles of specific size */
	int32 FillRegionWithTileSize(
	const FPresetRegion& Region, const TArray<FMeshPlacementInfo>& TilePool, FIntPoint TargetSize);

	/** Generate ceiling for a specific region */
	int32 GenerateCeilingForRegion(const FPresetRegion& Region, class UCeilingData* CeilingStyleData);

	/** Check if a coordinate should be filled by this region */
	bool ShouldRegionFillCoordinate(FIntPoint Coordinate, const FPresetRegion& Region) const;
#pragma endregion
};
