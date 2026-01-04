// Fill out your copyright notice in the Description page of Project Settings.

#include "Generators/Rooms/ChunkyRoomGenerator.h"

void UChunkyRoomGenerator::CreateGrid()
{
	   if (!bIsInitialized)
    { UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::CreateGrid - Generator not initialized!")); return; }

    // SET TARGET CELL TYPE FOR FLOOR GENERATION
    FloorTargetCellType = EGridCellType::ECT_Custom;

    // Initialize random stream
    if (RandomSeed == -1) { RandomStream.Initialize(FMath::Rand()); }
    else{ RandomStream.Initialize(RandomSeed); }

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::CreateGrid - Creating chunky room..."));

    // Step 1: Initialize grid (all cells VOID - outside room)
    int32 TotalCells = GridSize.X * GridSize.Y;
    GridState.SetNum(TotalCells);
    for (EGridCellType& Cell : GridState)
    {
        Cell = EGridCellType::ECT_Void;  // CHANGED: Outside room
    }

    // Step 2: Calculate base room bounds (starting at 0,0)
    BaseRoomSize.X = FMath::Max(4, (int32)(GridSize.X * BaseRoomPercentage));
    BaseRoomSize.Y = FMath::Max(4, (int32)(GridSize.Y * BaseRoomPercentage));
    BaseRoomStart.X = 0;
    BaseRoomStart.Y = 0;

    UE_LOG(LogTemp, Verbose, TEXT("  Base room: Start(%d, %d), Size(%d, %d)"),
        BaseRoomStart.X, BaseRoomStart.Y, BaseRoomSize.X, BaseRoomSize.Y);

    // Step 3: Mark base room cells as CUSTOM (part of room, needs floor)
    MarkRectangle(BaseRoomStart.X, BaseRoomStart.Y, BaseRoomSize.X, BaseRoomSize.Y);

    // Step 4: Add random protrusions
    int32 NumProtrusions = RandomStream.RandRange(MinProtrusions, MaxProtrusions);
    UE_LOG(LogTemp, Verbose, TEXT("  Adding %d protrusions..."), NumProtrusions);

    for (int32 i = 0; i < NumProtrusions; ++i)
    {
        AddRandomProtrusion();
    }

    // Step 5: Log statistics
    int32 CustomCells = GetCellCountByType(EGridCellType::ECT_Custom);
    int32 VoidCells = GetCellCountByType(EGridCellType::ECT_Void);

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::CreateGrid - Complete"));
    UE_LOG(LogTemp, Log, TEXT("  Grid: %d x %d (%d cells)"), GridSize.X, GridSize.Y, TotalCells);
    UE_LOG(LogTemp, Log, TEXT("  Custom (room area): %d cells"), CustomCells);
    UE_LOG(LogTemp, Log, TEXT("  Void (outside): %d cells"), VoidCells);
    UE_LOG(LogTemp, Log, TEXT("  Protrusions: %d"), NumProtrusions);
}

bool UChunkyRoomGenerator::GenerateFloor()
{
if (!bIsInitialized)
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateFloor - Generator not initialized!")); return false; }

	if (! RoomData || !RoomData->FloorStyleData)
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator:: GenerateFloor - FloorData not assigned!")); return false; }

	// Load FloorData and keep strong reference throughout function
	UFloorData* FloorStyleData = RoomData->FloorStyleData.LoadSynchronous();
	if (!FloorStyleData)
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateFloor - Failed to load FloorStyleData!")); return false; }

	// Validate FloorTilePool exists
	if (FloorStyleData->FloorTilePool. Num() == 0)
	{ UE_LOG(LogTemp, Warning, TEXT("UUniformRoomGenerator::GenerateFloor - No floor meshes defined in FloorTilePool!")); return false;}
	
	// Clear previous placement data
	//ClearPlacedFloorMeshes();
	//ClearPlacedFloorMeshes();
	
	int32 FloorLargeTilesPlaced = 0;
	int32 FloorMediumTilesPlaced = 0;
	int32 FloorSmallTilesPlaced = 0;
	int32 FloorFillerTilesPlaced = 0;

	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateFloor - Starting floor generation"));

 
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
	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateFloor - Floor generation complete"));
	UE_LOG(LogTemp, Log, TEXT("  Total meshes placed: %d"), PlacedFloorMeshes.Num());
	UE_LOG(LogTemp, Log, TEXT("  Large:  %d, Medium: %d, Small: %d, Filler: %d"), 
		FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	UE_LOG(LogTemp, Log, TEXT("  Remaining empty cells: %d"), RemainingEmpty);

	return true;
}

bool UChunkyRoomGenerator::GenerateWalls()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateCorners()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateDoorways()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateCeiling()
{
	return false;
}

#pragma region Internal Helpers
void UChunkyRoomGenerator::MarkRectangle(int32 StartX, int32 StartY, int32 Width, int32 Height)
{
	// Mark all cells in rectangle as floor
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			FIntPoint Cell(StartX + X, StartY + Y);
			
			// Bounds check
			if (IsValidGridCoordinate(Cell))
			{
				SetCellState(Cell, EGridCellType::ECT_Custom);
			}
		}
	}
}

