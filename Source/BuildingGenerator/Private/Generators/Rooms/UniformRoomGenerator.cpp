// Fill out your copyright notice in the Description page of Project Settings.


#include "Generators/Rooms/UniformRoomGenerator.h"

#include "Utilities/Generation/RoomGenerationHelpers.h"


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
	if (!bIsInitialized)
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateWalls - Generator not initialized! ")); return false; }

	if (!RoomData || RoomData->WallStyleData.IsNull())
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateWalls - WallStyleData not assigned!")); return false; }

	WallData = RoomData->WallStyleData.LoadSynchronous();
	if (!WallData || WallData->AvailableWallModules.Num() == 0)
	{ UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateWalls - No wall modules defined!"));	return false; }
	
	// Clear previous data
	ClearPlacedWalls();
	PlacedBaseWallSegments.Empty();  // ✅ Clear tracking array

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateWalls - Starting wall generation"));

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

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateWalls - Base walls tracked:  %d segments"), PlacedBaseWallSegments.Num());

	// PASS 3: Spawn middle layers using socket-based stacking
	SpawnMiddleWallLayers();

	// PASS 4: Spawn top layer using socket-based stacking
	SpawnTopWallLayer();

	UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateWalls - Complete.  Total wall records: %d"), PlacedWallMeshes.Num());

	return true;
}

bool UUniformRoomGenerator::GenerateCorners()
{
 if (!bIsInitialized)
    { UE_LOG(LogTemp, Error, TEXT("URoomGenerator:: GenerateCorners - Generator not initialized! ")); return false; }

    if (! RoomData || RoomData->WallStyleData. IsNull())
    { UE_LOG(LogTemp, Error, TEXT("URoomGenerator:: GenerateCorners - WallStyleData not assigned!")); return false; }

    WallData = RoomData->WallStyleData.LoadSynchronous();
    if (!WallData)
    { UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateCorners - Failed to load WallStyleData!")); return false; }

    // Clear previous corners
    ClearPlacedCorners();

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateCorners - Starting corner generation"));

    // Load corner mesh (required)
    if (WallData->DefaultCornerMesh.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateCorners - No default corner mesh defined, skipping corners"));
        return true; 
    }

    UStaticMesh* CornerMesh = WallData->DefaultCornerMesh.LoadSynchronous();
    if (!CornerMesh)
    { UE_LOG(LogTemp, Warning, TEXT("URoomGenerator::GenerateCorners - Failed to load corner mesh")); return false;		}

     
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

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateCorners - Complete.  Placed %d corners"), PlacedCornerMeshes.Num());

    return true;
}

bool UUniformRoomGenerator::GenerateDoorways()
{
 if (!bIsInitialized)
    { UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateDoorways - Generator not initialized!  ")); return false; }

    if (!RoomData)
    { UE_LOG(LogTemp, Error, TEXT("URoomGenerator::GenerateDoorways - RoomData is null! ")); return false; }
     
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

    UE_LOG(LogTemp, Log, TEXT("URoomGenerator::GenerateDoorways - Complete.   Cached %d layouts, placed %d doorways"),
        CachedDoorwayLayouts.Num(), PlacedDoorwayMeshes.Num());

    return true;
}

bool UUniformRoomGenerator::GenerateCeiling()
{
	// Will implement in Step 7
	return false;
}