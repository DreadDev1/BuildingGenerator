// Fill out your copyright notice in the Description page of Project Settings. 

#include "Generators/Room/RandWalkGenerator.h"

#include "Data/Room/FloorData.h"
#include "Utilities/Generation/RoomGenerationHelpers.h"

// Define static direction vectors
const TArray<FIntPoint> URandWalkGenerator::DirectionVectors = {
	FIntPoint(0, 1),   // 0: North (+Y)
	FIntPoint(1, 0),   // 1: East (+X)
	FIntPoint(0, -1),  // 2: South (-Y)
	FIntPoint(-1, 0)   // 3: West (-X)
};

void URandWalkGenerator:: CreateGrid()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("RandWalkRoomGenerator:: CreateGrid - Generator not initialized!"));
		return;
	}

	// Initialize grid with all empty cells (base implementation)
	Super::CreateGrid();

	UE_LOG(LogTemp, Log, TEXT("RandWalkRoomGenerator::CreateGrid - Starting random walk generation"));

	// Execute random walk algorithm
	ExecuteRandomWalk();

	// ✅ CRITICAL: Remove islands FIRST (before smoothing can create new ones)
	if (bRemoveIslands)
	{
		UE_LOG(LogTemp, Log, TEXT("  Removing disconnected regions (pre-smoothing)..."));
		RemoveDisconnectedRegions();
	}

	// Post-processing: Smoothing
	if (SmoothingPasses > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("  Smoothing edges (%d iterations)..."), SmoothingPasses);
		SmoothGridEdges(SmoothingPasses);
	}

	// ✅ CRITICAL: Remove islands AGAIN (in case smoothing created new disconnected cells)
	if (bRemoveIslands)
	{
		UE_LOG(LogTemp, Log, TEXT("  Removing disconnected regions (post-smoothing)..."));
		RemoveDisconnectedRegions();
	}

	// Validate result
	if (! ValidateMinimumSize())
	{
		UE_LOG(LogTemp, Warning, TEXT("RandWalkRoomGenerator::CreateGrid - Generated room is too small!"));
	}

	// Log statistics
	TArray<FIntPoint> OccupiedCells = GetOccupiedCells();
	int32 TotalCells = GridSize.X * GridSize.Y;
	float ActualFillRatio = (float)OccupiedCells.Num() / (float)TotalCells;

	UE_LOG(LogTemp, Log, TEXT("RandWalkRoomGenerator::CreateGrid - Complete:  %d/%d cells filled (%.2f%%)"),
		OccupiedCells.Num(), TotalCells, ActualFillRatio * 100.0f);
}

