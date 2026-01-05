// Fill out your copyright notice in the Description page of Project Settings.

#include "Generators/Rooms/ChunkyRoomGenerator.h"

#include "Utilities/Generation/RoomGenerationHelpers.h"

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
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::GenerateWalls - Not initialized! "));
		return false;
	}

	if (! RoomData || RoomData->WallStyleData.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::GenerateWalls - No WallData assigned!"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::GenerateWalls - Starting wall generation"));

	// Clear previous walls
	ClearPlacedWalls();

	// Fill each edge using adapted base class logic
	FillChunkyWallEdge(EWallEdge::North);
	FillChunkyWallEdge(EWallEdge::South);
	FillChunkyWallEdge(EWallEdge::East);
	FillChunkyWallEdge(EWallEdge::West);

	UE_LOG(LogTemp, Log, TEXT("  Placed %d base wall segments"), PlacedBaseWallSegments.Num());

	// Spawn middle and top layers (uses base class logic)
	SpawnMiddleWallLayers();
	SpawnTopWallLayer();

	UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::GenerateWalls - Complete!  %d walls placed"), 
		PlacedWallMeshes.Num());

	return true;
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

#pragma region Wall Generation Helpers
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

void UChunkyRoomGenerator::FillChunkyWallEdge(EWallEdge Edge)
{
    if (! RoomData || RoomData->WallStyleData.IsNull()) return;

    WallData = RoomData->WallStyleData.LoadSynchronous();
    if (!WallData || WallData->AvailableWallModules. Num() == 0) return;

    // Get perimeter cells for this edge (replaces GetEdgeCellIndices)
    TArray<FIntPoint> EdgeCells = GetPerimeterCellsForEdge(Edge);
    if (EdgeCells.Num() == 0) return;

    FRotator WallRotation = URoomGenerationHelpers::GetWallRotationForEdge(Edge);
    
    // Get wall offsets
    float NorthOffset = WallData->NorthWallOffsetX;
    float SouthOffset = WallData->SouthWallOffsetX;
    float EastOffset = WallData->EastWallOffsetY;
    float WestOffset = WallData->WestWallOffsetY;

    UE_LOG(LogTemp, Verbose, TEXT("  Filling %s edge with %d cells"),
        *UEnum::GetValueAsString(Edge), EdgeCells.Num());

    // GREEDY BIN PACKING:  Fill with largest modules first
    int32 CurrentCell = 0;

    while (CurrentCell < EdgeCells. Num())
    {
        int32 SpaceLeft = EdgeCells.Num() - CurrentCell;

        // Find largest module that fits
        const FWallModule* BestModule = nullptr;

        for (const FWallModule& Module : WallData->AvailableWallModules)
        {
            // Check if module fits remaining space
            if (Module.Y_AxisFootprint <= SpaceLeft)
            {
                // Check if cells are consecutive
                bool bCellsConsecutive = true;
                
                for (int32 i = 1; i < Module.Y_AxisFootprint; ++i)
                {
                    FIntPoint CurrentCellPos = EdgeCells[CurrentCell + i - 1];
                    FIntPoint NextCellPos = EdgeCells[CurrentCell + i];
                    
                    // Check adjacency based on edge direction
                    bool bAdjacent = false;
                    
                    if (Edge == EWallEdge::North || Edge == EWallEdge:: South)
                    {
                        // Horizontal:  X should increment by 1, Y same
                        bAdjacent = (NextCellPos.X == CurrentCellPos.X + 1) && (NextCellPos.Y == CurrentCellPos.Y);
                    }
                    else // East or West
                    {
                        // Vertical: Y should increment by 1, X same
                        bAdjacent = (NextCellPos.Y == CurrentCellPos.Y + 1) && (NextCellPos.X == CurrentCellPos.X);
                    }
                    
                    if (! bAdjacent)
                    {
                        bCellsConsecutive = false;
                        break;
                    }
                }
                
                if (bCellsConsecutive)
                {
                    // Choose largest consecutive module
                    if (! BestModule || Module.Y_AxisFootprint > BestModule->Y_AxisFootprint)
                    {
                        BestModule = &Module;
                    }
                }
            }
        }

        if (! BestModule)
        {
            UE_LOG(LogTemp, Warning, TEXT("    No wall module fits at cell %d (remaining: %d)"), 
                CurrentCell, SpaceLeft);
            CurrentCell++;  // Skip this cell
            continue;
        }

        // Load base mesh
        UStaticMesh* BaseMesh = BestModule->BaseMesh. LoadSynchronous();
        if (!BaseMesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("    Failed to load base mesh"));
            CurrentCell++;
            continue;
        }

        // Get starting cell for this module
        FIntPoint StartCell = EdgeCells[CurrentCell];

        // Calculate wall position using adapted logic
        FVector WallPosition = CalculateWallPositionForSegment(
            Edge,
            StartCell,
            BestModule->Y_AxisFootprint,
            NorthOffset,
            SouthOffset,
            EastOffset,
            WestOffset
        );

        // Create transform
        FTransform BaseTransform(WallRotation, WallPosition, FVector:: OneVector);

        // Store segment for middle/top spawning
        FGeneratorWallSegment Segment;
        Segment.Edge = Edge;
        Segment. StartCell = CurrentCell;
        Segment. SegmentLength = BestModule->Y_AxisFootprint;
        Segment.BaseTransform = BaseTransform;
        Segment.BaseMesh = BaseMesh;
        Segment.WallModule = BestModule;

        PlacedBaseWallSegments.Add(Segment);

        UE_LOG(LogTemp, VeryVerbose, TEXT("    Placed %dY module at cell (%d,%d)"),
            BestModule->Y_AxisFootprint, StartCell.X, StartCell.Y);

        // Advance by module footprint
        CurrentCell += BestModule->Y_AxisFootprint;
    }
}

