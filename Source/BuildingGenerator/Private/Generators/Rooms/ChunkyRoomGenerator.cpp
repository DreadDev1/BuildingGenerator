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

bool UChunkyRoomGenerator:: GenerateWalls()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::GenerateWalls - Not initialized! "));
		return false;
	}

	if (!RoomData || RoomData->WallStyleData. IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::GenerateWalls - No WallData assigned!"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::GenerateWalls - Starting wall generation"));

	// Clear previous walls
	ClearPlacedWalls();

	// PHASE 1: Get perimeter cells with direction info
	TArray<FPerimeterCellInfo> PerimeterCells = GetPerimeterCellsWithDirection();
    
	if (PerimeterCells. Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("  No perimeter cells found! "));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("  Found %d perimeter wall placements"), PerimeterCells.Num());

	// PHASE 2: Group into segments
	TArray<FChunkyWallSegment> WallSegments = GroupPerimeterIntoSegments(PerimeterCells);
    
	UE_LOG(LogTemp, Log, TEXT("  Grouped into %d wall segments"), WallSegments.Num());

	// PHASE 3: Place base walls for each segment
	for (const FChunkyWallSegment& Segment : WallSegments)
	{
		PlaceWallSegment(Segment);
	}

	UE_LOG(LogTemp, Log, TEXT("  Placed %d base wall segments"), PlacedBaseWallSegments. Num());

	// PHASE 4: Spawn middle and top layers (uses base class logic)
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