bool URandWalkGenerator::GenerateFloor()
{
	if (! bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("RandWalkRoomGenerator:: GenerateFloor - Generator not initialized! "));
		return false;
	}

	if (! RoomData || !RoomData->FloorStyleData)
	{
		UE_LOG(LogTemp, Error, TEXT("RandWalkRoomGenerator::GenerateFloor - FloorData not assigned!"));
		return false;
	}

	// Load FloorData
	UFloorData* FloorStyleData = RoomData->FloorStyleData. LoadSynchronous();
	if (!FloorStyleData)
	{
		UE_LOG(LogTemp, Error, TEXT("RandWalkRoomGenerator::GenerateFloor - Failed to load FloorStyleData!"));
		return false;
	}

	if (FloorStyleData->FloorTilePool. Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RandWalkRoomGenerator::GenerateFloor - No floor meshes in pool!"));
		return false;
	}

	// Clear previous placement data
	ClearPlacedFloorMeshes();

	int32 FloorLargeTilesPlaced = 0;
	int32 FloorMediumTilesPlaced = 0;
	int32 FloorSmallTilesPlaced = 0;
	int32 FloorFillerTilesPlaced = 0;

	UE_LOG(LogTemp, Log, TEXT("RandWalkRoomGenerator::GenerateFloor - Starting irregular floor generation"));

	// Get only occupied cells (created by random walk)
	TArray<FIntPoint> OccupiedCells = GetOccupiedCells();
	UE_LOG(LogTemp, Log, TEXT("  Found %d occupied cells to fill"), OccupiedCells.Num());

	// TEMPORARY WORKAROUND: Mark all occupied cells as Empty so base algorithm can fill them
	// This preserves the irregular shape while allowing tile placement
	TArray<EGridCellType> OriginalGridState = GridState;

	// Reset occupied cells to Empty (so tile placement algorithm can fill them)
	for (const FIntPoint& Cell : OccupiedCells)
	{
		SetCellState(Cell, EGridCellType::ECT_Empty);
	}

	// PHASE 1: FORCED PLACEMENTS (if any)
	int32 ForcedCount = ExecuteForcedPlacements();
	UE_LOG(LogTemp, Log, TEXT("  Phase 1: Placed %d forced meshes"), ForcedCount);

	// PHASE 2: GREEDY FILL (only within irregular shape)
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

	// PHASE 3: GAP FILL (Fill remaining empty cells)
	int32 GapFillCount = FillRemainingGaps(FloorMeshes, FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	UE_LOG(LogTemp, Log, TEXT("  Phase 3: Filled %d remaining gaps"), GapFillCount);

	// Restore original boundary (mark cells outside irregular shape as truly empty)
	RestoreIrregularBoundary(OriginalGridState);

	// FINAL STATISTICS
	int32 RemainingEmpty = 0;
	for (const FIntPoint& Cell :  OccupiedCells)
	{
		if (GetCellState(Cell) == EGridCellType::ECT_Empty)
		{
			RemainingEmpty++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("RandWalkRoomGenerator::GenerateFloor - Complete"));
	UE_LOG(LogTemp, Log, TEXT("  Total meshes placed: %d"), PlacedFloorMeshes.Num());
	UE_LOG(LogTemp, Log, TEXT("  Large:  %d, Medium: %d, Small: %d, Filler: %d"),
		FloorLargeTilesPlaced, FloorMediumTilesPlaced, FloorSmallTilesPlaced, FloorFillerTilesPlaced);
	UE_LOG(LogTemp, Log, TEXT("  Unfilled cells in irregular shape: %d"), RemainingEmpty);

	return true;
}

void URandWalkGenerator:: SetRandomWalkParams(float InFillRatio, float InBranchChance, 
                                              float InDirChangeChance, int32 InMaxWalkers, int32 InSmoothingPasses, bool bInRemoveIslands, int32 InSeed)
{
	TargetFillRatio = FMath::Clamp(InFillRatio, 0.1f, 0.95f);
	BranchingChance = FMath:: Clamp(InBranchChance, 0.0f, 1.0f);
	DirectionChangeChance = FMath:: Clamp(InDirChangeChance, 0.0f, 1.0f);
	MaxActiveWalkers = FMath::Max(1, InMaxWalkers);
	SmoothingPasses = FMath::Max(0, InSmoothingPasses);
	bRemoveIslands = bInRemoveIslands;
	RandomSeed = InSeed;

	// Initialize random stream
	RandomStream.Initialize(RandomSeed);

	UE_LOG(LogTemp, Log, TEXT("RandWalkRoomGenerator:  Parameters set - FillRatio: %.2f, Branch: %.2f, DirChange:%.2f, MaxWalkers:%d, Smooth:%d, Seed:%d"),
		TargetFillRatio, BranchingChance, DirectionChangeChance, MaxActiveWalkers, SmoothingPasses, RandomSeed);
}

void URandWalkGenerator::SetIrregularWallParams(bool bEnabled, float Wall2Chance, float Wall4Chance,
int32 MinSegmentLen, int32 MaxSegmentLen)
{
	bUseIrregularWalls = bEnabled;
	Prob2CellWall = FMath::Clamp(Wall2Chance, 0.0f, 1.0f);
	Prob4CellWall = FMath::Clamp(Wall4Chance, 0.0f, 1.0f);
	MinWallSegmentLength = FMath::Max(2, MinSegmentLen);
	MaxWallSegmentLength = FMath::Max(MinWallSegmentLength, MaxSegmentLen);

	// Normalize probabilities (only 2Y and 4Y)
	float Total = Prob2CellWall + Prob4CellWall;
	if (Total > 0.0f)
	{
		Prob2CellWall /= Total;
		Prob4CellWall /= Total;
	}
	else
	{
		// Default to 50/50 split
		Prob2CellWall = 0.5f;
		Prob4CellWall = 0.5f;
	}

	UE_LOG(LogTemp, Log, TEXT("RandWalkGenerator:  Irregular walls enabled - 2Y: %.2f (200cm), 4Y: %.2f (400cm)"), 
		Prob2CellWall, Prob4CellWall);
	UE_LOG(LogTemp, Log, TEXT("  Segment length:  %d-%d cells"), MinWallSegmentLength, MaxWallSegmentLength);
}

int32 URandWalkGenerator::GetRandomWallDepth()
{
	float Roll = RandomStream.FRand();

	if (Roll < Prob2CellWall) return 2;  // 2-cell wall (200cm)
	else return 4;  // 4-cell wall (400cm)
}

void URandWalkGenerator::GenerateIrregularWalls()
{
	if (!bUseIrregularWalls)
	{
		// Fallback to standard 2-cell uniform walls
		MarkStandardWalls();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RandWalkGenerator::GenerateIrregularWalls - Creating 2Y/4Y variable-depth walls"));

	// Process each edge separately
	GenerateIrregularEdgeWalls(EWallEdge:: North);
	GenerateIrregularEdgeWalls(EWallEdge::South);
	GenerateIrregularEdgeWalls(EWallEdge::East);
	GenerateIrregularEdgeWalls(EWallEdge::West);

	UE_LOG(LogTemp, Log, TEXT("  Irregular walls complete"));
}

void URandWalkGenerator::GenerateIrregularEdgeWalls(EWallEdge Edge)
{
	// Get edge-specific parameters
	int32 EdgeLength = (Edge == EWallEdge:: North || Edge == EWallEdge:: South) ? GridSize.X : GridSize.Y;
    
	int32 CurrentPos = 0;
	int32 SegmentsCreated = 0;
    
	while (CurrentPos < EdgeLength)
	{
		// Determine random segment length
		int32 SegmentLength = RandomStream.RandRange(MinWallSegmentLength, MaxWallSegmentLength);
		SegmentLength = FMath:: Min(SegmentLength, EdgeLength - CurrentPos);

		// Get random wall depth for this segment (2Y or 4Y)
		int32 WallDepth = GetRandomWallDepth();

		// Find starting cell on this edge
		FIntPoint StartCell = GetEdgeCellPosition(Edge, CurrentPos);

		// Extend wall segment
		ExtendWallSegment(StartCell, Edge, WallDepth, SegmentLength);
        
		UE_LOG(LogTemp, Verbose, TEXT("    Edge %d: Pos %d, Length %d cells, Depth %dY (%dcm)"), 
			(int32)Edge, CurrentPos, SegmentLength, WallDepth, WallDepth * 100);

		CurrentPos += SegmentLength;
		SegmentsCreated++;
	}

	UE_LOG(LogTemp, Log, TEXT("  Edge %d complete - %d segments created"), (int32)Edge, SegmentsCreated);
}

void URandWalkGenerator::ExtendWallSegment(FIntPoint StartCell, EWallEdge Edge, int32 Depth, int32 Length)
{
	// Determine direction to extend outward from edge (perpendicular to wall)
	FIntPoint ExtendDir = GetEdgeOutwardDirection(Edge);
    
	// Direction along the length of the wall
	FIntPoint LengthDir = GetEdgeLengthDirection(Edge);

	// Extend outward by Depth cells, along Length cells
	for (int32 LengthOffset = 0; LengthOffset < Length; ++LengthOffset)
	{
		for (int32 DepthOffset = 0; DepthOffset < Depth; ++DepthOffset)
		{
			FIntPoint WallCell = StartCell + (LengthDir * LengthOffset) + (ExtendDir * DepthOffset);

			if (IsValidGridCoordinate(WallCell))
			{
				// Only mark as wall if not already floor (preserve interior)
				if (GetCellState(WallCell) != EGridCellType::ECT_FloorMesh)
				{
					SetCellState(WallCell, EGridCellType::ECT_WallMesh);
				}
			}
		}
	}
}

FIntPoint URandWalkGenerator::GetEdgeOutwardDirection(EWallEdge Edge) const
{
	switch (Edge)
	{
	case EWallEdge::North:  return FIntPoint(0, 1);   // Extend upward (+Y)
	case EWallEdge::South:  return FIntPoint(0, -1);  // Extend downward (-Y)
	case EWallEdge::East:   return FIntPoint(1, 0);   // Extend right (+X)
	case EWallEdge::West:   return FIntPoint(-1, 0);  // Extend left (-X)
	default: return FIntPoint:: ZeroValue;
	}
}

FIntPoint URandWalkGenerator::GetEdgeLengthDirection(EWallEdge Edge) const
{
	switch (Edge)
	{
	case EWallEdge::North:  
	case EWallEdge::South:   return FIntPoint(1, 0);   // Run along X-axis
	case EWallEdge::East:  
	case EWallEdge::West:   return FIntPoint(0, 1);   // Run along Y-axis
	default: return FIntPoint::ZeroValue;
	}
}

FIntPoint URandWalkGenerator::GetEdgeCellPosition(EWallEdge Edge, int32 Offset) const
{
	switch (Edge)
	{
	case EWallEdge::North:   return FIntPoint(Offset, GridSize.Y - 1);
	case EWallEdge:: South:  return FIntPoint(Offset, 0);
	case EWallEdge::East:   return FIntPoint(GridSize.X - 1, Offset);
	case EWallEdge::West:   return FIntPoint(0, Offset);
	default: return FIntPoint::ZeroValue;
	}
}

bool URandWalkGenerator::IsAdjacentToFloor(FIntPoint Cell, EWallEdge Edge) const
{
	// Check if the cell on the interior side is floor
	FIntPoint InwardDir = GetEdgeOutwardDirection(Edge) * -1;
	FIntPoint InteriorCell = Cell + InwardDir;

	if (IsValidGridCoordinate(InteriorCell)) return GetCellState(InteriorCell) == EGridCellType::ECT_FloorMesh;

	return false;
}

void URandWalkGenerator::MarkStandardWalls()
{
	// Fallback:  Create uniform 2-cell walls around entire perimeter
	UE_LOG(LogTemp, Log, TEXT("  Creating standard 2-cell uniform walls"));

	TArray<FIntPoint> BoundaryCells = GetFloorBoundaryCells();

	for (const FIntPoint& Cell : BoundaryCells)
	{
		// Mark cell and one outward cell as wall
		SetCellState(Cell, EGridCellType::ECT_WallMesh);

		// Check each direction and extend outward by 1 cell
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			FIntPoint OutwardCell = Cell + GetDirectionVector(Dir);
            
			if (IsValidGridCoordinate(OutwardCell) && 
				GetCellState(OutwardCell) == EGridCellType::ECT_Empty)
			{
				SetCellState(OutwardCell, EGridCellType::ECT_WallMesh);
			}
		}
	}
}

bool URandWalkGenerator::IsFloorBoundaryCell(FIntPoint Cell) const
{
	// Check all 4 cardinal directions
	for (int32 Dir = 0; Dir < 4; ++Dir)
	{
		FIntPoint Neighbor = Cell + GetDirectionVector(Dir);

		if (! IsValidGridCoordinate(Neighbor) || GetCellState(Neighbor) == EGridCellType::ECT_Empty) return true;  // Adjacent to empty = boundary
	}

	return false;
}

TArray<FIntPoint> URandWalkGenerator::GetFloorBoundaryCells() const
{
	TArray<FIntPoint> BoundaryCells;

	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);

			if (GetCellState(Cell) == EGridCellType::ECT_FloorMesh)
			{
				if (IsFloorBoundaryCell(Cell)) BoundaryCells.Add(Cell);
			}
		}
	}

	return BoundaryCells;
}

