// Fill out your copyright notice in the Description page of Project Settings.

#include "Generators/Rooms/RoomGenerator.h"

#include "Data/Generation/RoomGenerationTypes.h"
#include "Utilities/Generation/RoomGenerationHelpers.h" 
#include "Data/Grid/GridData.h"
#include "Data/Room/CeilingData.h"
#include "Data/Room/DoorData.h"
#include "Data/Room/WallData.h"

bool URoomGenerator::Initialize(URoomData* InRoomData, FIntPoint InGridSize)
{
	if (!InRoomData)
	{
		UE_LOG(LogTemp, Error, TEXT("URoomGenerator::Initialize - InRoomData is null! "));
		return false;
	}

	RoomData = InRoomData;
	GridSize = InGridSize;
	CellSize = CELL_SIZE;
	bIsInitialized = true;

	// Initialize statistics
	LargeTilesPlaced = 0;
	MediumTilesPlaced = 0;
	SmallTilesPlaced = 0;
	FillerTilesPlaced = 0;

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::Initialize - Initialized with GridSize (%d, %d), CellSize %.2f"), 
	GridSize.X, GridSize.Y, CellSize);
	return true;
}

#pragma region Room Grid Management
void URoomGenerator::CreateGrid()
{
	// Empty - child classes must implement
	UE_LOG(LogTemp, Warning, TEXT("RoomGenerator::CreateGrid() called on base class - child should override!"));
}

void URoomGenerator:: ClearGrid()
{
	GridState.Empty();
	PlacedFloorMeshes. Empty();
	PlacedWallMeshes.Empty();
	PlacedBaseWallSegments.Empty();

	// Reset statistics
	LargeTilesPlaced = 0;
	MediumTilesPlaced = 0;
	SmallTilesPlaced = 0;
	FillerTilesPlaced = 0;
	
	bIsInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ClearGrid - Grid cleared"));
}

void URoomGenerator::ResetGridCellStates()
{
	if (! bIsInitialized)
	{ UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::ResetGridCellStates - Not initialized! ")); return; }

	// Reset all cells to empty
	int32 CellsReset = 0;
	for (EGridCellType& Cell : GridState)
	{
		if (Cell != EGridCellType::ECT_Empty)
		{
			Cell = EGridCellType:: ECT_Empty;
			CellsReset++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ResetGridCellStates - Reset %d cells to empty (Total: %d)"), 
		CellsReset, GridState.Num());
}

EGridCellType URoomGenerator:: GetCellState(FIntPoint GridCoord) const
{
	if (!IsValidGridCoordinate(GridCoord)) return EGridCellType::ECT_Empty;
	int32 Index = GridCoordToIndex(GridCoord); return GridState[Index];
}

bool URoomGenerator::SetCellState(FIntPoint GridCoord, EGridCellType NewState)
{
	if (!IsValidGridCoordinate(GridCoord))	return false;

	int32 Index = GridCoordToIndex(GridCoord);
	GridState[Index] = NewState; return true;
}

bool URoomGenerator::IsValidGridCoordinate(FIntPoint GridCoord) const
{
	return GridCoord.X >= 0 && GridCoord.X < GridSize.X && GridCoord.Y >= 0 && GridCoord.Y < GridSize.Y;
}

bool URoomGenerator::IsAreaAvailable(FIntPoint StartCoord, FIntPoint Size) const
{
	// Delegate to static helper
	return URoomGenerationHelpers::IsAreaAvailable(GridState, GridSize, StartCoord, Size, EGridCellType::ECT_Empty);
}

bool URoomGenerator::MarkArea(FIntPoint StartCoord, FIntPoint Size, EGridCellType CellType)
{
	// Check availability using helper
	if (!URoomGenerationHelpers::IsAreaAvailable(GridState, GridSize, StartCoord, Size, EGridCellType::ECT_Empty)) return false;

	// Mark cells using helper
	URoomGenerationHelpers:: MarkCellsOccupied(GridState, GridSize, StartCoord, Size, CellType); return true;
}

bool URoomGenerator::ClearArea(FIntPoint StartCoord, FIntPoint Size)
{
	// Validate coordinates
	if (StartCoord.X + Size.X > GridSize.X || StartCoord. Y + Size.Y > GridSize.Y) return false;

	// Use helper to clear (mark as Empty)
	URoomGenerationHelpers::MarkCellsOccupied(GridState, GridSize, StartCoord, Size, EGridCellType:: ECT_Empty); 
	return true;
}


#pragma endregion

#pragma region Floor Generation
bool URoomGenerator::GenerateFloor()
{
	UE_LOG(LogTemp, Warning, TEXT("RoomGenerator::GenerateFloor() called on base class - child should override!"));
	return false;
}

void URoomGenerator::ClearPlacedFloorMeshes()
{
	PlacedFloorMeshes.Empty();
	LargeTilesPlaced = 0;
	MediumTilesPlaced = 0;
	SmallTilesPlaced = 0;
	FillerTilesPlaced = 0;
}

void URoomGenerator::GetFloorStatistics(int32& OutLargeTiles, int32& OutMediumTiles, int32& OutSmallTiles, int32& OutFillerTiles) const
{
	OutLargeTiles = LargeTilesPlaced;
	OutMediumTiles = MediumTilesPlaced;
	OutSmallTiles = SmallTilesPlaced;
	OutFillerTiles = FillerTilesPlaced;
}

int32 URoomGenerator::ExecuteForcedPlacements()
{
	if (! bIsInitialized || !RoomData) 
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::ExecuteForcedPlacements - Not initialized! ")); return 0;}

	int32 SuccessfulPlacements = 0;
	const TMap<FIntPoint, FMeshPlacementInfo>& ForcedPlacements = RoomData->ForcedFloorPlacements;

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ExecuteForcedPlacements - Processing %d forced placements"), ForcedPlacements. Num());
	for (const auto& Pair : ForcedPlacements)
	{
		const FIntPoint StartCoord = Pair.Key;
		const FMeshPlacementInfo& MeshInfo = Pair.Value;

		// Validate mesh asset
		if (MeshInfo.MeshAsset. IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("  Forced placement at (%d,%d) has null mesh asset - skipping"), StartCoord.X, StartCoord.Y);
			continue;
		}

		// Calculate original footprint
		FIntPoint OriginalFootprint = CalculateFootprint(MeshInfo);

		UE_LOG(LogTemp, Verbose, TEXT("  Attempting forced placement at (%d,%d) with footprint %dx%d"), 
		StartCoord.X, StartCoord.Y, OriginalFootprint.X, OriginalFootprint.Y);

		// Try to find a rotation that fits the available space
		int32 BestRotation = -1;
		FIntPoint BestFootprint;

		if (MeshInfo.AllowedRotations.Num() > 0)
		{
			// Try each allowed rotation to find one that fits
			for (int32 Rotation :  MeshInfo.AllowedRotations)
			{
				FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);

				// Check if this rotation fits within grid bounds
				if (StartCoord.X + RotatedFootprint.X <= GridSize. X && StartCoord.Y + RotatedFootprint.Y <= GridSize.Y)
				{
					// Check if area is available
					if (IsAreaAvailable(StartCoord, RotatedFootprint))
					{
						BestRotation = Rotation;
						BestFootprint = RotatedFootprint;
						UE_LOG(LogTemp, Verbose, TEXT("    Found valid rotation %d° (footprint %dx%d)"), 
							Rotation, RotatedFootprint.X, RotatedFootprint.Y);
						break; // Use first valid rotation
					}
				}
			}
		}
		else
		{
			// No allowed rotations defined, try default (0°)
			BestRotation = 0;
			BestFootprint = OriginalFootprint;
		}

		// Check if we found a valid rotation
		if (BestRotation == -1)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Forced placement at (%d,%d) cannot fit with any allowed rotation - skipping"), 
				StartCoord.X, StartCoord.Y);
			continue;
		}

		// Validate bounds with best rotation
		if (StartCoord.X + BestFootprint.X > GridSize.X || 
		    StartCoord.Y + BestFootprint. Y > GridSize.Y)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Forced placement at (%d,%d) is out of bounds (size %dx%d) - skipping"), 
				StartCoord.X, StartCoord.Y, BestFootprint. X, BestFootprint.Y);
			continue;
		}

		// Final check if area is available
		if (! IsAreaAvailable(StartCoord, BestFootprint))
		{
			UE_LOG(LogTemp, Warning, TEXT("  Forced placement at (%d,%d) overlaps existing placement - skipping"), 
				StartCoord.X, StartCoord.Y);
			continue;
		}

		// Place the mesh with best rotation
		if (TryPlaceMesh(StartCoord, BestFootprint, MeshInfo, BestRotation))
		{
			SuccessfulPlacements++;
			UE_LOG(LogTemp, Log, TEXT("  ✓ Placed forced mesh at (%d,%d) size %dx%d rotation %d°"), 
				StartCoord.X, StartCoord.Y, BestFootprint. X, BestFootprint.Y, BestRotation);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("  Failed to place forced mesh at (%d,%d) - TryPlaceMesh returned false"), 
				StartCoord.X, StartCoord.Y);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ExecuteForcedPlacements - Placed %d/%d forced meshes"), 
		SuccessfulPlacements, ForcedPlacements.Num());

	return SuccessfulPlacements;
}