void UChunkyRoomGenerator::AddRandomProtrusion()
{
	// Pick random edge (0=North, 1=South, 2=East, 3=West)
	int32 EdgeIndex = RandomStream.RandRange(0, 3);
	EWallEdge Edge = (EWallEdge)EdgeIndex;

	// Pick random protrusion dimensions
	int32 ProtrusionWidth = RandomStream.RandRange(MinProtrusionSize, MaxProtrusionSize);
	int32 ProtrusionDepth = RandomStream.RandRange(MinProtrusionSize, MaxProtrusionSize);

	// Calculate protrusion rectangle based on edge
	FIntPoint Start;
	FIntPoint Size;

	switch (Edge)
	{
		case EWallEdge::North: // Extend upward (positive Y)
		{
			// Pick random position along top edge
			int32 EdgeLength = BaseRoomSize.X;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X + Position, BaseRoomStart.Y + BaseRoomSize.Y);
			Size = FIntPoint(ProtrusionWidth, ProtrusionDepth);
			break;
		}

		case EWallEdge::South: // Extend downward (negative Y)
		{
			// Pick random position along bottom edge
			int32 EdgeLength = BaseRoomSize.X;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X + Position, BaseRoomStart.Y - ProtrusionDepth);
			Size = FIntPoint(ProtrusionWidth, ProtrusionDepth);
			break;
		}

		case EWallEdge::East: // Extend right (positive Y direction in grid)
		{
			// Pick random position along right edge
			int32 EdgeLength = BaseRoomSize.Y;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X + BaseRoomSize.X, BaseRoomStart.Y + Position);
			Size = FIntPoint(ProtrusionDepth, ProtrusionWidth);
			break;
		}

		case EWallEdge::West: // Extend left (negative X)
		{
			// Pick random position along left edge
			int32 EdgeLength = BaseRoomSize.Y;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X - ProtrusionDepth, BaseRoomStart.Y + Position);
			Size = FIntPoint(ProtrusionDepth, ProtrusionWidth);
			break;
		}

		default:
			return;
	}

	// Clamp to grid bounds
	int32 ClampedStartX = FMath::Clamp(Start.X, 0, GridSize.X - 1);
	int32 ClampedStartY = FMath::Clamp(Start.Y, 0, GridSize.Y - 1);
	int32 ClampedWidth = FMath::Min(Size.X, GridSize.X - ClampedStartX);
	int32 ClampedHeight = FMath::Min(Size.Y, GridSize.Y - ClampedStartY);

	// Only add if there's valid space
	if (ClampedWidth >= MinProtrusionSize && ClampedHeight >= MinProtrusionSize)
	{
		MarkRectangle(ClampedStartX, ClampedStartY, ClampedWidth, ClampedHeight);
		
		UE_LOG(LogTemp, Verbose, TEXT("    Added protrusion on edge %d: Start(%d,%d), Size(%d,%d)"),
			EdgeIndex, ClampedStartX, ClampedStartY, ClampedWidth, ClampedHeight);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("    Protrusion too small after clamping, skipped"));
	}
}

bool UChunkyRoomGenerator::HasFloorNeighbor(FIntPoint Cell, FIntPoint Direction) const
{
	FIntPoint Neighbor = Cell + Direction;
	
	if (!IsValidGridCoordinate(Neighbor))
	{
		return false;
	}
	
	return GetCellState(Neighbor) == EGridCellType::ECT_FloorMesh;
}

TArray<FIntPoint> UChunkyRoomGenerator::GetPerimeterCells() const
{
	TArray<FIntPoint> PerimeterCells;

	// Check all floor cells to see if they're on the perimeter
	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);
			
			if (GetCellState(Cell) != EGridCellType::ECT_FloorMesh)
			{
				continue; // Not a floor cell
			}

			// Check if any neighbor is empty or out of bounds
			TArray<FIntPoint> Directions = {
				FIntPoint(1, 0),   // East
				FIntPoint(-1, 0),  // West
				FIntPoint(0, 1),   // North
				FIntPoint(0, -1)   // South
			};

			bool IsPerimeter = false;
			for (const FIntPoint& Dir : Directions)
			{
				FIntPoint Neighbor = Cell + Dir;
				
				// Perimeter if neighbor is out of bounds or empty
				if (!IsValidGridCoordinate(Neighbor) || 
					GetCellState(Neighbor) == EGridCellType::ECT_Empty)
				{
					IsPerimeter = true;
					break;
				}
			}

			if (IsPerimeter)
			{
				PerimeterCells.Add(Cell);
			}
		}
	}

	return PerimeterCells;
}
#pragma endregion