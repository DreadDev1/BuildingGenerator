// Fill out your copyright notice in the Description page of Project Settings.

#include "Generators/Rooms/ChunkyRoomGenerator.h"

#include "Utilities/Generation/RoomGenerationHelpers.h"


void UChunkyRoomGenerator:: CreateGrid()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::CreateGrid - Generator not initialized!"));
        return;
    }

    // SET TARGET CELL TYPE FOR FLOOR GENERATION
    FloorTargetCellType = EGridCellType::ECT_Custom;

    // Initialize random stream
    if (RandomSeed == -1) { RandomStream. Initialize(FMath:: Rand()); }
    else { RandomStream.Initialize(RandomSeed); }

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::CreateGrid - Creating chunk-based room... "));

    // ===== STEP 1: Calculate chunk grid dimensions =====
    ChunkGridSize.X = GridSize.X / 2;
    ChunkGridSize.Y = GridSize.Y / 2;

    if (ChunkGridSize.X < 2 || ChunkGridSize.Y < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("  Grid too small for chunk system!  Minimum 4×4 cells required (2×2 chunks)"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("  Cell Grid:  %d×%d cells"), GridSize.X, GridSize.Y);
    UE_LOG(LogTemp, Log, TEXT("  Chunk Grid: %d×%d chunks (each chunk = 2×2 cells = 200cm)"),
        ChunkGridSize.X, ChunkGridSize.Y);

    // ===== STEP 2: Initialize cell grid (all VOID) =====
    int32 TotalCells = GridSize.X * GridSize.Y;
    GridState.SetNum(TotalCells);
    for (EGridCellType& Cell : GridState)
    {
        Cell = EGridCellType::ECT_Void;
    }

    // ===== STEP 3: Initialize chunk state (all void) =====
    int32 TotalChunks = ChunkGridSize.X * ChunkGridSize.Y;
    ChunkState.SetNum(TotalChunks);
    for (bool& Chunk : ChunkState)
    {
        Chunk = false;  // false = void, true = room
    }

    // ===== STEP 4: Create base room in CHUNKS =====
    int32 BaseWidthChunks = (int32)(ChunkGridSize.X * BaseRoomPercentage);
    int32 BaseHeightChunks = (int32)(ChunkGridSize.Y * BaseRoomPercentage);

    // Force even chunks (minimum 2×2)
    BaseRoomSizeChunks.X = FMath::Max(2, (BaseWidthChunks / 2) * 2);
    BaseRoomSizeChunks.Y = FMath::Max(2, (BaseHeightChunks / 2) * 2);
    BaseRoomStartChunks = FIntPoint(0, 0);  // Always start at origin

    // Convert to cells (for doorway/wall systems)
    BaseRoomSize.X = BaseRoomSizeChunks.X * 2;
    BaseRoomSize. Y = BaseRoomSizeChunks.Y * 2;
    BaseRoomStart = FIntPoint(0, 0);

    UE_LOG(LogTemp, Log, TEXT("  Base room (chunks): Start(%d,%d), Size(%d×%d)"),
        BaseRoomStartChunks.X, BaseRoomStartChunks. Y,
        BaseRoomSizeChunks.X, BaseRoomSizeChunks. Y);
    UE_LOG(LogTemp, Log, TEXT("  Base room (cells):  Start(%d,%d), Size(%d×%d)"),
        BaseRoomStart.X, BaseRoomStart. Y,
        BaseRoomSize.X, BaseRoomSize. Y);

    // Mark base room chunks
    MarkChunkRectangle(BaseRoomStartChunks. X, BaseRoomStartChunks.Y,
                       BaseRoomSizeChunks.X, BaseRoomSizeChunks.Y);

    // ===== STEP 5: Add random protrusions (chunk-aligned) =====
    int32 NumProtrusions = RandomStream.RandRange(MinProtrusions, MaxProtrusions);
    UE_LOG(LogTemp, Log, TEXT("  Adding %d protrusions..."), NumProtrusions);

    for (int32 i = 0; i < NumProtrusions; ++i)
    {
        AddRandomProtrusionChunked();
    }

    // ===== STEP 6: Convert chunks → cells =====
    ConvertChunksToCells();

    // ===== STEP 7: Log statistics =====
    int32 CustomCells = GetCellCountByType(EGridCellType:: ECT_Custom);
    int32 VoidCells = GetCellCountByType(EGridCellType::ECT_Void);
    int32 RoomChunks = 0;
    for (bool Chunk : ChunkState) { if (Chunk) RoomChunks++; }

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::CreateGrid - Complete"));
    UE_LOG(LogTemp, Log, TEXT("  Cell Grid: %d×%d (%d cells)"), GridSize.X, GridSize. Y, TotalCells);
    UE_LOG(LogTemp, Log, TEXT("  Chunk Grid: %d×%d (%d chunks)"), ChunkGridSize.X, ChunkGridSize.Y, TotalChunks);
    UE_LOG(LogTemp, Log, TEXT("  Room chunks: %d, Custom cells: %d, Void cells: %d"),
        RoomChunks, CustomCells, VoidCells);
    UE_LOG(LogTemp, Log, TEXT("  Protrusions: %d"), NumProtrusions);
}