int32 URoomGenerator::FillRemainingGaps(const TArray<FMeshPlacementInfo>& TilePool,
	int32& OutLargeTiles,
	int32& OutMediumTiles,
	int32& OutSmallTiles,
	int32& OutFillerTiles)
{
if (TilePool.Num() == 0)
	{ UE_LOG(LogTemp, Warning, TEXT("URoomGenerator:: FillRemainingGaps - No meshes in tile pool! ")); return 0;}

	int32 PlacedCount = 0;

	// Define sizes to try (largest to smallest for efficiency)
	TArray<FIntPoint> SizesToTry = {
		FIntPoint(1, 4), // 100x400
		FIntPoint(4, 1), // 400x100
		FIntPoint(1, 2), // 100x200
		FIntPoint(2, 1), // 200x100
		FIntPoint(1, 1)  // 100x100
	};

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::FillRemainingGaps - Starting gap fill"));

	// Try each size in order
	for (const FIntPoint& TargetSize : SizesToTry)
	{
		// Filter tiles that match this size
		TArray<FMeshPlacementInfo> MatchingTiles;

		for (const FMeshPlacementInfo& MeshInfo : TilePool)
		{
			FIntPoint Footprint = CalculateFootprint(MeshInfo);

			// Check if footprint matches target size (or rotated version)
			if ((Footprint.X == TargetSize.X && Footprint.Y == TargetSize.Y) || (Footprint.X == TargetSize.Y && Footprint. Y == TargetSize.X))
			{
				MatchingTiles.Add(MeshInfo);
			}
		}

		if (MatchingTiles.Num() == 0) continue; // No tiles of this size, try next

		int32 SizePlacedCount = 0;

		// Try to place tiles of this size in all empty spaces
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			for (int32 X = 0; X < GridSize.X; ++X)
			{
				FIntPoint StartCoord(X, Y);

				// Check if area is available
				if (IsAreaAvailable(StartCoord, TargetSize))
				{
					// Select weighted random mesh
					FMeshPlacementInfo SelectedMesh = SelectWeightedMesh(MatchingTiles);
					FIntPoint OriginalFootprint = CalculateFootprint(SelectedMesh);

					// Find rotation that matches target size
					int32 BestRotation = 0;
					
					if (SelectedMesh.AllowedRotations.Num() > 0)
					{
						// Build list of rotations that would fit the target size
						TArray<int32> ValidRotations;

						for (int32 Rotation : SelectedMesh.AllowedRotations)
						{
							FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);
							
							if (RotatedFootprint.X == TargetSize.X && RotatedFootprint.Y == TargetSize.Y)
							{
								ValidRotations.Add(Rotation);
							}
						}

						// Select random rotation from valid options
						if (ValidRotations.Num() > 0)
						{
							int32 RandomIndex = FMath::RandRange(0, ValidRotations.Num() - 1);
							BestRotation = ValidRotations[RandomIndex];
						}
					}

					// Try to place mesh with rotation
					if (TryPlaceMesh(StartCoord, TargetSize, SelectedMesh, BestRotation))
					{
						SizePlacedCount++;
						PlacedCount++;

						// Update statistics
						int32 TileArea = TargetSize.X * TargetSize.Y;
						if (TileArea >= 16) OutLargeTiles++;
						else if (TileArea >= 4) OutMediumTiles++;
						else if (TileArea >= 2) OutSmallTiles++;
						else OutFillerTiles++;
					}
				}
			}
		}

		if (SizePlacedCount > 0)
		{
			UE_LOG(LogTemp, Verbose, TEXT("  Filled %d gaps with %dx%d tiles"), SizePlacedCount, TargetSize. X, TargetSize.Y);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::FillRemainingGaps - Placed %d gap-fill meshes"), PlacedCount);

	return PlacedCount;
}

TArray<FIntPoint> URoomGenerator::ExpandForcedEmptyRegions() const
{
	TArray<FIntPoint> ExpandedCells;

	if (! RoomData) return ExpandedCells;

	// 1. Expand all rectangular regions into individual cells
	for (const FForcedEmptyRegion& Region : RoomData->ForcedEmptyRegions)
	{
		// Calculate bounding box (handles any corner order)
		int32 MinX = FMath::Min(Region.StartCell.X, Region.EndCell. X);
		int32 MaxX = FMath::Max(Region.StartCell.X, Region.EndCell.X);
		int32 MinY = FMath::Min(Region.StartCell.Y, Region.EndCell. Y);
		int32 MaxY = FMath::Max(Region.StartCell.Y, Region.EndCell.Y);

		// Clamp to valid grid bounds
		MinX = FMath:: Clamp(MinX, 0, GridSize.X - 1);
		MaxX = FMath::Clamp(MaxX, 0, GridSize.X - 1);
		MinY = FMath::Clamp(MinY, 0, GridSize.Y - 1);
		MaxY = FMath:: Clamp(MaxY, 0, GridSize.Y - 1);

		// Add all cells within the rectangular region
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				FIntPoint Cell(X, Y);
				ExpandedCells.AddUnique(Cell); // AddUnique prevents duplicates
			}
		}
	}

	// 2. Add individual forced empty cells
	for (const FIntPoint& Cell : RoomData->ForcedEmptyFloorCells)
	{
		// Validate cell is within grid bounds
		if (Cell.X >= 0 && Cell.X < GridSize.X && Cell.Y >= 0 && Cell.Y < GridSize.Y) {	ExpandedCells.AddUnique(Cell); }
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator:: ExpandForcedEmptyRegions - Expanded to %d cells"), ExpandedCells.Num());

	return ExpandedCells;
}

void URoomGenerator::MarkForcedEmptyCells(const TArray<FIntPoint>& EmptyCells)
{
	for (const FIntPoint& Cell :  EmptyCells)
	{
		// Mark as Wall type (reserved/boundary marker)
		SetCellState(Cell, EGridCellType::ECT_WallMesh);
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::MarkForcedEmptyCells - Marked %d cells as empty"), EmptyCells.Num());
}
#pragma endregion

#pragma region Wall Generation
bool URoomGenerator::GenerateWalls()
{
	UE_LOG(LogTemp, Warning, TEXT("RoomGenerator::GenerateWalls() called on base class - child should override!"));
	return false;
}

int32 URoomGenerator::ExecuteForcedWallPlacements()
{
	if (!bIsInitialized || !RoomData)
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::ExecuteForcedWallPlacements - Not initialized!")); return 0;	}

	// Check if there are any forced placements
	if (RoomData->ForcedWallPlacements.Num() == 0)
	{ UE_LOG(LogTemp, Verbose, TEXT("URoomGenerator:: ExecuteForcedWallPlacements - No forced walls to place")); return 0;}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ExecuteForcedWallPlacements - Processing %d forced walls"), 
		RoomData->ForcedWallPlacements.Num());

	int32 SuccessfulPlacements = 0;
	int32 FailedPlacements = 0;

	// Get wall offsets once (used for all walls)
	float NorthOffset = 0.0f;
	float SouthOffset = 0.0f;
	float EastOffset = 0.0f;
	float WestOffset = 0.0f;

	if (RoomData->WallStyleData.IsValid())
	{
		WallData = RoomData->WallStyleData.LoadSynchronous();
		if (WallData)
		{
			NorthOffset = WallData->NorthWallOffsetX;
			SouthOffset = WallData->SouthWallOffsetX;
			EastOffset = WallData->EastWallOffsetY;
			WestOffset = WallData->WestWallOffsetY;
		}
	}

	for (int32 i = 0; i < RoomData->ForcedWallPlacements.Num(); ++i)
	{
		const FForcedWallPlacement& ForcedWall = RoomData->ForcedWallPlacements[i];
		const FWallModule& Module = ForcedWall.WallModule;

		UE_LOG(LogTemp, Verbose, TEXT("  Forced Wall [%d]: Edge=%s, StartCell=%d, Footprint=%d"), i, 
		*UEnum::GetValueAsString(ForcedWall.Edge), ForcedWall.StartCell, Module.Y_AxisFootprint);
	 
		// VALIDATION:  Load Base Mesh
		UStaticMesh* BaseMesh = URoomGenerationHelpers::LoadAndValidateMesh( Module.BaseMesh,
			FString:: Printf(TEXT("ForcedWall[%d]"), i), true);

		if (!BaseMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("    SKIPPED: BaseMesh failed to load"));
			FailedPlacements++;
			continue;
		}

	 
		// VALIDATION: Check Edge Cells
		TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(ForcedWall.Edge, GridSize);

		if (EdgeCells.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("    SKIPPED: No cells on edge %s"), *UEnum::GetValueAsString(ForcedWall.Edge));
			FailedPlacements++;
			continue;
		}
	 
		// VALIDATION: Check Bounds
	 	int32 Footprint = Module.Y_AxisFootprint;
		if (ForcedWall.StartCell < 0 || ForcedWall.StartCell + Footprint > EdgeCells.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("    SKIPPED: Out of bounds (StartCell=%d, Footprint=%d, EdgeLength=%d)"),
				ForcedWall.StartCell, Footprint, EdgeCells. Num());
			FailedPlacements++;
			continue;
		}

		// NOTE: Overlap checking with other forced walls would happen here
		// For now, we assume designers don't create overlapping forced placements
	 
		// PLACEMENT: Calculate Position & Rotation Using Helpers
		FVector WallPosition = URoomGenerationHelpers:: CalculateWallPosition(
		ForcedWall.Edge, ForcedWall.StartCell, Footprint, GridSize, CellSize,
		NorthOffset, SouthOffset, EastOffset, WestOffset);

		FRotator WallRotation = URoomGenerationHelpers::GetWallRotationForEdge(ForcedWall.Edge);
	 
		// PLACEMENT: Create Base Wall Transform
		FTransform BaseTransform(WallRotation, WallPosition, FVector:: OneVector);
	 
		// TRACKING: Store Segment for Middle/Top Spawning
		FGeneratorWallSegment Segment;
		Segment.Edge = ForcedWall.Edge;
		Segment.StartCell = ForcedWall.StartCell;
		Segment. SegmentLength = Footprint;
		Segment.BaseTransform = BaseTransform;
		Segment.BaseMesh = BaseMesh;
		Segment.WallModule = &Module;  // Store pointer to module data

		PlacedBaseWallSegments.Add(Segment);

		UE_LOG(LogTemp, Verbose, TEXT("    ✓ Forced wall tracked: Edge=%s, StartCell=%d, Footprint=%d"),
		*UEnum::GetValueAsString(ForcedWall.Edge), ForcedWall.StartCell, Footprint);
		SuccessfulPlacements++;
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ExecuteForcedWallPlacements - Placed %d/%d forced walls (%d failed)"),
	SuccessfulPlacements, RoomData->ForcedWallPlacements. Num(), FailedPlacements);
	return SuccessfulPlacements;
}