FIntPoint UChunkyRoomGenerator::GetDirectionOffset(EWallEdge Direction) const
{
	switch (Direction)
	{
	case EWallEdge::North:  return FIntPoint(0, 1);
	case EWallEdge::South: return FIntPoint(0, -1);
	case EWallEdge:: East:   return FIntPoint(1, 0);
	case EWallEdge::West:  return FIntPoint(-1, 0);
	default: return FIntPoint::ZeroValue;
	}
}

TArray<FIntPoint> UChunkyRoomGenerator::GetPerimeterCellsForEdge(EWallEdge Edge) const
{
    TArray<FIntPoint> EdgeCells;

    // Determine which direction to check for void based on edge
    FIntPoint VoidCheckOffset;
    
    switch (Edge)
    {
        case EWallEdge::North:
            VoidCheckOffset = FIntPoint(0, 1);   // Check North neighbor (Y+1)
            break;
        case EWallEdge::South:
            VoidCheckOffset = FIntPoint(0, -1);  // Check South neighbor (Y-1)
            break;
        case EWallEdge::East:
            VoidCheckOffset = FIntPoint(1, 0);   // Check East neighbor (X+1)
            break;
        case EWallEdge::West:
            VoidCheckOffset = FIntPoint(-1, 0);  // Check West neighbor (X-1)
            break;
    	default: 
    		return EdgeCells;
    }

    // Scan all grid cells to find floor cells with void in the specified direction
    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            FIntPoint Cell(X, Y);
            
            // Must be a floor cell
            if (GetCellState(Cell) != EGridCellType::ECT_FloorMesh)
                continue;

            // Check if neighbor in specified direction is void
            FIntPoint Neighbor = Cell + VoidCheckOffset;
            
            if (! IsValidGridCoordinate(Neighbor) || GetCellState(Neighbor) == EGridCellType::ECT_Void)
            {
                EdgeCells.Add(Cell);
            }
        }
    }

    // Sort cells to create a linear sequence
    if (Edge == EWallEdge::North || Edge == EWallEdge::South)
    {
        // Horizontal walls: sort by X coordinate
        EdgeCells.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X < B. X; });
    }
    else // East or West
    {
        // Vertical walls: sort by Y coordinate
        EdgeCells.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.Y < B.Y; });
    }

    UE_LOG(LogTemp, Verbose, TEXT("  GetPerimeterCellsForEdge(%s): Found %d cells"), 
        *UEnum::GetValueAsString(Edge), EdgeCells.Num());

    return EdgeCells;
}

FVector UChunkyRoomGenerator::CalculateWallPositionForSegment(EWallEdge Direction, FIntPoint StartCell,
	int32 ModuleFootprint, float NorthOffset, float SouthOffset, float EastOffset, float WestOffset) const
{
	FVector Position = FVector::ZeroVector;

	// Calculate center of the module span
	float HalfFootprint = (ModuleFootprint - 1) * 0.5f;

	switch (Direction)
	{
	case EWallEdge::North:
		// Wall faces south (into room), positioned at cell's north edge
		Position. X = (StartCell.X + HalfFootprint) * CellSize + (CellSize * 0.5f) + NorthOffset;
		Position.Y = (StartCell.Y * CellSize) + CellSize + (CellSize * 0.5f);
		break;

	case EWallEdge::South:
		// Wall faces north (into room), positioned at cell's south edge
		Position. X = (StartCell.X + HalfFootprint) * CellSize + (CellSize * 0.5f) + SouthOffset;
		Position.Y = (StartCell.Y * CellSize);
		break;

	case EWallEdge::East: 
		// Wall faces west (into room), positioned at cell's east edge
		Position.X = (StartCell.X * CellSize) + CellSize;
		Position.Y = (StartCell.Y + HalfFootprint) * CellSize + (CellSize * 0.5f) + EastOffset;
		break;

	case EWallEdge:: West:
		// Wall faces east (into room), positioned at cell's west edge
		Position.X = (StartCell.X * CellSize);
		Position.Y = (StartCell.Y + HalfFootprint) * CellSize + (CellSize * 0.5f) + WestOffset;
		break;
	}

	Position. Z = 0.0f;  // Floor level

	return Position;
}
#pragma endregion
#pragma endregion