bool UChunkyRoomGenerator:: GenerateCorners()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCorners - Not initialized! "));
		return false;
	}

	if (!RoomData || RoomData->WallStyleData.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCorners - No WallData assigned!"));
		return false;
	}

	// Load WallData
	WallData = RoomData->WallStyleData. LoadSynchronous();
	if (!WallData || ! WallData->DefaultCornerMesh.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerateCorners - No corner mesh defined"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("GenerateCorners - Starting"));

	// Clear previous corners
	ClearPlacedCorners();
	CornerOccupiedCells.Empty();

	// PHASE 1: Interior Corners (Chunky-specific)
	GenerateInteriorCorners();

	// PHASE 2: Exterior Corners (TODO - implement later)
	// GenerateExteriorCorners();

	UE_LOG(LogTemp, Log, TEXT("GenerateCorners - Complete!  %d corners placed"), 
		PlacedCornerMeshes.Num());

	return true;
}

bool UChunkyRoomGenerator:: GenerateWalls()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator:: GenerateWalls - Not initialized! "));
        return false;
    }

    if (!RoomData || RoomData->WallStyleData.IsNull())
    {
        UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::GenerateWalls - No WallData assigned!"));
        return false;
    }

    // Clear previous walls
    ClearPlacedWalls();

    // Fill each edge using dual detection (void + boundaries), skipping corners
    FillChunkyWallEdge(EWallEdge::North);
    FillChunkyWallEdge(EWallEdge::South);
    FillChunkyWallEdge(EWallEdge::East);
    FillChunkyWallEdge(EWallEdge::West);

    UE_LOG(LogTemp, Log, TEXT("  Placed %d base wall segments"), PlacedBaseWallSegments.Num());

    // Spawn middle and top layers
    SpawnMiddleWallLayers();
    SpawnTopWallLayer();

    UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::GenerateWalls - Complete!  %d walls placed"), 
        PlacedWallMeshes.Num());

    return true;
}

bool UChunkyRoomGenerator:: GenerateFloor()
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

bool UChunkyRoomGenerator::GenerateDoorways()
{
    return false;
}

bool UChunkyRoomGenerator::GenerateCeiling()
{
    return false;
}

// ===== CHUNK HELPER FUNCTIONS =====

void UChunkyRoomGenerator::MarkChunkRectangle(int32 StartX, int32 StartY, int32 Width, int32 Height)
{
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            FIntPoint ChunkCoord(StartX + X, StartY + Y);

            // Bounds check
            if (ChunkCoord.X >= 0 && ChunkCoord.X < ChunkGridSize.X &&
                ChunkCoord.Y >= 0 && ChunkCoord. Y < ChunkGridSize.Y)
            {
                int32 ChunkIndex = ChunkCoord.Y * ChunkGridSize.X + ChunkCoord.X;
                ChunkState[ChunkIndex] = true;  // Mark as room chunk
            }
        }
    }
}