bool URoomGenerator::IsCellRangeOccupied(EWallEdge Edge, int32 StartCell, int32 Length) const
{
	// Check if any forced wall overlaps with this range
	for (const FGeneratorWallSegment& Segment : PlacedBaseWallSegments)
	{
		if (Segment.Edge != Edge) continue;

		// Check for overlap:  [Start1, End1) overlaps [Start2, End2) if Start1 < End2 AND Start2 < End1
		int32 SegmentEnd = Segment.StartCell + Segment.SegmentLength;
		int32 RangeEnd = StartCell + Length;

		if (StartCell < SegmentEnd && Segment.StartCell < RangeEnd) return true;  // Overlap detected
	}
	return false;
}

void URoomGenerator::ClearPlacedWalls()
{
	PlacedWallMeshes.Empty();
}

void URoomGenerator::SpawnMiddleWallLayers()
{
	if (!RoomData || RoomData->WallStyleData.IsNull()) return;

	// Get fallback height from WallData
	float FallbackHeight = 100.0f;
	WallData = RoomData->WallStyleData.LoadSynchronous();
	if (WallData) { FallbackHeight = WallData->WallHeight;}

	int32 Middle1Spawned = 0;
	int32 Middle2Spawned = 0;

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::SpawnMiddleWallLayers - Processing %d base segments"), PlacedBaseWallSegments.Num());

	for (const FGeneratorWallSegment& Segment : PlacedBaseWallSegments)
	{
		if (!Segment.WallModule) continue;

		// MIDDLE 1 LAYER
		UStaticMesh* Middle1Mesh = Segment.WallModule->MiddleMesh1.LoadSynchronous();

		if (Middle1Mesh)
		{
			// ✅ Single line replaces 15+ lines of socket querying
			FTransform Middle1WorldTransform = URoomGenerationHelpers::CalculateSocketWorldTransform(
				Segment.BaseMesh,
				FName("TopBackCenter"),
				Segment.BaseTransform,
				FVector(0, 0, FallbackHeight));

			// Store wall info
			FPlacedWallInfo PlacedWall;
			PlacedWall.Edge = Segment.Edge;
			PlacedWall.StartCell = Segment.StartCell;
			PlacedWall.SpanLength = Segment.SegmentLength;
			PlacedWall.WallModule = *Segment.WallModule;
			PlacedWall.BottomTransform = Segment.BaseTransform;
			PlacedWall.Middle1Transform = Middle1WorldTransform;

			PlacedWallMeshes.Add(PlacedWall);
			Middle1Spawned++;

			// MIDDLE 2 LAYER
			UStaticMesh* Middle2Mesh = Segment.WallModule->MiddleMesh2.LoadSynchronous();

			if (Middle2Mesh)
			{
				// ✅ Single line replaces 10+ lines of socket querying
				FTransform Middle2WorldTransform = URoomGenerationHelpers::CalculateSocketWorldTransform(
				Middle1Mesh, FName("TopBackCenter"), Middle1WorldTransform, FVector(0, 0, FallbackHeight));
				PlacedWallMeshes.Last().Middle2Transform = Middle2WorldTransform;
				Middle2Spawned++;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::SpawnMiddleWallLayers - Middle1: %d, Middle2: %d"), Middle1Spawned, Middle2Spawned);
}

void URoomGenerator::SpawnTopWallLayer()
{
	if (! RoomData || RoomData->WallStyleData.IsNull()) return;

	// Get fallback height from WallData
	float FallbackHeight = 100.0f;  // Default fallback
	WallData = RoomData->WallStyleData.LoadSynchronous();
	if (WallData) {	FallbackHeight = WallData->WallHeight;}

	int32 TopSpawned = 0;

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator:: SpawnTopWallLayer - Processing %d wall segments"), PlacedWallMeshes.Num());

	for (FPlacedWallInfo& Wall :  PlacedWallMeshes)
	{
		// Load top mesh (required)
		UStaticMesh* TopMesh = Wall.WallModule. TopMesh.LoadSynchronous();
		if (!TopMesh)continue;
	 
		// DETERMINE WHICH LAYER TO STACK ON (Priority:  Middle2 > Middle1 > Base)
		// Load middle meshes to determine stack point
		UStaticMesh* Middle2Mesh = Wall.WallModule.MiddleMesh2.LoadSynchronous();
		UStaticMesh* Middle1Mesh = Wall.WallModule. MiddleMesh1.LoadSynchronous();

		UStaticMesh* SnapToMesh = nullptr;
		FTransform StackBaseTransform;

		// Priority 1: Stack on Middle2 (if it exists)
		if (Middle2Mesh)
		{
			SnapToMesh = Middle2Mesh;
			StackBaseTransform = Wall.Middle2Transform;
		}
		// Priority 2: Stack on Middle1 (if Middle2 doesn't exist)
		else if (Middle1Mesh)
		{
			SnapToMesh = Middle1Mesh;
			StackBaseTransform = Wall.Middle1Transform;
		}
		// Priority 3: Stack directly on Base
		else
		{
			SnapToMesh = Wall.WallModule.BaseMesh. LoadSynchronous();
			StackBaseTransform = Wall. BottomTransform;
		}
	 
		// CALCULATE TOP TRANSFORM USING SOCKET HELPER
		FTransform TopWorldTransform = URoomGenerationHelpers::CalculateSocketWorldTransform(
		SnapToMesh, FName("TopBackCenter"), StackBaseTransform, FVector(0, 0, FallbackHeight));
		// Store transform
		Wall.TopTransform = TopWorldTransform;
		TopSpawned++;
	}

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::SpawnTopWallLayer - Top meshes: %d"), TopSpawned);
}
#pragma endregion

#pragma region Corner Generation
bool URoomGenerator::GenerateCorners()
{
	 if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator:: GenerateCorners - Generator not initialized! "));
        return false;
    }

    if (! RoomData || RoomData->WallStyleData. IsNull())
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator:: GenerateCorners - WallStyleData not assigned!"));
        return false;
    }

    WallData = RoomData->WallStyleData.LoadSynchronous();
    if (!WallData)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateCorners - Failed to load WallStyleData!"));
        return false;
    }

    // Clear previous corners
    ClearPlacedCorners();

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateCorners - Starting corner generation"));

    // Load corner mesh (required)
    if (WallData->DefaultCornerMesh.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateCorners - No default corner mesh defined, skipping corners"));
        return true; // Not an error, just no corners to place
    }

    UStaticMesh* CornerMesh = WallData->DefaultCornerMesh.LoadSynchronous();
    if (!CornerMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateCorners - Failed to load corner mesh"));
        return false;
    }

     
    // Define corner data (matching MasterRoom's clockwise order:  SW, SE, NE, NW)
     
    
    struct FCornerData
    {
        ECornerPosition Position;
        FVector BasePosition;  // Grid corner position (before offset)
        FRotator Rotation;     // From WallData
        FVector Offset;        // Per-corner offset from WallData
        FString Name;          // For logging
    };

    TArray<FCornerData> Corners = {
        { 
            ECornerPosition::SouthWest, 
            FVector(0.0f, 0.0f, 0.0f),  // Bottom-left
            WallData->SouthWestCornerRotation, 
            WallData->SouthWestCornerOffset,
            TEXT("SouthWest")
        },
        { 
            ECornerPosition::SouthEast, 
            FVector(0.0f, GridSize.Y * CellSize, 0.0f),  // Bottom-right
            WallData->SouthEastCornerRotation, 
            WallData->SouthEastCornerOffset,
            TEXT("SouthEast")
        },
        { 
            ECornerPosition::NorthEast, 
            FVector(GridSize.X * CellSize, GridSize.Y * CellSize, 0.0f),  // Top-right
            WallData->NorthEastCornerRotation, 
            WallData->NorthEastCornerOffset,
            TEXT("NorthEast")
        },
        { 
            ECornerPosition:: NorthWest, 
            FVector(GridSize.X * CellSize, 0.0f, 0.0f),  // Top-left
            WallData->NorthWestCornerRotation, 
            WallData->NorthWestCornerOffset,
            TEXT("NorthWest")
        }
    };

     
    // Generate each corner
     
    
    for (const FCornerData& CornerData : Corners)
    {
        // Apply designer offset to base position
        FVector FinalPosition = CornerData. BasePosition + CornerData. Offset;

        // Create transform (local/component space)
        FTransform CornerTransform(CornerData.Rotation, FinalPosition, FVector:: OneVector);

        // Create placed corner info
        FPlacedCornerInfo PlacedCorner;
        PlacedCorner.Corner = CornerData. Position;
        PlacedCorner.CornerMesh = WallData->DefaultCornerMesh;
        PlacedCorner.Transform = CornerTransform;

        PlacedCornerMeshes.Add(PlacedCorner);

        UE_LOG(LogTemp, Verbose, TEXT("  Placed %s corner at position %s with rotation (%.0f, %.0f, %.0f)"),
            *CornerData.Name,
            *FinalPosition.ToString(),
            CornerData. Rotation.Roll,
            CornerData.Rotation. Pitch,
            CornerData.Rotation.Yaw);
    }

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateCorners - Complete.  Placed %d corners"), PlacedCornerMeshes.Num());

    return true;
}

