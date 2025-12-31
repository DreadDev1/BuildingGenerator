// Fill out your copyright notice in the Description page of Project Settings.


#include "Generators/Room/PresetRoomGenerator.h"
#include "Data/Room/RoomData.h"
#include "Data/Presets/RoomPreset.h"
#include "Data/Room//FloorData.h"
#include "Data/Room/CeilingData.h"
#include "Utilities/Generation/RoomGenerationHelpers.h"

#pragma region Room Generation Overrides
bool UPresetRoomGenerator::GenerateFloor()
{
	if (!bIsInitialized)
	{ UE_LOG(LogTemp, Error, TEXT("PresetRoomGenerator:: GenerateFloor - Not initialized! ")); return false; }

	// Check if using preset layout
	if (! IsUsingPresetLayout())
	{
		UE_LOG(LogTemp, Warning, TEXT("PresetRoomGenerator::GenerateFloor - No preset layout, using base generation"));
		return Super::GenerateFloor(); // Fall back to base implementation
	}

	URoomPreset* PresetLayout = GetPresetLayout();
	if (!PresetLayout)
	{ UE_LOG(LogTemp, Error, TEXT("PresetRoomGenerator::GenerateFloor - Failed to load preset layout")); return false; }

	// Clear previous floor data
	ClearPlacedFloorMeshes();

	UE_LOG(LogTemp, Log, TEXT("PresetRoomGenerator:: GenerateFloor - Starting region-based generation"));
	UE_LOG(LogTemp, Log, TEXT("  Total regions: %d"), PresetLayout->Regions.Num());

	int32 TotalTilesPlaced = 0;

	// PHASE 1: Generate floor for each region (sorted by priority)
	TArray<FPresetRegion> SortedRegions = PresetLayout->Regions;
	SortedRegions.Sort([](const FPresetRegion& A, const FPresetRegion& B) {
		return A.FillPriority > B.FillPriority;	}); // Higher priority first


	for (const FPresetRegion& Region : SortedRegions)
	{
		// Load region floor style
		UFloorData* RegionFloorStyle = Region.RegionFloorStyle.LoadSynchronous();
		if (!RegionFloorStyle)
		{ UE_LOG(LogTemp, Warning, TEXT("  Region '%s' has no floor style, skipping"), *Region.RegionName);	continue; }

		int32 RegionTiles = GenerateFloorForRegion(Region, RegionFloorStyle);
		TotalTilesPlaced += RegionTiles;

		UE_LOG(LogTemp, Log, TEXT("  Region '%s':  Placed %d floor tiles"), *Region.RegionName, RegionTiles);
	}

	// PHASE 2: Fill remaining cells with default floor style (if defined)
	if (!PresetLayout->DefaultFloorStyle.IsNull())
	{
		UFloorData* DefaultFloor = PresetLayout->DefaultFloorStyle.LoadSynchronous();
		if (DefaultFloor)
		{
			// TODO: Implement filling remaining empty cells with default style
			UE_LOG(LogTemp, Log, TEXT("  Default floor style: Not yet implemented"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("PresetRoomGenerator::GenerateFloor - Complete:  %d total tiles placed"), TotalTilesPlaced);

	return TotalTilesPlaced > 0;
}

bool UPresetRoomGenerator::GenerateCeiling()
{
	if (!bIsInitialized)
	{ UE_LOG(LogTemp, Error, TEXT("PresetRoomGenerator::GenerateCeiling - Not initialized!")); return false; }

	// Check if using preset layout
	if (!IsUsingPresetLayout())
	{
		UE_LOG(LogTemp, Warning, TEXT("PresetRoomGenerator::GenerateCeiling - No preset layout, using base generation"));
		return Super::GenerateCeiling(); // Fall back to base implementation
	}

	URoomPreset* PresetLayout = GetPresetLayout();
	if (!PresetLayout)
	{ UE_LOG(LogTemp, Error, TEXT("PresetRoomGenerator::GenerateCeiling - Failed to load preset layout")); return false; }

	// Clear previous ceiling data
	ClearPlacedCeiling();

	UE_LOG(LogTemp, Log, TEXT("PresetRoomGenerator::GenerateCeiling - Starting region-based generation"));

	int32 TotalTilesPlaced = 0;

	// Generate ceiling for each region (sorted by priority)
	TArray<FPresetRegion> SortedRegions = PresetLayout->Regions;
	SortedRegions.Sort([](const FPresetRegion& A, const FPresetRegion& B) {
		return A.FillPriority > B.FillPriority;
	});

	for (const FPresetRegion& Region :  SortedRegions)
	{
		UCeilingData* RegionCeilingStyle = Region.RegionCeilingStyle.LoadSynchronous();
		if (!RegionCeilingStyle)
		{ UE_LOG(LogTemp, Warning, TEXT("  Region '%s' has no ceiling style, skipping"), *Region.RegionName); continue; }

		int32 RegionTiles = GenerateCeilingForRegion(Region, RegionCeilingStyle);
		TotalTilesPlaced += RegionTiles;

		UE_LOG(LogTemp, Log, TEXT("  Region '%s': Placed %d ceiling tiles"), *Region.RegionName, RegionTiles);
	}

	UE_LOG(LogTemp, Log, TEXT("PresetRoomGenerator::GenerateCeiling - Complete: %d total tiles placed"), TotalTilesPlaced);

	return TotalTilesPlaced > 0;	
}
#pragma endregion

#pragma region Region Generation Functions
int32 UPresetRoomGenerator::GenerateFloorForRegion(const FPresetRegion& Region, UFloorData* FloorStyleData)
{
	if (!FloorStyleData || FloorStyleData->FloorTilePool.Num() == 0)
	{ UE_LOG(LogTemp, Warning, TEXT("GenerateFloorForRegion - No floor tiles in pool")); return 0; }

	int32 TilesPlaced = 0;
	
	UE_LOG(LogTemp, Log, TEXT("  Filling region '%s' with multiple tile sizes..."), *Region.RegionName);

	// MULTI-PASS TILE PLACEMENT (large to small)
	// This matches the base generator's approach

	// PASS 1: Large tiles (4x4, 4x2, 2x4)
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(4, 4));
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(4, 2));
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(2, 4));

	// PASS 2: Medium tiles (2x2)
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(2, 2));

	// PASS 3: Small tiles (2x1, 1x2, 1x1)
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(2, 1));
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(1, 2));
	TilesPlaced += FillRegionWithTileSize(Region, FloorStyleData->FloorTilePool, FIntPoint(1, 1));

	UE_LOG(LogTemp, Log, TEXT("  Region '%s' complete:  %d tiles placed"), *Region.RegionName, TilesPlaced);

	return TilesPlaced;
}