void UChunkyRoomGenerator::AddRandomProtrusionChunked()
{
    // Pick random edge
    int32 EdgeIndex = RandomStream.RandRange(0, 3);
    EWallEdge Edge = (EWallEdge)EdgeIndex;

    // Pick random protrusion dimensions (IN CHUNKS, minimum 2)
    int32 ProtrusionWidthChunks = RandomStream. RandRange(MinProtrusionSizeChunks, MaxProtrusionSizeChunks);
    int32 ProtrusionDepthChunks = RandomStream. RandRange(MinProtrusionSizeChunks, MaxProtrusionSizeChunks);

    // Calculate protrusion rectangle in chunk coordinates
    FIntPoint StartChunks;
    FIntPoint SizeChunks;

    switch (Edge)
    {
        case EWallEdge::North:  // Extend upward (+Y in chunk space)
        {
            int32 EdgeLength = BaseRoomSizeChunks. X;
            int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidthChunks));

            StartChunks = FIntPoint(BaseRoomStartChunks.X + Position,
                                    BaseRoomStartChunks. Y + BaseRoomSizeChunks. Y);
            SizeChunks = FIntPoint(ProtrusionWidthChunks, ProtrusionDepthChunks);
            break;
        }

        case EWallEdge::South:   // Extend downward (-Y)
        {
            int32 EdgeLength = BaseRoomSizeChunks.X;
            int32 Position = RandomStream. RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidthChunks));

            StartChunks = FIntPoint(BaseRoomStartChunks.X + Position,
                                    BaseRoomStartChunks.Y - ProtrusionDepthChunks);
            SizeChunks = FIntPoint(ProtrusionWidthChunks, ProtrusionDepthChunks);
            break;
        }

        case EWallEdge::East:  // Extend right (+X)
        {
            int32 EdgeLength = BaseRoomSizeChunks.Y;
            int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidthChunks));

            StartChunks = FIntPoint(BaseRoomStartChunks. X + BaseRoomSizeChunks.X,
                                    BaseRoomStartChunks.Y + Position);
            SizeChunks = FIntPoint(ProtrusionDepthChunks, ProtrusionWidthChunks);
            break;
        }

        case EWallEdge::West:  // Extend left (-X)
        {
            int32 EdgeLength = BaseRoomSizeChunks.Y;
            int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidthChunks));

            StartChunks = FIntPoint(BaseRoomStartChunks.X - ProtrusionDepthChunks,
                                    BaseRoomStartChunks.Y + Position);
            SizeChunks = FIntPoint(ProtrusionDepthChunks, ProtrusionWidthChunks);
            break;
        }

        default:
            return;
    }

    // Clamp to chunk grid bounds
    int32 ClampedStartX = FMath:: Clamp(StartChunks. X, 0, ChunkGridSize.X - 1);
    int32 ClampedStartY = FMath::Clamp(StartChunks.Y, 0, ChunkGridSize.Y - 1);
    int32 ClampedWidth = FMath::Min(SizeChunks.X, ChunkGridSize.X - ClampedStartX);
    int32 ClampedHeight = FMath::Min(SizeChunks.Y, ChunkGridSize.Y - ClampedStartY);

    // Only add if valid size (minimum 2×2 chunks)
    if (ClampedWidth >= MinProtrusionSizeChunks && ClampedHeight >= MinProtrusionSizeChunks)
    {
        MarkChunkRectangle(ClampedStartX, ClampedStartY, ClampedWidth, ClampedHeight);

        UE_LOG(LogTemp, Verbose, TEXT("    Added protrusion on edge %d:  Start(%d,%d) chunks, Size(%d×%d) chunks = (%d×%d) cells"),
            EdgeIndex, ClampedStartX, ClampedStartY, ClampedWidth, ClampedHeight,
            ClampedWidth * 2, ClampedHeight * 2);
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("    Protrusion too small after clamping, skipped"));
    }
}