void URoomGenerator::ClearPlacedCorners()
{
	PlacedCornerMeshes.Empty();
}
#pragma endregion

#pragma region Doorway Generation

bool URoomGenerator::GenerateDoorways()
{
    if (!  bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateDoorways - Generator not initialized!  "));
        return false;
    }

    if (!  RoomData)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateDoorways - RoomData is null! "));
        return false;
    }

     
    // CHECK FOR CACHED LAYOUT
     
    
    if (CachedDoorwayLayouts.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateDoorways - Using cached layout (%d doorways), recalculating transforms"),
            CachedDoorwayLayouts.Num());
        
        // Clear old transforms but keep layout
        PlacedDoorwayMeshes.  Empty();
        
        // Recalculate transforms from cached layouts (with current offsets)
        for (const FDoorwayLayoutInfo& Layout : CachedDoorwayLayouts)
        {
            FPlacedDoorwayInfo PlacedDoor = CalculateDoorwayTransforms(Layout);
            PlacedDoorwayMeshes.Add(PlacedDoor);
        }
        
        MarkDoorwayCells();
        
        UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateDoorways - Transforms recalculated with current offsets"));
        return true;
    }

     
    // NO CACHE - GENERATE NEW LAYOUT
     

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateDoorways - Generating new doorway layout"));

    // Clear both layout and transforms
    PlacedDoorwayMeshes. Empty();
    CachedDoorwayLayouts.  Empty();

    int32 ManualDoorwaysPlaced = 0;
    int32 AutomaticDoorwaysPlaced = 0;

     
    // PHASE 1: Process Manual Doorway Placements
     

    for (const FFixedDoorLocation& ForcedDoor : RoomData->ForcedDoorways)
    {
        // Validate door data
        UDoorData* DoorData = ForcedDoor.DoorData ?  ForcedDoor.DoorData : RoomData->DefaultDoorData;
        
        if (!DoorData)
        {
            UE_LOG(LogTemp, Warning, TEXT("  Forced doorway has no DoorData, skipping"));
            continue;
        }
        
    	int32 DoorWidth = DoorData->GetTotalDoorwayWidth();
    	UE_LOG(LogTemp, Log, TEXT("  Manual doorway:  Edge=%s, FrameFootprint=%d, SideFills=%s, TotalWidth=%d"),
    	*UEnum::GetValueAsString(ForcedDoor.WallEdge), DoorData->FrameFootprintY, *UEnum:: GetValueAsString(DoorData->SideFillType), DoorWidth);
        // Validate bounds
        TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(ForcedDoor.WallEdge, GridSize);
        
        if (ForcedDoor.StartCell < 0 || ForcedDoor.StartCell + DoorWidth > EdgeCells.Num())
        {
            UE_LOG(LogTemp, Warning, TEXT("  Forced doorway out of bounds, skipping"));
            continue;
        }

        // ✅ Create and cache layout info
        FDoorwayLayoutInfo LayoutInfo;
        LayoutInfo.Edge = ForcedDoor.WallEdge;
        LayoutInfo.StartCell = ForcedDoor.StartCell;
        LayoutInfo.WidthInCells = DoorWidth;
        LayoutInfo.DoorData = DoorData;
        LayoutInfo.bIsStandardDoorway = false;
        LayoutInfo.ManualOffsets = ForcedDoor.DoorPositionOffsets;  // Store manual offsets

        CachedDoorwayLayouts.Add(LayoutInfo);

        // ✅ Calculate transforms from layout
        FPlacedDoorwayInfo PlacedDoor = CalculateDoorwayTransforms(LayoutInfo);
        PlacedDoorwayMeshes.Add(PlacedDoor);

        ManualDoorwaysPlaced++;
    }

     
    // PHASE 2: Generate Automatic Standard Doorway
     

    if (RoomData->bGenerateStandardDoorway && RoomData->DefaultDoorData)
    {
        // Determine edges to use
        TArray<EWallEdge> EdgesToUse;
        
        if (RoomData->bSetStandardDoorwayEdge)
        {
            EdgesToUse. Add(RoomData->StandardDoorwayEdge);
            UE_LOG(LogTemp, Log, TEXT("  Using manual edge:   %s"), 
                *UEnum::GetValueAsString(RoomData->StandardDoorwayEdge));
        }
        else if (RoomData->bMultipleDoorways)
        {
            int32 NumDoorways = FMath:: Clamp(RoomData->NumAutomaticDoorways, 2, 4);
            
            TArray<EWallEdge> AllEdges = {
                EWallEdge:: North, EWallEdge::  South, 
                EWallEdge:: East, EWallEdge:: West
            };
            
            FRandomStream Stream(FMath:: Rand());
            for (int32 i = AllEdges.Num() - 1; i > 0; --i)
            {
                int32 j = Stream.RandRange(0, i);
                AllEdges. Swap(i, j);
            }
            
            for (int32 i = 0; i < NumDoorways && i < AllEdges.Num(); ++i)
            {
                EdgesToUse.Add(AllEdges[i]);
            }
            
            UE_LOG(LogTemp, Log, TEXT("  Generating %d automatic doorways"), NumDoorways);
        }
        else
        {
            FRandomStream Stream(FMath::  Rand());
            TArray<EWallEdge> AllEdges = {
                EWallEdge::North, EWallEdge::South, 
                EWallEdge:: East, EWallEdge:: West
            };
            EWallEdge ChosenEdge = AllEdges[Stream.RandRange(0, AllEdges.Num() - 1)];
            EdgesToUse.Add(ChosenEdge);
            
            UE_LOG(LogTemp, Log, TEXT("  Using random edge:  %s"), 
                *UEnum::GetValueAsString(ChosenEdge));
        }
        
        // Generate doorway on each chosen edge
        for (EWallEdge ChosenEdge : EdgesToUse)
        {
            TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(ChosenEdge, GridSize);
            int32 EdgeLength = EdgeCells.Num();

            int32 StartCell = (EdgeLength - RoomData->StandardDoorwayWidth) / 2;
            StartCell = FMath:: Clamp(StartCell, 0, EdgeLength - RoomData->StandardDoorwayWidth);

            // Check for overlap with existing doorways
            bool bOverlaps = false;
            for (const FDoorwayLayoutInfo& ExistingLayout : CachedDoorwayLayouts)
            {
                if (ExistingLayout.Edge == ChosenEdge)
                {
                    int32 ExistingStart = ExistingLayout.StartCell;
                    int32 ExistingEnd = ExistingStart + ExistingLayout.WidthInCells;
                    int32 NewStart = StartCell;
                    int32 NewEnd = StartCell + RoomData->StandardDoorwayWidth;

                    if (NewStart < ExistingEnd && ExistingStart < NewEnd)
                    {
                        bOverlaps = true;
                        UE_LOG(LogTemp, Warning, TEXT("  Doorway on %s would overlap, skipping"),
                            *UEnum::  GetValueAsString(ChosenEdge));
                        break;
                    }
                }
            }

            if (!bOverlaps)
            {
                // ✅ Create and cache layout info
                FDoorwayLayoutInfo LayoutInfo;
                LayoutInfo.Edge = ChosenEdge;
                LayoutInfo.StartCell = StartCell;
                LayoutInfo. WidthInCells = RoomData->StandardDoorwayWidth;
                LayoutInfo.DoorData = RoomData->DefaultDoorData;
                LayoutInfo.bIsStandardDoorway = true;
                // No manual offsets for automatic doorways

                CachedDoorwayLayouts.Add(LayoutInfo);

                // ✅ Calculate transforms from layout
                FPlacedDoorwayInfo PlacedDoor = CalculateDoorwayTransforms(LayoutInfo);
                PlacedDoorwayMeshes. Add(PlacedDoor);

                AutomaticDoorwaysPlaced++;
            }
        }
    }

     
    // PHASE 3: Mark Doorway Cells
     

    MarkDoorwayCells();

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateDoorways - Complete.   Cached %d layouts, placed %d doorways"),
        CachedDoorwayLayouts.Num(), PlacedDoorwayMeshes.Num());

    return true;
}