int32 UPresetRoomGenerator::FillRegionWithTileSize(
	const FPresetRegion& Region,
	const TArray<FMeshPlacementInfo>& TilePool,
	FIntPoint TargetSize)
{
	// Filter tiles that match target size
	TArray<FMeshPlacementInfo> MatchingTiles;

	for (const FMeshPlacementInfo& MeshInfo : TilePool)
	{
		FIntPoint Footprint = CalculateFootprint(MeshInfo);

		// Check if footprint matches target size (or rotated version)
		if ((Footprint.X == TargetSize.X && Footprint.Y == TargetSize.Y) ||
			(Footprint.X == TargetSize.Y && Footprint.Y == TargetSize.X))
		{
			MatchingTiles.Add(MeshInfo);
		}
	}

	if (MatchingTiles.Num() == 0)
	{
		return 0; // No tiles of this size available
	}

	int32 TilesPlaced = 0;

	// Iterate through region bounds only
	for (int32 Y = Region.StartCell.Y; Y <= Region.EndCell.Y; ++Y)
	{
		for (int32 X = Region.StartCell.X; X <= Region.EndCell.X; ++X)
		{
			FIntPoint StartCoord(X, Y);

			// Check if tile would fit entirely within region
			FIntPoint TileEnd = StartCoord + TargetSize - FIntPoint(1, 1);
			if (TileEnd. X > Region.EndCell.X || TileEnd.Y > Region.EndCell.Y)
				continue; // Tile would extend outside region

			// Check if area is available
			if (! IsAreaAvailable(StartCoord, TargetSize))
				continue;

			// Select weighted mesh
			FMeshPlacementInfo SelectedMesh = SelectWeightedMesh(MatchingTiles);
			FIntPoint OriginalFootprint = CalculateFootprint(SelectedMesh);

			// Find rotation that matches target size
			int32 BestRotation = 0;

			if (SelectedMesh.AllowedRotations.Num() > 0)
			{
				TArray<int32> ValidRotations;

				for (int32 Rotation : SelectedMesh.AllowedRotations)
				{
					FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);

					if (RotatedFootprint.X == TargetSize. X && RotatedFootprint. Y == TargetSize.Y)
					{
						ValidRotations.Add(Rotation);
					}
				}

				if (ValidRotations.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, ValidRotations. Num() - 1);
					BestRotation = ValidRotations[RandomIndex];
				}
			}

			// Try to place mesh
			if (TryPlaceMesh(StartCoord, TargetSize, SelectedMesh, BestRotation))
			{
				TilesPlaced++;
			}
		}
	}

	return TilesPlaced;
}

int32 UPresetRoomGenerator::GenerateCeilingForRegion(const FPresetRegion& Region, UCeilingData* CeilingStyleData)
{
	if (!CeilingStyleData || CeilingStyleData->CeilingTilePool.Num() == 0)
	{ UE_LOG(LogTemp, Warning, TEXT("GenerateCeilingForRegion - No ceiling tiles in pool")); return 0; }

	// TODO: Implement ceiling generation for region
	// For now, just return 0 (placeholder)
	UE_LOG(LogTemp, Warning, TEXT("GenerateCeilingForRegion - Not yet implemented"));
	return 0;
}

bool UPresetRoomGenerator::ShouldRegionFillCoordinate(FIntPoint Coordinate, const FPresetRegion& Region) const
{
	// Check if coordinate is in this region
	if (!URoomGenerationHelpers::IsCoordinateInRegion(Coordinate, Region)) return false;

	// If coordinate is already occupied, don't fill
	if (GetCellState(Coordinate) != EGridCellType::ECT_Empty) return false;

	// TODO: Check region overlap rules if needed
	// For now, just allow if in region and empty

	return true;
}
#pragma endregion