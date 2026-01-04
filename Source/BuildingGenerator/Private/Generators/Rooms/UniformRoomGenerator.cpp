// Fill out your copyright notice in the Description page of Project Settings.


#include "Generators/Rooms/UniformRoomGenerator.h"

#include "Utilities/Generation/RoomGenerationHelpers.h"

#pragma region Room Grid Management
void UUniformRoomGenerator::CreateGrid()
{
	if (!bIsInitialized) 
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::CreateGrid - Generator not initialized!")); return; }

	UE_LOG(LogTemp, Log, TEXT("UniformRoomGenerator: Creating uniform rectangular grid..."));
    
	// Initialize grid state array (all floor cells for uniform room)
	GridState.SetNum(GridSize.X * GridSize.Y);
	for (EGridCellType& Cell : GridState) { Cell = EGridCellType::ECT_Empty; }
    
	// Log statistics
	int32 TotalCells = GetTotalCellCount();
	UE_LOG(LogTemp, Log, TEXT("UniformRoomGenerator: Grid created - %d x %d (%d cells)"), GridSize.X, GridSize.Y, TotalCells);
}
#pragma endregion

#pragma region Floor Generation
bool UUniformRoomGenerator::GenerateFloor()
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
	ClearPlacedFloorMeshes();
	
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
#pragma endregion

#pragma region Wall Generation
bool UUniformRoomGenerator::GenerateWalls()
{
	if (!bIsInitialized)
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateWalls - Generator not initialized! ")); return false; }

	if (!RoomData || RoomData->WallStyleData.IsNull())
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateWalls - WallStyleData not assigned!")); return false; }

	WallData = RoomData->WallStyleData.LoadSynchronous();
	if (!WallData || WallData->AvailableWallModules.Num() == 0)
	{ UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateWalls - No wall modules defined!"));	return false; }
	
	// Clear previous data
	ClearPlacedWalls();
	PlacedBaseWallSegments.Empty();  // ✅ Clear tracking array

	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateWalls - Starting wall generation"));

	// PHASE 0:   GENERATE DOORWAYS FIRST (Before any walls are placed!)
	UE_LOG(LogTemp, Log, TEXT("  Phase 0: Generating doorways"));
	if (! GenerateDoorways())
	{ UE_LOG(LogTemp, Warning, TEXT("  Doorway generation failed, continuing with walls")); }
	else
	{ UE_LOG(LogTemp, Log, TEXT("  Doorways generated:   %d"), PlacedDoorwayMeshes. Num()); }
	
	// PHASE 1: FORCED WALL PLACEMENTS
	int32 ForcedCount = ExecuteForcedWallPlacements();
	if (ForcedCount > 0) UE_LOG(LogTemp, Log, TEXT("  Phase 0: Placed %d forced walls"), ForcedCount);
	
	// PHASE 2: Generate base walls for each edge
	FillWallEdge(EWallEdge::North);
	FillWallEdge(EWallEdge::South);
	FillWallEdge(EWallEdge::East);
	FillWallEdge(EWallEdge::West);

	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateWalls - Base walls tracked:  %d segments"), PlacedBaseWallSegments.Num());

	// PASS 3: Spawn middle layers using socket-based stacking
	SpawnMiddleWallLayers();

	// PASS 4: Spawn top layer using socket-based stacking
	SpawnTopWallLayer();

	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateWalls - Complete.  Total wall records: %d"), PlacedWallMeshes.Num());

	return true;
}
#pragma endregion

#pragma region Corner Generation
bool UUniformRoomGenerator::GenerateCorners()
{
 if (!bIsInitialized)
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator:: GenerateCorners - Generator not initialized! ")); return false; }

    if (! RoomData || RoomData->WallStyleData. IsNull())
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator:: GenerateCorners - WallStyleData not assigned!")); return false; }

    WallData = RoomData->WallStyleData.LoadSynchronous();
    if (!WallData)
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateCorners - Failed to load WallStyleData!")); return false; }

    // Clear previous corners
    ClearPlacedCorners();

    UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateCorners - Starting corner generation"));

    // Load corner mesh (required)
    if (WallData->DefaultCornerMesh.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("UUniformRoomGenerator::GenerateCorners - No default corner mesh defined, skipping corners"));
        return true; 
    }

    UStaticMesh* CornerMesh = WallData->DefaultCornerMesh.LoadSynchronous();
    if (!CornerMesh)
    { UE_LOG(LogTemp, Warning, TEXT("UUniformRoomGenerator::GenerateCorners - Failed to load corner mesh")); return false;		}

     
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
        *CornerData.Name,  *FinalPosition.ToString(), CornerData. Rotation.Roll, CornerData.Rotation. Pitch, CornerData.Rotation.Yaw);
    }

    UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateCorners - Complete.  Placed %d corners"), PlacedCornerMeshes.Num());

    return true;
}
#pragma endregion