FPlacedDoorwayInfo URoomGenerator::CalculateDoorwayTransforms(const FDoorwayLayoutInfo& Layout)
{
    FPlacedDoorwayInfo PlacedDoor;
    PlacedDoor.Edge = Layout.Edge;
    PlacedDoor.StartCell = Layout.StartCell;
    PlacedDoor.  WidthInCells = Layout.WidthInCells;
    PlacedDoor. DoorData = Layout.DoorData;
    PlacedDoor.bIsStandardDoorway = Layout.bIsStandardDoorway;

    // Calculate base position (no offsets)
    FVector BasePosition = URoomGenerationHelpers::  CalculateDoorwayPosition(
        Layout.Edge,
        Layout.StartCell,
        Layout.WidthInCells,
        GridSize,
        CellSize
    );

    // ✅ Get offsets based on doorway type
    FDoorPositionOffsets Offsets;
    
    if (Layout.bIsStandardDoorway)
    {
        // Automatic doorway:   use edge-specific offsets from DoorData
        Offsets = Layout.DoorData->GetOffsetsForEdge(Layout.  Edge);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("    Using edge-specific offsets for %s:  Frame=%s, Actor=%s"),
            *UEnum::GetValueAsString(Layout. Edge),
            *Offsets.  FramePositionOffset. ToString(),
            *Offsets. ActorPositionOffset.  ToString());
    }
    else
    {
        // Manual doorway:  use stored manual offsets
        Offsets = Layout.ManualOffsets;
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("    Using manual offsets:  Frame=%s, Actor=%s"),
            *Offsets. FramePositionOffset.ToString(),
            *Offsets. ActorPositionOffset. ToString());
    }

    // Apply offsets to base position
    FVector FinalFramePosition = BasePosition + Offsets.  FramePositionOffset;
    FVector FinalActorPosition = BasePosition + Offsets.ActorPositionOffset;

    // Calculate rotation
    FRotator Rotation = URoomGenerationHelpers::GetWallRotationForEdge(Layout.Edge);
    Rotation += Layout.DoorData->FrameRotationOffset;

    // Store transforms
    PlacedDoor. FrameTransform = FTransform(Rotation, FinalFramePosition, FVector:: OneVector);
    PlacedDoor.ActorTransform = FTransform(Rotation, FinalActorPosition, FVector::OneVector);

    return PlacedDoor;
}

void URoomGenerator::MarkDoorwayCells()
{
    for (const FPlacedDoorwayInfo& Doorway : PlacedDoorwayMeshes)
    {
        TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(Doorway.Edge, GridSize);

        for (int32 i = 0; i < Doorway.WidthInCells; ++i)
        {
            int32 CellIndex = Doorway.StartCell + i;
            if (CellIndex >= 0 && CellIndex < EdgeCells.Num())
            {
                FIntPoint Cell = EdgeCells[CellIndex];
                
                // Mark in grid state (if cell is within interior grid)
                // Note:  Boundary cells (virtual) are outside interior grid bounds
                // So we only mark if cell is within (0, GridSize-1)
                if (Cell.X >= 0 && Cell.X < GridSize. X && Cell.Y >= 0 && Cell.Y < GridSize.Y)
                {
                    int32 GridIndex = Cell.Y * GridSize.X + Cell.X;
                    if (GridState. IsValidIndex(GridIndex))
                    {
                        GridState[GridIndex] = EGridCellType::ECT_Doorway;
                    }
                }
                
                UE_LOG(LogTemp, VeryVerbose, TEXT("    Marked doorway cell:  (%d, %d)"), Cell.X, Cell.Y);
            }
        }
    }
}

bool URoomGenerator::IsCellPartOfDoorway(FIntPoint Cell) const
{
	for (const FPlacedDoorwayInfo& Doorway :  PlacedDoorwayMeshes)
	{
		TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(Doorway. Edge, GridSize);

		for (int32 i = 0; i < Doorway.WidthInCells; ++i)
		{
			int32 CellIndex = Doorway.StartCell + i;
			if (CellIndex >= 0 && CellIndex < EdgeCells.Num())
			{
				FIntPoint DoorwayCell = EdgeCells[CellIndex];
                
				// ✅ ADD LOGGING: 
				if (Cell == DoorwayCell)
				{
					UE_LOG(LogTemp, Warning, TEXT("    Cell (%d,%d) IS part of doorway on edge %s at index %d"),
						Cell. X, Cell.Y, *UEnum::GetValueAsString(Doorway.Edge), CellIndex);
					return true;
				}
			}
		}
	}

	return false;
}

void URoomGenerator::ClearPlacedDoorways()
{
    PlacedDoorwayMeshes.Empty();
	CachedDoorwayLayouts. Empty(); 
}
#pragma endregion

#pragma region Ceiling Generation
bool URoomGenerator::GenerateCeiling()
{
    if (! bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateCeiling - Generator not initialized!  "));
        return false;
    }

    if (! RoomData || RoomData->CeilingStyleData.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateCeiling - No CeilingStyleData assigned"));
        return false;
    }

    UCeilingData* CeilingData = RoomData->CeilingStyleData.LoadSynchronous();
    if (!CeilingData)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateCeiling - Failed to load CeilingStyleData"));
        return false;
    }

	if (CeilingData->CeilingTilePool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateCeiling - No tiles in CeilingTilePool! "));
		return false;
	}
	
    // Clear previous ceiling data
    ClearPlacedCeiling();

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateCeiling - Starting ceiling generation"));

    // Create occupancy grid
    TArray<bool> CeilingOccupied;
    CeilingOccupied.Init(false, GridSize.X * GridSize.Y);

    FRandomStream Stream(FMath::  Rand());

    int32 CeilingLargeTilesPlaced = 0;
    int32 CeilingMediumTilesPlaced = 0;
    int32 CeilingSmallTilesPlaced = 0;
	int32 CeilingFillerTilesPlaced = 0; 
     
    // HELPER LAMBDAS
     

    auto IsCellOccupied = [&](int32 X, int32 Y) -> bool
    {
        if (X < 0 || X >= GridSize.X || Y < 0 || Y >= GridSize.Y) return true;
        return CeilingOccupied[Y * GridSize.X + X];
    };

    auto MarkCellsOccupied = [&](int32 StartX, int32 StartY, FIntPoint Size)
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                int32 X = StartX + dx;
                int32 Y = StartY + dy;
                if (X >= 0 && X < GridSize.X && Y >= 0 && Y < GridSize.Y)
                {
                    CeilingOccupied[Y * GridSize.X + X] = true;
                }
            }
        }
    };

    auto IsAreaAvailable = [&](int32 StartX, int32 StartY, FIntPoint Size) -> bool
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                if (IsCellOccupied(StartX + dx, StartY + dy))
                    return false;
            }
        }
        return true;
    };

	// PHASE 0:  FORCED PLACEMENTS (Designer overrides - highest priority)
	int32 ForcedCount = ExecuteForcedCeilingPlacements(CeilingOccupied);
	if (ForcedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("  Phase 0: Placed %d forced ceiling tiles"), ForcedCount);
	}
	
    // PASS 1:  LARGE TILES (4x4)

	// Large tiles (400x400, 200x400, 400x200)
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(4, 4), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingLargeTilesPlaced);
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(2, 4), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingLargeTilesPlaced);
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(4, 2), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingLargeTilesPlaced);

	// Medium tiles (200x200)
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(2, 2), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingMediumTilesPlaced);

	// Small tiles (100x200, 200x100, 100x100)
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(1, 2), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingSmallTilesPlaced);
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(2, 1), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingSmallTilesPlaced);
	FillCeilingWithTileSize(CeilingData->CeilingTilePool, CeilingOccupied, FIntPoint(1, 1), CeilingData->CeilingRotation, CeilingData->CeilingHeight, CeilingSmallTilesPlaced);


     
    // PASS 2:  MEDIUM TILES (2x2)
	int32 GapFillCount = FillRemainingCeilingGaps(CeilingData->CeilingTilePool, CeilingOccupied, CeilingData->CeilingRotation, CeilingData->CeilingHeight,
	  CeilingLargeTilesPlaced, CeilingMediumTilesPlaced, CeilingSmallTilesPlaced, CeilingFillerTilesPlaced);
	UE_LOG(LogTemp, Log, TEXT("  Phase 2: Filled %d remaining gaps"), GapFillCount);
     
    // PASS 3:  SMALL TILES (1x1)
	if (CeilingData->CeilingTilePool.Num() > 0)
    {
        for (int32 Y = 0; Y < GridSize.Y; Y++)
        {
            for (int32 X = 0; X < GridSize.X; X++)
            {
                if (!  IsCellOccupied(X, Y))
                {
                	FMeshPlacementInfo SelectedTile = SelectWeightedMesh(CeilingData->CeilingTilePool);

                    if (SelectedTile.MeshAsset.IsNull())
                    {
                        // ✅ CHANGED:   Use GridFootprint from tile
                        FIntPoint TileFootprint = SelectedTile.GridFootprint;
                        
                        FVector TilePosition = FVector(
                            (X + TileFootprint.X / 2.0f) * CellSize,
                            (Y + TileFootprint.Y / 2.0f) * CellSize,
                            CeilingData->CeilingHeight
                        );

                    	// Create rotation (base ceiling rotation + tile rotation)
                    	FRotator FinalRotation = CeilingData->CeilingRotation;

                    	// Normalize quaternion to avoid floating point errors
                    	FQuat NormalizedRotation = FinalRotation.Quaternion();
                    	NormalizedRotation.Normalize();

                    	// Create transform
                    	FTransform TileTransform(NormalizedRotation, TilePosition, FVector(1.0f));

                        FPlacedCeilingInfo PlacedTile;
                        PlacedTile. GridCoordinate = FIntPoint(X, Y);
                        PlacedTile.TileSize = TileFootprint;
                        PlacedTile.MeshInfo = SelectedTile;
                         PlacedTile.LocalTransform = TileTransform;

                        PlacedCeilingTiles.Add(PlacedTile);
                        MarkCellsOccupied(X, Y, TileFootprint);
                        CeilingSmallTilesPlaced++;
                    }
                }
            }
        }
    }

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateCeiling - Complete:  %d large, %d medium, %d small, %d filler = %d total"),
		CeilingLargeTilesPlaced, CeilingMediumTilesPlaced, CeilingSmallTilesPlaced, CeilingFillerTilesPlaced, PlacedCeilingTiles. Num());

	return true;
}