#pragma region Wall Generation Helpers
TArray<FPerimeterCellInfo> UChunkyRoomGenerator::GetPerimeterCellsWithDirection() const
{
    TArray<FPerimeterCellInfo> PerimeterInfo;

    // Check all floor cells to find perimeter
    for (int32 X = 0; X < GridSize. X; ++X)
    {
        for (int32 Y = 0; Y < GridSize.Y; ++Y)
        {
            FIntPoint Cell(X, Y);
            
            // Only check floor mesh cells (already placed floors)
            if (GetCellState(Cell) != EGridCellType::ECT_FloorMesh)
                continue;

            // Check all 4 directions to see if this cell borders void
            TArray<EWallEdge> WallDirections;
            
            // North (Y+1)
            FIntPoint NorthNeighbor = Cell + FIntPoint(0, 1);
            if (! IsValidGridCoordinate(NorthNeighbor) || GetCellState(NorthNeighbor) == EGridCellType::ECT_Void)
            {
                WallDirections.Add(EWallEdge::North);  // Wall faces south (into room)
            }
            
            // South (Y-1)
            FIntPoint SouthNeighbor = Cell + FIntPoint(0, -1);
            if (!IsValidGridCoordinate(SouthNeighbor) || GetCellState(SouthNeighbor) == EGridCellType::ECT_Void)
            {
                WallDirections.Add(EWallEdge::South);  // Wall faces north
            }
            
            // East (X+1)
            FIntPoint EastNeighbor = Cell + FIntPoint(1, 0);
            if (!IsValidGridCoordinate(EastNeighbor) || GetCellState(EastNeighbor) == EGridCellType::ECT_Void)
            {
                WallDirections.Add(EWallEdge::East);  // Wall faces west
            }
            
            // West (X-1)
            FIntPoint WestNeighbor = Cell + FIntPoint(-1, 0);
            if (!IsValidGridCoordinate(WestNeighbor) || GetCellState(WestNeighbor) == EGridCellType::ECT_Void)
            {
                WallDirections.Add(EWallEdge::West);  // Wall faces east
            }

            // Add perimeter cell info for each direction that needs a wall
            bool bIsCorner = WallDirections.Num() > 1;
            
            for (EWallEdge Direction :  WallDirections)
            {
                PerimeterInfo.Add(FPerimeterCellInfo(Cell, Direction, bIsCorner));
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::GetPerimeterCellsWithDirection - Found %d perimeter walls"), 
        PerimeterInfo.Num());

    return PerimeterInfo;
}

EWallEdge UChunkyRoomGenerator::DetermineWallDirection(FIntPoint Cell) const
{
    // Check each direction and return first one that borders void
    // North
    FIntPoint NorthNeighbor = Cell + FIntPoint(0, 1);
    if (!IsValidGridCoordinate(NorthNeighbor) || GetCellState(NorthNeighbor) == EGridCellType::ECT_Void)
        return EWallEdge::North;
    
    // South
    FIntPoint SouthNeighbor = Cell + FIntPoint(0, -1);
    if (!IsValidGridCoordinate(SouthNeighbor) || GetCellState(SouthNeighbor) == EGridCellType::ECT_Void)
        return EWallEdge::South;
    
    // East
    FIntPoint EastNeighbor = Cell + FIntPoint(1, 0);
    if (!IsValidGridCoordinate(EastNeighbor) || GetCellState(EastNeighbor) == EGridCellType::ECT_Void)
        return EWallEdge::East;
    
    // West
    FIntPoint WestNeighbor = Cell + FIntPoint(-1, 0);
    if (!IsValidGridCoordinate(WestNeighbor) || GetCellState(WestNeighbor) == EGridCellType::ECT_Void)
        return EWallEdge::West;

    // Default fallback
    return EWallEdge::North;
}

bool UChunkyRoomGenerator:: IsCornerCell(FIntPoint Cell) const
{
    int32 VoidNeighborCount = 0;

    // Count how many neighbors are void
    TArray<FIntPoint> Directions = {
        FIntPoint(0, 1),   // North
        FIntPoint(0, -1),  // South
        FIntPoint(1, 0),   // East
        FIntPoint(-1, 0)   // West
    };

    for (const FIntPoint& Dir : Directions)
    {
        FIntPoint Neighbor = Cell + Dir;
        if (!IsValidGridCoordinate(Neighbor) || GetCellState(Neighbor) == EGridCellType::ECT_Void)
        {
            VoidNeighborCount++;
        }
    }

    // Corner if 2+ sides face void
    return VoidNeighborCount >= 2;
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

TArray<FChunkyWallSegment> UChunkyRoomGenerator::GroupPerimeterIntoSegments(
    const TArray<FPerimeterCellInfo>& PerimeterCells) const
{
    TArray<FChunkyWallSegment> Segments;

    // Group cells by direction
    TMap<EWallEdge, TArray<FIntPoint>> CellsByDirection;
    
    for (const FPerimeterCellInfo& CellInfo : PerimeterCells)
    {
        CellsByDirection. FindOrAdd(CellInfo.FacingDirection).Add(CellInfo.Cell);
    }

    // For each direction, create segments from consecutive cells
    for (const auto& Pair : CellsByDirection)
    {
        EWallEdge Direction = Pair.Key;
        const TArray<FIntPoint>& Cells = Pair.Value;

        if (Cells.Num() == 0) continue;

        // Simple approach: Each group of cells in same direction = one segment
        FChunkyWallSegment Segment;
        Segment.Direction = Direction;
        Segment. Cells = Cells;
        Segment.StartCell = Cells[0];
        Segment.Length = Cells.Num();

        Segments.Add(Segment);
    }

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::GroupPerimeterIntoSegments - Created %d segments"), 
        Segments.Num());

    return Segments;
}

void UChunkyRoomGenerator::PlaceWallSegment(const FChunkyWallSegment& Segment)
{
    if (! RoomData || RoomData->WallStyleData.IsNull()) return;

    WallData = RoomData->WallStyleData. LoadSynchronous();
    if (!WallData || WallData->AvailableWallModules. Num() == 0) return;

    // Get wall offsets
    float NorthOffset = WallData->NorthWallOffsetX;
    float SouthOffset = WallData->SouthWallOffsetX;
    float EastOffset = WallData->EastWallOffsetY;
    float WestOffset = WallData->WestWallOffsetY;

    FRotator WallRotation = URoomGenerationHelpers::GetWallRotationForEdge(Segment.Direction);

    // For now:  Place a 1-cell wall at each cell in the segment
    // TODO: Later optimize to use larger wall modules
    
    for (const FIntPoint& Cell : Segment.Cells)
    {
        // Find a 1-cell wall module
        const FWallModule* SingleCellModule = nullptr;
        
        for (const FWallModule& Module : WallData->AvailableWallModules)
        {
            if (Module.Y_AxisFootprint == 1)
            {
                SingleCellModule = &Module;
                break;
            }
        }

        if (!SingleCellModule)
        {
            UE_LOG(LogTemp, Warning, TEXT("No 1-cell wall module found! "));
            continue;
        }

        // Load base mesh
        UStaticMesh* BaseMesh = SingleCellModule->BaseMesh. LoadSynchronous();
        if (!BaseMesh) continue;

        // Calculate wall position
        FVector WallPosition = URoomGenerationHelpers:: CalculateWallPosition(
            Segment.Direction,
            0,  // Cell index (not used for single cell)
            1,  // Footprint
            GridSize,
            CellSize,
            NorthOffset,
            SouthOffset,
            EastOffset,
            WestOffset
        );

        // Adjust position to cell coordinate
        WallPosition.X = Cell.X * CellSize + (CellSize * 0.5f);
        WallPosition.Y = Cell. Y * CellSize + (CellSize * 0.5f);

        // Create transform
        FTransform BaseTransform(WallRotation, WallPosition, FVector:: OneVector);

        // Store segment for middle/top spawning
        FGeneratorWallSegment GeneratorSegment;
        GeneratorSegment.Edge = Segment.Direction;
        GeneratorSegment.StartCell = 0;  // Not used for chunky
        GeneratorSegment. SegmentLength = 1;
        GeneratorSegment.BaseTransform = BaseTransform;
        GeneratorSegment.BaseMesh = BaseMesh;
        GeneratorSegment.WallModule = SingleCellModule;

        PlacedBaseWallSegments.Add(GeneratorSegment);
    }

    UE_LOG(LogTemp, Verbose, TEXT("  Placed %d walls for %s-facing segment"), 
        Segment.Cells.Num(), *UEnum::GetValueAsString(Segment.Direction));
}

#pragma endregion
#pragma endregion