#pragma region Doorway Generation
bool UUniformRoomGenerator::GenerateDoorways()
{
 if (!bIsInitialized)
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateDoorways - Generator not initialized!  ")); return false; }

    if (!RoomData)
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateDoorways - RoomData is null! ")); return false; }
     
    // CHECK FOR CACHED LAYOUT
	if (CachedDoorwayLayouts.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateDoorways - Using cached layout (%d doorways), recalculating transforms"),
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
        
        UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateDoorways - Transforms recalculated with current offsets"));
        return true;
    }
	
    // NO CACHE - GENERATE NEW LAYOUT
	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateDoorways - Generating new doorway layout"));

    // Clear both layout and transforms
    PlacedDoorwayMeshes.Empty();
    CachedDoorwayLayouts.Empty();

    int32 ManualDoorwaysPlaced = 0;
    int32 AutomaticDoorwaysPlaced = 0;

     
    // PHASE 1: Process Manual Doorway Placements
	for (const FFixedDoorLocation& ForcedDoor : RoomData->ForcedDoorways)
    {
        // Validate door data
         DoorData = ForcedDoor.DoorData ?  ForcedDoor.DoorData : RoomData->DefaultDoorData;
        
        if (!DoorData)
        { UE_LOG(LogTemp, Warning, TEXT("  Forced doorway has no DoorData, skipping")); continue; }
    	
		int32 DoorWidth = DoorData->GetTotalDoorwayWidth();
    	UE_LOG(LogTemp, Log, TEXT("  Manual doorway:  Edge=%s, FrameFootprint=%d, SideFills=%s, TotalWidth=%d"),
    	*UEnum::GetValueAsString(ForcedDoor.WallEdge), DoorData->FrameFootprintY, *UEnum:: GetValueAsString(DoorData->SideFillType), DoorWidth);
        // Validate bounds
        TArray<FIntPoint> EdgeCells = URoomGenerationHelpers::GetEdgeCellIndices(ForcedDoor.WallEdge, GridSize);
        
        if (ForcedDoor.StartCell < 0 || ForcedDoor.StartCell + DoorWidth > EdgeCells.Num())
        { UE_LOG(LogTemp, Warning, TEXT("  Forced doorway out of bounds, skipping")); continue; }

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
            UE_LOG(LogTemp, Log, TEXT("  Using manual edge:   %s"), *UEnum::GetValueAsString(RoomData->StandardDoorwayEdge));
        }
        else if (RoomData->bMultipleDoorways)
        {
            int32 NumDoorways = FMath:: Clamp(RoomData->NumAutomaticDoorways, 2, 4);
            
            TArray<EWallEdge> AllEdges = { EWallEdge:: North, EWallEdge::  South, EWallEdge:: East, EWallEdge:: West };
            
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
            FRandomStream Stream(FMath::Rand());
            TArray<EWallEdge> AllEdges = 
            { EWallEdge::North, EWallEdge::South, 
				EWallEdge:: East, EWallEdge:: West 
            };
            EWallEdge ChosenEdge = AllEdges[Stream.RandRange(0, AllEdges.Num() - 1)];
            EdgesToUse.Add(ChosenEdge);
            
            UE_LOG(LogTemp, Log, TEXT("  Using random edge:  %s"), *UEnum::GetValueAsString(ChosenEdge));
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
                        UE_LOG(LogTemp, Warning, TEXT("  Doorway on %s would overlap, skipping"), *UEnum::  GetValueAsString(ChosenEdge));
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

    UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateDoorways - Complete.   Cached %d layouts, placed %d doorways"),
        CachedDoorwayLayouts.Num(), PlacedDoorwayMeshes.Num());

    return true;
}
#pragma endregion

#pragma region Ceiling Generation
bool UUniformRoomGenerator::GenerateCeiling()
{
	 if (! bIsInitialized)
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateCeiling - Generator not initialized!  ")); return false; }

    if (! RoomData || RoomData->CeilingStyleData.IsNull())
    { UE_LOG(LogTemp, Warning, TEXT("UUniformRoomGenerator::GenerateCeiling - No CeilingStyleData assigned")); return false; }

    CeilingData = RoomData->CeilingStyleData.LoadSynchronous();
    if (!CeilingData)
    { UE_LOG(LogTemp, Error, TEXT("UUniformRoomGenerator::GenerateCeiling - Failed to load CeilingStyleData")); return false; }

	if (CeilingData->CeilingTilePool.Num() == 0)
	{ UE_LOG(LogTemp, Warning, TEXT("UUniformRoomGenerator::GenerateCeiling - No tiles in CeilingTilePool! ")); return false; }
	
    // Clear previous ceiling data
    ClearPlacedCeiling();

    UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateCeiling - Starting ceiling generation"));

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
	{ UE_LOG(LogTemp, Log, TEXT("  Phase 0: Placed %d forced ceiling tiles"), ForcedCount); }
	
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

	UE_LOG(LogTemp, Log, TEXT("UUniformRoomGenerator::GenerateCeiling - Complete:  %d large, %d medium, %d small, %d filler = %d total"),
		CeilingLargeTilesPlaced, CeilingMediumTilesPlaced, CeilingSmallTilesPlaced, CeilingFillerTilesPlaced, PlacedCeilingTiles. Num());

	return true;
}
#pragma endregion