int32 URoomGenerator::ExecuteForcedCeilingPlacements(TArray<bool>& CeilingOccupied)
{
    if (!bIsInitialized || ! RoomData)
    {
        UE_LOG(LogTemp, Error, TEXT("URoomGenerator:: ExecuteForcedCeilingPlacements - Not initialized!"));
        return 0;
    }

    // Check if there are any forced placements
    if (RoomData->ForcedCeilingPlacements.Num() == 0)
    {
        UE_LOG(LogTemp, Verbose, TEXT("URoomGenerator::ExecuteForcedCeilingPlacements - No forced ceiling tiles"));
        return 0;
    }

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ExecuteForcedCeilingPlacements - Processing %d forced tiles"),
        RoomData->ForcedCeilingPlacements. Num());

    int32 SuccessfulPlacements = 0;

    // Load ceiling data for height/rotation
    UCeilingData* CeilingData = RoomData->CeilingStyleData.LoadSynchronous();
    if (!CeilingData)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteForcedCeilingPlacements - Failed to load CeilingStyleData"));
        return 0;
    }

    // Lambda: Check if area is available
    auto IsAreaAvailable = [&](int32 StartX, int32 StartY, FIntPoint Size) -> bool
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                int32 CheckX = StartX + dx;
                int32 CheckY = StartY + dy;
                
                if (CheckX < 0 || CheckX >= GridSize.X || CheckY < 0 || CheckY >= GridSize.Y)
                    return false;

                int32 Index = CheckY * GridSize.X + CheckX;
                if (CeilingOccupied. IsValidIndex(Index) && CeilingOccupied[Index])
                    return false;
            }
        }
        return true;
    };

    // Lambda: Mark cells as occupied
    auto MarkCellsOccupied = [&](int32 StartX, int32 StartY, FIntPoint Size)
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                int32 X = StartX + dx;
                int32 Y = StartY + dy;
                int32 Index = Y * GridSize.X + X;

                if (CeilingOccupied.IsValidIndex(Index))
                {
                    CeilingOccupied[Index] = true;
                }
            }
        }
    };

    // Process each forced placement
    for (int32 i = 0; i < RoomData->ForcedCeilingPlacements. Num(); ++i)
    {
        const FForcedCeilingPlacement& ForcedTile = RoomData->ForcedCeilingPlacements[i];
        const FMeshPlacementInfo& TileInfo = ForcedTile. TileInfo;  // ✅ Now uses FMeshPlacementInfo

        UE_LOG(LogTemp, Verbose, TEXT("  Forced Tile [%d]:  Coord=(%d,%d), Footprint=(%d,%d)"),
            i, ForcedTile.GridCoordinate.X, ForcedTile.GridCoordinate.Y,
            TileInfo.GridFootprint.X, TileInfo. GridFootprint.Y);

        // VALIDATION: Check mesh
        if (TileInfo.MeshAsset.IsNull())
        {
            UE_LOG(LogTemp, Warning, TEXT("    SKIPPED:  Null mesh asset"));
            continue;
        }

        // Calculate original footprint
        FIntPoint OriginalFootprint = CalculateFootprint(TileInfo);

        // Try to find a rotation that fits
        int32 BestRotation = -1;
        FIntPoint BestFootprint;

        // Determine rotations to try
        TArray<int32> RotationsToTry;
        if (ForcedTile.AllowedRotations.Num() > 0)
        {
            RotationsToTry = ForcedTile.AllowedRotations;  // Use forced placement overrides
        }
        else if (TileInfo.AllowedRotations.Num() > 0)
        {
            RotationsToTry = TileInfo.AllowedRotations;    // Use tile's default rotations
        }
        else
        {
            RotationsToTry. Add(0);  // Default to no rotation
        }

        // Try each allowed rotation
        for (int32 Rotation : RotationsToTry)
        {
            FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);

            // Check bounds
            int32 EndX = ForcedTile.GridCoordinate.X + RotatedFootprint.X;
            int32 EndY = ForcedTile.GridCoordinate.Y + RotatedFootprint.Y;

            if (EndX <= GridSize.X && EndY <= GridSize.Y &&
                ForcedTile.GridCoordinate.X >= 0 && ForcedTile.GridCoordinate.Y >= 0)
            {
                // Check availability
                if (IsAreaAvailable(ForcedTile.GridCoordinate.X, ForcedTile.GridCoordinate.Y, RotatedFootprint))
                {
                    BestRotation = Rotation;
                    BestFootprint = RotatedFootprint;
                    break; // Use first valid rotation
                }
            }
        }

        // Check if we found a valid rotation
        if (BestRotation == -1)
        {
            UE_LOG(LogTemp, Warning, TEXT("    SKIPPED:  No valid rotation fits (tried %d rotations)"),
                RotationsToTry. Num());
            continue;
        }

        // Calculate tile position (centered on footprint)
        FVector TilePosition = FVector(
            (ForcedTile.GridCoordinate.X + BestFootprint.X / 2.0f) * CellSize,
            (ForcedTile.GridCoordinate.Y + BestFootprint. Y / 2.0f) * CellSize,
            CeilingData->CeilingHeight
        );

        // Create rotation (base ceiling rotation + tile rotation)
        FRotator FinalRotation = CeilingData->CeilingRotation;
        FinalRotation.Yaw += BestRotation;

        // Normalize quaternion to avoid floating point errors
        FQuat NormalizedRotation = FinalRotation.Quaternion();
        NormalizedRotation.Normalize();

        // Create transform
        FTransform TileTransform(NormalizedRotation, TilePosition, FVector(1.0f));

        // Create placed ceiling info
        FPlacedCeilingInfo PlacedTile;
        PlacedTile.GridCoordinate = ForcedTile.GridCoordinate;
        PlacedTile.TileSize = BestFootprint;
        PlacedTile.MeshInfo = TileInfo;  // ✅ Store full MeshPlacementInfo
         PlacedTile.LocalTransform = TileTransform;

        PlacedCeilingTiles.Add(PlacedTile);
        MarkCellsOccupied(ForcedTile.GridCoordinate.X, ForcedTile.GridCoordinate.Y, BestFootprint);

        UE_LOG(LogTemp, Log, TEXT("    ✓ Placed forced tile at (%d,%d) size (%dx%d) rotation (%d°)"),
            ForcedTile.GridCoordinate.X, ForcedTile.GridCoordinate.Y,
            BestFootprint.X, BestFootprint.Y, BestRotation);

        SuccessfulPlacements++;
    }

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::ExecuteForcedCeilingPlacements - Placed %d/%d tiles"),
        SuccessfulPlacements, RoomData->ForcedCeilingPlacements.Num());

    return SuccessfulPlacements;
}
#pragma endregion

#pragma region Internal Floor Generation
void URoomGenerator::FillWithTileSize(const TArray<FMeshPlacementInfo>& TilePool, 
	FIntPoint TargetSize,
	int32& OutLargeTiles,
	int32& OutMediumTiles,
	int32& OutSmallTiles,
	int32& OutFillerTiles)
{
	// Filter tiles that match target size
	TArray<FMeshPlacementInfo> MatchingTiles;

	for (const FMeshPlacementInfo& MeshInfo : TilePool)
	{
		FIntPoint Footprint = CalculateFootprint(MeshInfo);

		// Check if footprint matches target size (or rotated version)
		if ((Footprint.X == TargetSize.X && Footprint.Y == TargetSize.Y) ||
			(Footprint.X == TargetSize.Y && Footprint. Y == TargetSize.X))
		{
			MatchingTiles.Add(MeshInfo);
		}
	}

	if (MatchingTiles.Num() == 0) return; // No tiles of this size

	UE_LOG(LogTemp, Verbose, TEXT("URoomGenerator::FillWithTileSize - Filling with %dx%d tiles (%d options)"), 
		TargetSize.X, TargetSize.Y, MatchingTiles.Num());

	// Try to place tiles of this size across the grid
	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			FIntPoint StartCoord(X, Y);

			// Check if area is available for target size
			if (IsAreaAvailable(StartCoord, TargetSize))
			{
				// Select weighted random mesh
				FMeshPlacementInfo SelectedMesh = SelectWeightedMesh(MatchingTiles);
				FIntPoint OriginalFootprint = CalculateFootprint(SelectedMesh);

				// Find rotation that matches target size
				int32 BestRotation = 0;
				
				if (SelectedMesh.AllowedRotations.Num() > 0)
				{
					// Build list of rotations that would fit the target size
					TArray<int32> ValidRotations;

					for (int32 Rotation :  SelectedMesh.AllowedRotations)
					{
						FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);
						
						if (RotatedFootprint.X == TargetSize.X && RotatedFootprint.Y == TargetSize.Y)
						{
							ValidRotations.Add(Rotation);
						}
					}

					// Select random rotation from valid options
					if (ValidRotations.Num() > 0)
					{
						int32 RandomIndex = FMath::RandRange(0, ValidRotations.Num() - 1);
						BestRotation = ValidRotations[RandomIndex];
					}
				}

				// Try to place mesh with selected rotation
				if (TryPlaceMesh(StartCoord, TargetSize, SelectedMesh, BestRotation))
				{
					// Update statistics
					int32 TileArea = TargetSize.X * TargetSize.Y;
					if (TileArea >= 16) OutLargeTiles++;
					else if (TileArea >= 4) OutMediumTiles++;
					else if (TileArea >= 2) OutSmallTiles++;
					else OutFillerTiles++;
				}
			}
		}
	}
}

FMeshPlacementInfo URoomGenerator::SelectWeightedMesh(const TArray<FMeshPlacementInfo>& Pool)
{
	// Delegate to helper function
	const FMeshPlacementInfo* Selected = URoomGenerationHelpers::SelectWeightedMeshPlacement(Pool);
	
	// Return by value (copy), or empty if selection failed
	if (Selected) return *Selected;
	
	return FMeshPlacementInfo(); // Return empty if pool was empty
}

