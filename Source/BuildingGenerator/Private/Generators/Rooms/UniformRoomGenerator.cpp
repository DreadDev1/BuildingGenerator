// Fill out your copyright notice in the Description page of Project Settings.


#include "Generators/Rooms/UniformRoomGenerator.h"


void UUniformRoomGenerator::CreateGrid()
{
	if (!bIsInitialized) 
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::CreateGrid - Generator not initialized!")); return; }

	UE_LOG(LogTemp, Log, TEXT("UniformRoomGenerator: Creating uniform rectangular grid..."));
    
	// Initialize grid state array (all floor cells for uniform room)
	GridState.SetNum(GridSize.X * GridSize.Y);
	for (EGridCellType& Cell : GridState) { Cell = EGridCellType::ECT_FloorMesh; }
    
	// Log statistics
	int32 TotalCells = GetTotalCellCount();
	UE_LOG(LogTemp, Log, TEXT("UniformRoomGenerator: Grid created - %d x %d (%d cells)"), GridSize.X, GridSize.Y, TotalCells);
}

bool UUniformRoomGenerator::GenerateFloor()
{
if (!bIsInitialized)
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateFloor - Generator not initialized!")); return false; }

	if (! RoomData || !RoomData->FloorStyleData)
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator:: GenerateFloor - FloorData not assigned!")); return false; }

	// Load FloorData and keep strong reference throughout function
	UFloorData* FloorStyleData = RoomData->FloorStyleData.LoadSynchronous();
	if (!FloorStyleData)
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateFloor - Failed to load FloorStyleData!")); return false; }

	// Validate FloorTilePool exists
	if (FloorStyleData->FloorTilePool. Num() == 0)
	{ UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateFloor - No floor meshes defined in FloorTilePool!")); return false;}
	
	// Clear previous placement data
	ClearPlacedFloorMeshes();
	
	int32 FloorLargeTilesPlaced = 0;
	int32 FloorMediumTilesPlaced = 0;
	int32 FloorSmallTilesPlaced = 0;
	int32 FloorFillerTilesPlaced = 0;

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateFloor - Starting floor generation"));

 
	// PHASE 0:  FORCED EMPTY REGIONS (Mark cells as reserved)
 	TArray<FIntPoint> ForcedEmptyCells = ExpandForcedEmptyRegions();
	if (ForcedEmptyCells.Num() > 0)
	{
		MarkForcedEmptyCells(ForcedEmptyCells);
		UE_LOG(LogTemp, Log, TEXT("  Phase 0: Marked %d forced empty cells"), ForcedEmptyCells. Num());
	}
	
	// PHASE 1: FORCED PLACEMENTS (Designer overrides - highest priority)
 	int32 ForcedCount = ExecuteForcedPlacements();
	UE_LOG(LogTemp, Log, TEXT("  Phase 1: Placed %d forced meshes"), ForcedCount);
	
	// PHASE 2: GREEDY FILL (Large → Medium → Small)
 	// Use the FloorData pointer we loaded at the top (safer than re-accessing)
	const TArray<FMeshPlacementInfo>& FloorMeshes = FloorStyleData->FloorTilePool;
	UE_LOG(LogTemp, Log, TEXT("  Phase 2: Greedy fill with %d tile options"), FloorMeshes.Num());

	// Large tiles (400x400, 200x400, 400x200)
	FillWithTileSize(FloorMeshes, FIntPoint(4, 4), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	FillWithTileSize(FloorMeshes, FIntPoint(2, 4), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	FillWithTileSize(FloorMeshes, FIntPoint(4, 2), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);

	// Medium tiles (200x200)
	FillWithTileSize(FloorMeshes, FIntPoint(2, 2), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);

	// Small tiles (100x200, 200x100, 100x100)
	FillWithTileSize(FloorMeshes, FIntPoint(1, 2), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	FillWithTileSize(FloorMeshes, FIntPoint(2, 1), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	FillWithTileSize(FloorMeshes, FIntPoint(1, 1), FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	
	// PHASE 3: GAP FILL (Fill remaining empty cells with any available mesh)
	int32 GapFillCount = FillRemainingGaps(FloorMeshes, FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	UE_LOG(LogTemp, Log, TEXT("  Phase 3:  Filled %d remaining gaps"), GapFillCount);
 
	// FINAL STATISTICS
	int32 RemainingEmpty = GetCellCountByType(EGridCellType::ECT_Empty);
	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateFloor - Floor generation complete"));
	UE_LOG(LogTemp, Log, TEXT("  Total meshes placed: %d"), PlacedFloorMeshes.Num());
	UE_LOG(LogTemp, Log, TEXT("  Large:  %d, Medium: %d, Small: %d, Filler: %d"), 
		FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	UE_LOG(LogTemp, Log, TEXT("  Remaining empty cells: %d"), RemainingEmpty);

	return true;
}

bool UUniformRoomGenerator::GenerateWalls()
{
	// Will implement in Step 4
	return false;
}

bool UUniformRoomGenerator::GenerateCorners()
{
	// Will implement in Step 5
	return false;
}

bool UUniformRoomGenerator::GenerateDoorways()
{
	// Will implement in Step 6
	return false;
}

bool UUniformRoomGenerator::GenerateCeiling()
{
	// Will implement in Step 7
	return false;
}