#pragma region Random Walk Algorithm

void URandWalkGenerator::ExecuteRandomWalk()
{
	// Start from center of grid
	FIntPoint StartPos(GridSize.X / 2, GridSize.Y / 2);
	SetCellState(StartPos, EGridCellType::ECT_FloorMesh);

	// Create initial walker
	TArray<FWalkerState> ActiveWalkers;
	ActiveWalkers.Add(FWalkerState(StartPos, RandomStream.RandRange(0, 3)));

	// Target number of cells to fill
	int32 TotalCells = GridSize.X * GridSize.Y;
	int32 TargetCells = FMath::RoundToInt(TotalCells * TargetFillRatio);
	int32 FilledCells = 1;

	UE_LOG(LogTemp, Log, TEXT("  Random walk starting - Target:  %d/%d cells"), TargetCells, TotalCells);

	// Walk until target reached or no more active walkers
	int32 StuckCounter = 0;
	const int32 MaxStuckIterations = 100;

	while (FilledCells < TargetCells && ActiveWalkers.Num() > 0 && StuckCounter < MaxStuckIterations)
	{
		bool bAnyProgress = false;

		// Process each walker
		for (int32 i = ActiveWalkers.Num() - 1; i >= 0; --i)
		{
			if (ProcessWalkerStep(ActiveWalkers[i], ActiveWalkers))
			{
				FilledCells++;
				bAnyProgress = true;
			}
		}

		// Remove stuck walkers (haven't moved in a while)
		for (int32 i = ActiveWalkers.Num() - 1; i >= 0; --i)
		{
			if (ActiveWalkers[i]. StepsTaken > 20)  // Stuck threshold
			{
				TArray<int32> ValidDirs = GetValidDirections(ActiveWalkers[i]. Position);
				if (ValidDirs.Num() == 0)
				{
					ActiveWalkers.RemoveAt(i);
				}
			}
		}

		if (! bAnyProgress)
		{
			StuckCounter++;
		}
		else
		{
			StuckCounter = 0;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("  Random walk complete - Filled:  %d cells, Active walkers: %d"), FilledCells, ActiveWalkers. Num());
}

bool URandWalkGenerator::ProcessWalkerStep(FWalkerState& Walker, TArray<FWalkerState>& ActiveWalkers)
{
	// Get valid directions from current position
	TArray<int32> ValidDirections = GetValidDirections(Walker.Position);

	if (ValidDirections. Num() == 0)
	{
		// Walker is stuck
		Walker.StepsTaken++;
		return false;
	}

	// Decide if walker should change direction
	int32 ChosenDirection = Walker.Direction;

	if (RandomStream.FRand() < DirectionChangeChance || ! ValidDirections.Contains(Walker.Direction))
	{
		// Pick random valid direction
		ChosenDirection = ValidDirections[RandomStream.RandRange(0, ValidDirections.Num() - 1)];
	}

	// Try to move in chosen direction
	if (TryMoveWalker(Walker, ChosenDirection))
	{
		// Successfully moved
		Walker.StepsTaken = 0;  // Reset stuck counter

		// Check for branching
		if (ActiveWalkers.Num() < MaxActiveWalkers && RandomStream.FRand() < BranchingChance)
		{
			CreateBranch(Walker. Position, ActiveWalkers);
		}

		return true;
	}

	Walker.StepsTaken++;
	return false;
}

bool URandWalkGenerator::TryMoveWalker(FWalkerState& Walker, int32 Direction)
{
	FIntPoint NewPos = Walker.Position + GetDirectionVector(Direction);

	// Check if valid and empty
	if (IsValidGridCoordinate(NewPos) && GetCellState(NewPos) == EGridCellType::ECT_Empty)
	{
		// Move walker
		Walker.Position = NewPos;
		Walker.Direction = Direction;
		SetCellState(NewPos, EGridCellType:: ECT_FloorMesh);
		return true;
	}

	return false;
}

TArray<int32> URandWalkGenerator::GetValidDirections(FIntPoint Position) const
{
	TArray<int32> ValidDirs;

	for (int32 Dir = 0; Dir < 4; ++Dir)
	{
		FIntPoint NewPos = Position + GetDirectionVector(Dir);

		if (IsValidGridCoordinate(NewPos) && GetCellState(NewPos) == EGridCellType::ECT_Empty)
		{
			ValidDirs.Add(Dir);
		}
	}

	return ValidDirs;
}

void URandWalkGenerator:: CreateBranch(FIntPoint Position, TArray<FWalkerState>& ActiveWalkers)
{
	TArray<int32> ValidDirs = GetValidDirections(Position);

	if (ValidDirs. Num() > 0)
	{
		int32 BranchDir = ValidDirs[RandomStream.RandRange(0, ValidDirs. Num() - 1)];
		ActiveWalkers.Add(FWalkerState(Position, BranchDir));

		UE_LOG(LogTemp, Verbose, TEXT("    Created branch at (%d, %d) - Direction: %d"), Position.X, Position.Y, BranchDir);
	}
}

#pragma endregion

#pragma region Post-Processing

void URandWalkGenerator::SmoothGridEdges(int32 Iterations)
{
	for (int32 Iter = 0; Iter < Iterations; ++Iter)
	{
		TArray<EGridCellType> NewGridState = GridState;

		for (int32 X = 0; X < GridSize.X; ++X)
		{
			for (int32 Y = 0; Y < GridSize.Y; ++Y)
			{
				FIntPoint Cell(X, Y);
				int32 OccupiedNeighbors = CountOccupiedNeighbors(Cell);

				EGridCellType CurrentState = GetCellState(Cell);
				int32 Index = URoomGenerationHelpers::CoordinateToIndex(Cell, GridSize. X);

				// Cellular automata rules (4-5 rule for smooth caves)
				if (CurrentState == EGridCellType::ECT_FloorMesh)
				{
					// Stay occupied if 4+ neighbors occupied
					if (OccupiedNeighbors < 4)
					{
						NewGridState[Index] = EGridCellType::ECT_Empty;
					}
				}
				else
				{
					// Become occupied if 5+ neighbors occupied
					if (OccupiedNeighbors >= 5)
					{
						NewGridState[Index] = EGridCellType::ECT_FloorMesh;
					}
				}
			}
		}

		GridState = NewGridState;
	}
}

void URandWalkGenerator:: RemoveDisconnectedRegions()
{
	TArray<FIntPoint> OccupiedCells = GetOccupiedCells();

	if (OccupiedCells. Num() == 0)
		return;

	// Find all connected regions using flood fill
	TSet<FIntPoint> Visited;
	TArray<TSet<FIntPoint>> Regions;

	for (const FIntPoint& Cell : OccupiedCells)
	{
		if (! Visited.Contains(Cell))
		{
			TSet<FIntPoint> Region;
			FloodFill(Cell, Region);

			Visited. Append(Region);
			Regions.Add(Region);
		}
	}

	if (Regions.Num() <= 1)
	{
		UE_LOG(LogTemp, Log, TEXT("    No disconnected regions found"));
		return;
	}

	// Find largest region
	int32 LargestRegionIndex = 0;
	int32 LargestRegionSize = 0;

	for (int32 i = 0; i < Regions.Num(); ++i)
	{
		if (Regions[i]. Num() > LargestRegionSize)
		{
			LargestRegionSize = Regions[i].Num();
			LargestRegionIndex = i;
		}
	}

	// Clear all cells except largest region
	for (int32 i = 0; i < Regions.Num(); ++i)
	{
		if (i != LargestRegionIndex)
		{
			for (const FIntPoint& Cell : Regions[i])
			{
				SetCellState(Cell, EGridCellType::ECT_Empty);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("    Removed %d disconnected regions (kept largest: %d cells)"), Regions.Num() - 1, LargestRegionSize);
}

void URandWalkGenerator::RestoreIrregularBoundary(const TArray<EGridCellType>& OriginalGridState)
{
	// Any cell that was originally Empty (not part of random walk) should remain Empty
	// This ensures walls/boundaries respect the irregular shape
	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);
			int32 Index = URoomGenerationHelpers::CoordinateToIndex(Cell, GridSize. X);

			// If cell was originally empty (outside irregular shape), keep it empty
			if (OriginalGridState[Index] == EGridCellType::ECT_Empty)
			{
				SetCellState(Cell, EGridCellType::ECT_Empty);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("  Restored irregular boundary"));
}

bool URandWalkGenerator:: ValidateMinimumSize()
{
	TArray<FIntPoint> OccupiedCells = GetOccupiedCells();
	const int32 MinCells = 16;  // Minimum 4x4 equivalent

	return OccupiedCells. Num() >= MinCells;
}

#pragma endregion

#pragma region Helper Functions

int32 URandWalkGenerator::CountOccupiedNeighbors(FIntPoint Cell, int32 Radius) const
{
	int32 Count = 0;

	for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
	{
		for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
		{
			if (OffsetX == 0 && OffsetY == 0)
				continue;  // Skip self

			FIntPoint Neighbor = Cell + FIntPoint(OffsetX, OffsetY);

			if (IsValidGridCoordinate(Neighbor))
			{
				if (GetCellState(Neighbor) == EGridCellType::ECT_FloorMesh)
				{
					Count++;
				}
			}
			else
			{
				// Treat out-of-bounds as occupied (helps with edge smoothing)
				Count++;
			}
		}
	}

	return Count;
}

void URandWalkGenerator:: FloodFill(FIntPoint StartCell, TSet<FIntPoint>& OutRegion) const
{
	TArray<FIntPoint> Stack;
	Stack.Add(StartCell);

	while (Stack.Num() > 0)
	{
		FIntPoint Current = Stack. Pop();

		if (OutRegion.Contains(Current))
			continue;

		if (! IsValidGridCoordinate(Current) || GetCellState(Current) != EGridCellType::ECT_FloorMesh)
			continue;

		OutRegion. Add(Current);

		// Add neighbors to stack (4-directional)
		Stack.Add(Current + FIntPoint(1, 0));
		Stack.Add(Current + FIntPoint(-1, 0));
		Stack.Add(Current + FIntPoint(0, 1));
		Stack.Add(Current + FIntPoint(0, -1));
	}
}

TArray<FIntPoint> URandWalkGenerator::GetOccupiedCells() const
{
	TArray<FIntPoint> OccupiedCells;

	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);
			if (GetCellState(Cell) == EGridCellType::ECT_FloorMesh)
			{
				OccupiedCells.Add(Cell);
			}
		}
	}

	return OccupiedCells;
}

FIntPoint URandWalkGenerator::GetDirectionVector(int32 Direction)
{
	if (Direction >= 0 && Direction < DirectionVectors.Num())
	{
		return DirectionVectors[Direction];
	}

	return FIntPoint:: ZeroValue;
}

int32 URandWalkGenerator::GetOppositeDirection(int32 Direction)
{
	return (Direction + 2) % 4;
}

#pragma endregion