bool URoomGenerator::TryPlaceMesh(FIntPoint StartCoord, FIntPoint Size, const FMeshPlacementInfo& MeshInfo, int32 Rotation)
{
	// Try to place in grid using helper (handles validation + marking)
	if (! URoomGenerationHelpers::TryPlaceMeshInGrid(GridState, GridSize, StartCoord, Size, EGridCellType::ECT_FloorMesh))
		return false;
	
	// Create placed mesh info
	FPlacedMeshInfo PlacedMesh;
	PlacedMesh.GridPosition = StartCoord;
	PlacedMesh.GridFootprint = Size;
	PlacedMesh. Rotation = Rotation;
	PlacedMesh.MeshInfo = MeshInfo;

	// Calculate transform using helper
	PlacedMesh.LocalTransform = URoomGenerationHelpers::CalculateMeshTransform(StartCoord,Size,
	CellSize, Rotation,0.0f);  // Z offset (floor is at 0)

	// Store placed mesh (internal state management)
	PlacedFloorMeshes.Add(PlacedMesh);

	return true;
}

FIntPoint URoomGenerator::CalculateFootprint(const FMeshPlacementInfo& MeshInfo) const
{
	// If footprint is explicitly defined, use it
	if (MeshInfo.GridFootprint.X > 0 && MeshInfo. GridFootprint.Y > 0) return MeshInfo.GridFootprint;

	// Otherwise, calculate from mesh bounds
	if (! MeshInfo.MeshAsset. IsNull())
	{
		// For now, return default 1x1 if bounds not available
		// TODO: Load mesh and calculate actual bounds
		return FIntPoint(1, 1);
	}

	// Fallback
	return FIntPoint(1, 1);
}

void URoomGenerator::FillCeilingWithTileSize(const TArray<FMeshPlacementInfo>& TilePool, TArray<bool>& CeilingOccupied,
	FIntPoint TargetSize, const FRotator& CeilingRotation, float CeilingHeight, int32& OutTilesPlaced)
{
	// Filter tiles that match target size
    TArray<FMeshPlacementInfo> MatchingTiles;

    for (const FMeshPlacementInfo& TileInfo :  TilePool)
    {
        FIntPoint Footprint = CalculateFootprint(TileInfo);

        // Check if footprint matches target size (or rotated version)
        if ((Footprint.X == TargetSize.X && Footprint.Y == TargetSize. Y) ||
            (Footprint.X == TargetSize.Y && Footprint.Y == TargetSize.X))
        {
            MatchingTiles.Add(TileInfo);
        }
    }

    if (MatchingTiles. Num() == 0) return; // No tiles of this size

    UE_LOG(LogTemp, Verbose, TEXT("  Filling ceiling with %dx%d tiles (%d options)"),
        TargetSize.X, TargetSize. Y, MatchingTiles. Num());

    // Lambda:  Check if cell is occupied
    auto IsCellOccupied = [&](int32 X, int32 Y) -> bool
    {
        if (X < 0 || X >= GridSize.X || Y < 0 || Y >= GridSize.Y) return true;
        return CeilingOccupied[Y * GridSize.X + X];
    };

    // Lambda: Check if area is available
    auto IsAreaAvailable = [&](int32 StartX, int32 StartY, FIntPoint Size) -> bool
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                if (IsCellOccupied(StartX + dx, StartY + dy))
                    return false;
            }
        }
        return true;
    };

    // Lambda: Mark cells as occupied
    auto MarkCellsOccupied = [&](int32 StartX, int32 StartY, FIntPoint Size)
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                int32 X = StartX + dx;
                int32 Y = StartY + dy;
                if (X >= 0 && X < GridSize.X && Y >= 0 && Y < GridSize.Y)
                {
                    CeilingOccupied[Y * GridSize.X + X] = true;
                }
            }
        }
    };

    // Try to place tiles of this size across the grid
    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            // Check if area is available for target size
            if (IsAreaAvailable(X, Y, TargetSize))
            {
                // Select weighted random mesh
                FMeshPlacementInfo SelectedTile = SelectWeightedMesh(MatchingTiles);
                FIntPoint OriginalFootprint = CalculateFootprint(SelectedTile);

                // Find rotation that matches target size
                int32 BestRotation = 0;

                if (SelectedTile.AllowedRotations.Num() > 0)
                {
                    // Build list of rotations that would fit the target size
                    TArray<int32> ValidRotations;

                    for (int32 Rotation : SelectedTile.AllowedRotations)
                    {
                        FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);

                        if (RotatedFootprint.X == TargetSize.X && RotatedFootprint.Y == TargetSize.Y)
                        {
                            ValidRotations.Add(Rotation);
                        }
                    }

                    // Select random rotation from valid options
                    if (ValidRotations.Num() > 0)
                    {
                        int32 RandomIndex = FMath::RandRange(0, ValidRotations.Num() - 1);
                        BestRotation = ValidRotations[RandomIndex];
                    }
                }
            	           	
                // Calculate tile position (centered on footprint)
                FVector TilePosition = FVector(
                    (X + TargetSize.X / 2.0f) * CellSize,
                    (Y + TargetSize.Y / 2.0f) * CellSize,
                    CeilingHeight
                );

                // Create rotation (base ceiling rotation + tile rotation)
                FRotator FinalRotation = CeilingRotation;
                FinalRotation. Yaw += BestRotation;
            	
                // Create transform
                FTransform TileTransform(FinalRotation, TilePosition, FVector(1.0f));

                // Create placed ceiling info
                FPlacedCeilingInfo PlacedTile;
                PlacedTile. GridCoordinate = FIntPoint(X, Y);
                PlacedTile.TileSize = TargetSize;
                PlacedTile.MeshInfo = SelectedTile;  // ✅ Store full MeshPlacementInfo
                 PlacedTile.LocalTransform = TileTransform;

                PlacedCeilingTiles.Add(PlacedTile);
                MarkCellsOccupied(X, Y, TargetSize);
                OutTilesPlaced++;
            }
        }
    }
}

int32 URoomGenerator::FillRemainingCeilingGaps(const TArray<FMeshPlacementInfo>& TilePool,
	TArray<bool>& CeilingOccupied, const FRotator& CeilingRotation, float CeilingHeight, int32& OutLargeTiles,
	int32& OutMediumTiles, int32& OutSmallTiles, int32& OutFillerTiles)
{
	 if (TilePool.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("  FillRemainingCeilingGaps - No tiles in pool!"));
        return 0;
    }

    int32 PlacedCount = 0;

    // Define sizes to try (largest to smallest for efficiency)
    TArray<FIntPoint> SizesToTry = {
        FIntPoint(1, 4), // 100x400
        FIntPoint(4, 1), // 400x100
        FIntPoint(1, 2), // 100x200
        FIntPoint(2, 1), // 200x100
        FIntPoint(1, 1)  // 100x100
    };

    UE_LOG(LogTemp, Verbose, TEXT("  FillRemainingCeilingGaps - Starting gap fill"));

    // Lambda: Check if cell is occupied
    auto IsCellOccupied = [&](int32 X, int32 Y) -> bool
    {
        if (X < 0 || X >= GridSize.X || Y < 0 || Y >= GridSize.Y) return true;
        return CeilingOccupied[Y * GridSize.X + X];
    };

    // Lambda:  Check if area is available
    auto IsAreaAvailable = [&](int32 StartX, int32 StartY, FIntPoint Size) -> bool
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                if (IsCellOccupied(StartX + dx, StartY + dy))
                    return false;
            }
        }
        return true;
    };

    // Lambda: Mark cells as occupied
    auto MarkCellsOccupied = [&](int32 StartX, int32 StartY, FIntPoint Size)
    {
        for (int32 dy = 0; dy < Size.Y; dy++)
        {
            for (int32 dx = 0; dx < Size.X; dx++)
            {
                int32 X = StartX + dx;
                int32 Y = StartY + dy;
                if (X >= 0 && X < GridSize. X && Y >= 0 && Y < GridSize.Y)
                {
                    CeilingOccupied[Y * GridSize.X + X] = true;
                }
            }
        }
    };

    // Try each size in order
    for (const FIntPoint& TargetSize : SizesToTry)
    {
        // Filter tiles that match this size
        TArray<FMeshPlacementInfo> MatchingTiles;

        for (const FMeshPlacementInfo& TileInfo : TilePool)
        {
            FIntPoint Footprint = CalculateFootprint(TileInfo);

            // Check if footprint matches target size (or rotated version)
            if ((Footprint.X == TargetSize.X && Footprint.Y == TargetSize.Y) ||
                (Footprint.X == TargetSize.Y && Footprint.Y == TargetSize.X))
            {
                MatchingTiles.Add(TileInfo);
            }
        }

        if (MatchingTiles.Num() == 0) continue; // No tiles of this size, try next

        int32 SizePlacedCount = 0;

        // Try to place tiles of this size in all empty spaces
        for (int32 Y = 0; Y < GridSize.Y; ++Y)
        {
            for (int32 X = 0; X < GridSize.X; ++X)
            {
                // Check if area is available
                if (IsAreaAvailable(X, Y, TargetSize))
                {
                    // Select weighted random mesh
                    FMeshPlacementInfo SelectedTile = SelectWeightedMesh(MatchingTiles);
                    FIntPoint OriginalFootprint = CalculateFootprint(SelectedTile);

                    // Find rotation that matches target size
                    int32 BestRotation = 0;

                    if (SelectedTile.AllowedRotations. Num() > 0)
                    {
                        // Build list of rotations that would fit the target size
                        TArray<int32> ValidRotations;

                        for (int32 Rotation : SelectedTile.AllowedRotations)
                        {
                            FIntPoint RotatedFootprint = GetRotatedFootprint(OriginalFootprint, Rotation);

                            if (RotatedFootprint.X == TargetSize.X && RotatedFootprint.Y == TargetSize.Y)
                            {
                                ValidRotations.Add(Rotation);
                            }
                        }

                        // Select random rotation from valid options
                        if (ValidRotations.Num() > 0)
                        {
                            int32 RandomIndex = FMath::RandRange(0, ValidRotations.Num() - 1);
                            BestRotation = ValidRotations[RandomIndex];
                        }
                    }

                    // Calculate tile position
                    FVector TilePosition = FVector(
                        (X + TargetSize.X / 2.0f) * CellSize,
                        (Y + TargetSize.Y / 2.0f) * CellSize,
                        CeilingHeight
                    );

                    // Create rotation
                    FRotator FinalRotation = CeilingRotation;
                    FinalRotation. Yaw += BestRotation;

                    // Normalize quaternion
                    FQuat NormalizedRotation = FinalRotation. Quaternion();
                    NormalizedRotation.Normalize();

                    // Create transform
                    FTransform TileTransform(NormalizedRotation, TilePosition, FVector(1.0f));

                    // Create placed ceiling info
                    FPlacedCeilingInfo PlacedTile;
                    PlacedTile.GridCoordinate = FIntPoint(X, Y);
                    PlacedTile.TileSize = TargetSize;
                    PlacedTile.MeshInfo = SelectedTile;  // ✅ Store full MeshPlacementInfo
                     PlacedTile.LocalTransform = TileTransform;

                    PlacedCeilingTiles. Add(PlacedTile);
                    MarkCellsOccupied(X, Y, TargetSize);

                    SizePlacedCount++;
                    PlacedCount++;

                    // Update statistics
                    int32 TileArea = TargetSize.X * TargetSize.Y;
                    if (TileArea >= 16) OutLargeTiles++;
                    else if (TileArea >= 4) OutMediumTiles++;
                    else if (TileArea >= 2) OutSmallTiles++;
                    else OutFillerTiles++;
                }
            }
        }

        if (SizePlacedCount > 0)
        {
            UE_LOG(LogTemp, Verbose, TEXT("    Filled %d gaps with %dx%d tiles"), SizePlacedCount, TargetSize.X, TargetSize.Y);
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("  FillRemainingCeilingGaps - Placed %d gap-fill tiles"), PlacedCount);

    return PlacedCount;
}
#pragma endregion