void UChunkyRoomGenerator:: ConvertChunksToCells()
{
    UE_LOG(LogTemp, Verbose, TEXT("  Converting chunks to cells..."));

    int32 ConvertedChunks = 0;

    // For each chunk marked as "room", mark all 4 cells (2×2) as ECT_Custom
    for (int32 ChunkY = 0; ChunkY < ChunkGridSize.Y; ++ChunkY)
    {
        for (int32 ChunkX = 0; ChunkX < ChunkGridSize.X; ++ChunkX)
        {
            int32 ChunkIndex = ChunkY * ChunkGridSize.X + ChunkX;

            if (ChunkState[ChunkIndex])  // This chunk is part of the room
            {
                // Calculate starting cell coordinate for this chunk
                int32 CellStartX = ChunkX * 2;
                int32 CellStartY = ChunkY * 2;

                // Mark all 4 cells in this chunk as ECT_Custom
                for (int32 Y = 0; Y < 2; ++Y)
                {
                    for (int32 X = 0; X < 2; ++X)
                    {
                        FIntPoint CellCoord(CellStartX + X, CellStartY + Y);

                        if (IsValidGridCoordinate(CellCoord))
                        {
                            SetCellState(CellCoord, EGridCellType::ECT_Custom);
                        }
                    }
                }

                ConvertedChunks++;
            }
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("  Converted %d chunks to %d cells"), ConvertedChunks, ConvertedChunks * 4);
}

FIntPoint UChunkyRoomGenerator::ChunkToCell(FIntPoint ChunkCoord) const
{
    return ChunkCoord * 2;
}

FIntPoint UChunkyRoomGenerator::CellToChunk(FIntPoint CellCoord) const
{
    return FIntPoint(CellCoord. X / 2, CellCoord.Y / 2);
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

FIntPoint UChunkyRoomGenerator::GetDirectionOffset(EWallEdge Direction) const
{
	switch (Direction)
	{
	case EWallEdge::North:  return FIntPoint(1, 0);   // +X (North)
	case EWallEdge::South:  return FIntPoint(-1, 0);  // -X (South)
	case EWallEdge:: East:   return FIntPoint(0, 1);   // +Y (East)
	case EWallEdge::West:   return FIntPoint(0, -1);  // -Y (West)
	default:  return FIntPoint:: ZeroValue;
	}
}

FVector UChunkyRoomGenerator:: CalculateWallPositionForSegment(EWallEdge Direction, FIntPoint StartCell,
    int32 ModuleFootprint, float NorthOffset, float SouthOffset, float EastOffset, float WestOffset) const
{
    FVector Position = FVector:: ZeroVector;

    // StartCell is a FLOOR cell at the edge
    // Calculate center of the module span
    float HalfFootprint = (ModuleFootprint - 1) * 0.5f;

    // Coordinate system: +X=North, +Y=East
    // Wall mesh Y-axis = length of wall
    // Offsets are applied PERPENDICULAR to the wall's length
    
    switch (Direction)
    {
        case EWallEdge::North:
            // Wall at NORTH edge (+X boundary)
            // Wall extends along Y-axis (East-West)
            // Offset is applied on X-axis (perpendicular to wall length)
            Position. X = (StartCell.X + 1) * CellSize + NorthOffset;  // ← Offset on X-axis
            Position.Y = (StartCell.Y + HalfFootprint) * CellSize + (CellSize * 0.5f);
            break;

        case EWallEdge:: South:
            // Wall at SOUTH edge (-X boundary)
            // Wall extends along Y-axis (East-West)
            // Offset is applied on X-axis (perpendicular to wall length)
            Position.X = StartCell.X * CellSize + SouthOffset;  // ← Offset on X-axis
            Position.Y = (StartCell.Y + HalfFootprint) * CellSize + (CellSize * 0.5f);
            break;

        case EWallEdge::East:
            // Wall at EAST edge (+Y boundary)
            // Wall extends along X-axis (North-South)
            // Offset is applied on Y-axis (perpendicular to wall length)
            Position.X = (StartCell.X + HalfFootprint) * CellSize + (CellSize * 0.5f);
            Position.Y = (StartCell.Y + 1) * CellSize + EastOffset;  // ← Offset on Y-axis
            break;

        case EWallEdge:: West:
            // Wall at WEST edge (-Y boundary)
            // Wall extends along X-axis (North-South)
            // Offset is applied on Y-axis (perpendicular to wall length)
            Position.X = (StartCell.X + HalfFootprint) * CellSize + (CellSize * 0.5f);
            Position.Y = StartCell.Y * CellSize + WestOffset;  // ← Offset on Y-axis
            break;
    }

    Position.Z = 0.0f;  // Floor level

    return Position;
}

TArray<FIntPoint> UChunkyRoomGenerator::GetPerimeterCellsForEdge(EWallEdge Edge) const
{
	TArray<FIntPoint> EdgeCells;

	FIntPoint EdgeDirection = GetDirectionOffset(Edge);
    
	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			FIntPoint FloorCell(X, Y);
            
			// ✅ CHANGE THIS: Use ECT_Custom instead of ECT_FloorMesh
			if (GetCellState(FloorCell) != EGridCellType::ECT_Custom)
				continue;

			FIntPoint Neighbor = FloorCell + EdgeDirection;
            
			bool bIsEdge = false;
            
			if (! IsValidGridCoordinate(Neighbor))
			{
				bIsEdge = true;  // Grid boundary edge
			}
			else if (GetCellState(Neighbor) == EGridCellType::ECT_Void)
			{
				// ✅ ADD THIS: Skip if neighbor void cell has a corner
				if (CornerOccupiedCells.Contains(Neighbor))
				{
					continue;  // Skip this edge - corner occupies the void cell
				}
                
				bIsEdge = true;  // Void edge
			}
            
			if (bIsEdge)
			{
				EdgeCells.Add(FloorCell);
			}
		}
	}

	// Sort cells (existing code...)
	if (Edge == EWallEdge::North || Edge == EWallEdge::South)
	{
		EdgeCells.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.Y < B.Y; });
	}
	else
	{
		EdgeCells. Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X < B.X; });
	}

	UE_LOG(LogTemp, Verbose, TEXT("  GetPerimeterCellsForEdge(%s): Found %d edge cells"), 
		*UEnum::GetValueAsString(Edge), EdgeCells.Num());

	return EdgeCells;
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
                // Check if VOID cells are consecutive
                bool bCellsConsecutive = true;
                
                for (int32 i = 1; i < Module.Y_AxisFootprint; ++i)
                {
                	FIntPoint CurrentCellPos = EdgeCells[CurrentCell + i - 1];
                	FIntPoint NextCellPos = EdgeCells[CurrentCell + i];
                    
                	// Check adjacency based on edge direction
                	// Coordinate system: +X=North, +Y=East
                	bool bAdjacent = false;
                    
                	if (Edge == EWallEdge::North || Edge == EWallEdge:: South)
                	{
                		// North/South walls extend East-West (along Y-axis)
                		// Y should increment by 1, X should stay same
                		bAdjacent = (NextCellPos. Y == CurrentCellPos.Y + 1) && (NextCellPos.X == CurrentCellPos.X);
                	}
                	else // East or West
                	{
                		// East/West walls extend North-South (along X-axis)
                		// X should increment by 1, Y should stay same
                		bAdjacent = (NextCellPos.X == CurrentCellPos.X + 1) && (NextCellPos.Y == CurrentCellPos.Y);
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
            UE_LOG(LogTemp, Warning, TEXT("    No wall module fits at void cell %d (remaining:  %d)"), 
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

        // Get starting VOID cell for this module
        FIntPoint StartVoidCell = EdgeCells[CurrentCell];

        // Calculate wall position using void-based logic
        FVector WallPosition = CalculateWallPositionForSegment(
            Edge,
            StartVoidCell,  // ← Now passing void cell, not floor cell
            BestModule->Y_AxisFootprint,
            NorthOffset,
            SouthOffset,
            EastOffset,
            WestOffset
        );

        // Create transform
        FTransform BaseTransform(WallRotation, WallPosition, FVector::OneVector);

        // Store segment for middle/top spawning
        FGeneratorWallSegment Segment;
        Segment.Edge = Edge;
        Segment. StartCell = CurrentCell;
        Segment. SegmentLength = BestModule->Y_AxisFootprint;
        Segment.BaseTransform = BaseTransform;
        Segment.BaseMesh = BaseMesh;
        Segment.WallModule = BestModule;

        PlacedBaseWallSegments.Add(Segment);

        UE_LOG(LogTemp, VeryVerbose, TEXT("    Placed %dY module at void cell (%d,%d)"),
            BestModule->Y_AxisFootprint, StartVoidCell.X, StartVoidCell.Y);

        // Advance by module footprint
        CurrentCell += BestModule->Y_AxisFootprint;
    }
}

bool UChunkyRoomGenerator::IsVoidCornerCell(FIntPoint Cell, TArray<EWallEdge>& OutAdjacentFloorEdges) const
{
	OutAdjacentFloorEdges. Empty();
    
	// Cell must be void
	if (GetCellState(Cell) != EGridCellType::ECT_Void)
		return false;
    
	// Check all four cardinal directions for Custom neighbors
	TArray<EWallEdge> DirectionsToCheck = {
		EWallEdge::North,
		EWallEdge::South,
		EWallEdge::East,
		EWallEdge::West
	};
    
	for (EWallEdge Edge : DirectionsToCheck)
	{
		FIntPoint Direction = GetDirectionOffset(Edge);
		FIntPoint Neighbor = Cell + Direction;
        
		// Check if neighbor is Custom (room area)
		if (IsValidGridCoordinate(Neighbor))
		{
			EGridCellType NeighborState = GetCellState(Neighbor);
			if (NeighborState == EGridCellType::ECT_Custom)
			{
				OutAdjacentFloorEdges.Add(Edge);
			}
		}
	}
    
	// A corner has exactly 2 adjacent Custom neighbors
	if (OutAdjacentFloorEdges.Num() != 2)
		return false;
    
	// Check if the two Custom sides are ADJACENT (not opposite)
	EWallEdge Edge1 = OutAdjacentFloorEdges[0];
	EWallEdge Edge2 = OutAdjacentFloorEdges[1];
    
	// Opposite pairs (NOT corners - these are thin corridors):
	if ((Edge1 == EWallEdge::North && Edge2 == EWallEdge::South) ||
		(Edge1 == EWallEdge::South && Edge2 == EWallEdge::North) ||
		(Edge1 == EWallEdge::East && Edge2 == EWallEdge::West) ||
		(Edge1 == EWallEdge::West && Edge2 == EWallEdge::East))
	{
		return false;  // Opposite sides = corridor, not corner
	}
    
	// Adjacent sides = valid interior corner
	return true;
}

void UChunkyRoomGenerator::GenerateInteriorCorners()
{
    if (!WallData || !WallData->DefaultCornerMesh. IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("GenerateInteriorCorners - No corner mesh defined"));
        return;
    }

    TSoftObjectPtr<UStaticMesh> CornerMeshPtr = WallData->DefaultCornerMesh;
    int32 CornersPlaced = 0;

    UE_LOG(LogTemp, Log, TEXT("GenerateInteriorCorners - Scanning for interior corners..."));

    // Scan all VOID cells for interior corners
    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            FIntPoint Cell(X, Y);

            // Check if this void cell is an interior corner
            TArray<EWallEdge> AdjacentFloorEdges;
            if (! IsVoidCornerCell(Cell, AdjacentFloorEdges))
                continue;

            // This is an interior corner! 
            UE_LOG(LogTemp, Verbose, TEXT("  Found interior corner at VOID cell (%d,%d)"),
                Cell.X, Cell.Y);

            // Calculate corner position (center of VOID cell)
            FVector CornerPosition;
            CornerPosition.X = Cell.X * CellSize + (CellSize * 0.5f);
            CornerPosition.Y = Cell.Y * CellSize + (CellSize * 0.5f);
            CornerPosition.Z = 0.0f;

            // Create transform
            FTransform CornerTransform(FRotator::ZeroRotator, CornerPosition, FVector:: OneVector);

            // Create FPlacedCornerInfo
            FPlacedCornerInfo CornerInfo;
            CornerInfo.Corner = ECornerPosition::None;  // Interior corner (not a standard corner)
            CornerInfo.Transform = CornerTransform;
            CornerInfo. CornerMesh = CornerMeshPtr;

            PlacedCornerMeshes.Add(CornerInfo);
            
            // Mark this void cell as corner-occupied (block from wall placement)
            CornerOccupiedCells. Add(Cell);
            
            CornersPlaced++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GenerateInteriorCorners - Placed %d interior corners"), CornersPlaced);
}