#pragma region Coordinate Conversion

FVector URoomGenerator::GridToLocalPosition(FIntPoint GridCoord) const
{
	// Calculate center of cell
	float LocalX = GridCoord.X * CellSize + (CellSize * 0.5f);
	float LocalY = GridCoord.Y * CellSize + (CellSize * 0.5f);
	return FVector(LocalX, LocalY, 0.0f);
}

FIntPoint URoomGenerator::LocalToGridPosition(FVector LocalPos) const
{
	// Floor division to get grid coordinate
	int32 GridX = FMath::FloorToInt(LocalPos.X / CellSize);
	int32 GridY = FMath:: FloorToInt(LocalPos. Y / CellSize);
	return FIntPoint(GridX, GridY);
}

FIntPoint URoomGenerator::GetRotatedFootprint(FIntPoint OriginalFootprint, int32 Rotation)
{
	// Normalize rotation to 0-359 range
	Rotation = Rotation % 360;
	if (Rotation < 0) Rotation += 360;

	// For 90 and 270 degree rotations, swap X and Y
	if (Rotation == 90 || Rotation == 270) return FIntPoint(OriginalFootprint.Y, OriginalFootprint.X);

	// For 0 and 180 degree rotations, keep original
	return OriginalFootprint;
}
#pragma endregion

#pragma region Room Statistics

int32 URoomGenerator::GetCellCountByType(EGridCellType CellType) const
{
	int32 Count = 0;
	for (const EGridCellType& Cell : GridState)
	{ if (Cell == CellType) ++Count; } return Count;
}

float URoomGenerator::GetOccupancyPercentage() const
{
	int32 TotalCells = GetTotalCellCount();
	if (TotalCells == 0) return 0.0f;

	int32 OccupiedCells = GetCellCountByType(EGridCellType::ECT_FloorMesh);
	return (static_cast<float>(OccupiedCells) / static_cast<float>(TotalCells)) * 100.0f;
}
#pragma endregion

#pragma region Internal Helpers

int32 URoomGenerator::GridCoordToIndex(FIntPoint GridCoord) const
{
	// Row-major order: Index = Y * Width + X
	return GridCoord.Y * GridSize.X + GridCoord.X;
}

FIntPoint URoomGenerator:: IndexToGridCoord(int32 Index) const
{
	// Reverse row-major order
	int32 X = Index % GridSize.X;
	int32 Y = Index / GridSize.X;
	return FIntPoint(X, Y);
}

void URoomGenerator::FillWallEdge(EWallEdge Edge)
{
    if (!  RoomData || RoomData->WallStyleData.IsNull()) return;

    WallData = RoomData->WallStyleData.LoadSynchronous();
    if (!WallData || WallData->AvailableWallModules. Num() == 0) return;

    TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(Edge, GridSize);
    if (EdgeCells.Num() == 0) return;

    FRotator WallRotation = URoomGenerationHelpers:: GetWallRotationForEdge(Edge);
    UE_LOG(LogTemp, Verbose, TEXT("  Filling edge %s with %d cells"),
        *UEnum::GetValueAsString(Edge), EdgeCells.Num());

    // Greedy bin packing: Fill with largest modules first (BASE LAYER ONLY)
    int32 CurrentCell = 0;

    while (CurrentCell < EdgeCells. Num())
    {
        // ✅ FIXED:   Check if THIS SPECIFIC CELL is part of a doorway
        FIntPoint CellToCheck = EdgeCells[CurrentCell];
        
        if (IsCellPartOfDoorway(CellToCheck))
        {
            UE_LOG(LogTemp, Warning, TEXT("    Skipping cell %d (%d,%d) - part of doorway"),
                CurrentCell, CellToCheck.X, CellToCheck.Y);
            CurrentCell++;
            continue;
        }
        
        // Skip cells occupied by forced walls
        if (IsCellRangeOccupied(Edge, CurrentCell, 1))
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("    Skipping cell %d (occupied by forced wall)"), CurrentCell);
            CurrentCell++;
            continue;
        }
        
        // Find largest module that fits remaining space
        const FWallModule* BestModule = nullptr;
        int32 SpaceLeft = EdgeCells.Num() - CurrentCell;

        for (const FWallModule& Module : WallData->AvailableWallModules)
        {
            // ✅ FIXED:  Check if the ENTIRE module span overlaps with doorways
            bool bModuleOverlapsDoorway = false;
            
            for (int32 i = 0; i < Module. Y_AxisFootprint; ++i)
            {
                int32 CheckIndex = CurrentCell + i;
                if (CheckIndex < EdgeCells.Num())
                {
                    FIntPoint CheckCell = EdgeCells[CheckIndex];
                    if (IsCellPartOfDoorway(CheckCell))
                    {
                        bModuleOverlapsDoorway = true;
                        break;
                    }
                }
            }
            
            if (bModuleOverlapsDoorway)
            {
                continue;  // Skip this module - it would overlap a doorway
            }
            
            if (Module.Y_AxisFootprint <= SpaceLeft && 
                !  IsCellRangeOccupied(Edge, CurrentCell, Module.Y_AxisFootprint))
            {
                if (!  BestModule || Module.Y_AxisFootprint > BestModule->Y_AxisFootprint)
                {
                    BestModule = &Module;
                }
            }
        }

        if (! BestModule)
        {
            UE_LOG(LogTemp, Warning, TEXT("    No wall module fits remaining %d cells on edge %s at cell %d"), 
                SpaceLeft, *UEnum::GetValueAsString(Edge), CurrentCell);
            CurrentCell++;  // Skip this cell and try next
            continue;
        }

        // Load base mesh
        UStaticMesh* BaseMesh = BestModule->BaseMesh.LoadSynchronous();
        if (!BaseMesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("    Failed to load base mesh for wall module"));
            break;
        }

        // Calculate position for this wall segment
        FVector BasePosition = URoomGenerationHelpers:: CalculateWallPosition(
            Edge,
            CurrentCell,
            BestModule->Y_AxisFootprint,
            GridSize,
            CellSize,
            WallData->NorthWallOffsetX,
            WallData->SouthWallOffsetX,
            WallData->EastWallOffsetY,
            WallData->WestWallOffsetY
        );

        // Create base wall transform
        FTransform BaseTransform(WallRotation, BasePosition, FVector:: OneVector);

        // Store segment info for Middle/Top spawning
        FGeneratorWallSegment Segment;
        Segment.Edge = Edge;
        Segment. StartCell = CurrentCell;
        Segment. SegmentLength = BestModule->Y_AxisFootprint;
        Segment.BaseTransform = BaseTransform;
        Segment.BaseMesh = BaseMesh;
        Segment.WallModule = BestModule;

        PlacedBaseWallSegments.Add(Segment);

        UE_LOG(LogTemp, VeryVerbose, TEXT("    Tracked %d-cell base wall at cell %d"),
            BestModule->Y_AxisFootprint, CurrentCell);

        // Advance to next segment
        CurrentCell += BestModule->Y_AxisFootprint;
    }
}
#pragma endregion
