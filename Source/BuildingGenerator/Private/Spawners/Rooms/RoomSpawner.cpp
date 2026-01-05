// Fill out your copyright notice in the Description page of Project Settings.

#include "Spawners/Rooms/RoomSpawner.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Data/Generation/RoomGenerationTypes.h"
#include "Data/Room/DoorData.h" 
#include "Generators/Rooms/UniformRoomGenerator.h"
#include "RoomActors/Doorway.h"
#include "Utilities/Generation/RoomGenerationHelpers.h"
#include "Utilities/Spawners/RoomSpawnerHelpers.h" 

// Sets default values
ARoomSpawner::ARoomSpawner()
{
 	// Set this actor to call Tick() every frame (can turn off for performance)
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create debug helpers component
	DebugHelpers = CreateDefaultSubobject<UDebugHelpers>(TEXT("DebugHelpers"));

	// Bind delegate so DebugHelpers can request text components
	DebugHelpers->OnCreateTextComponent.BindUObject(this, &ARoomSpawner::CreateTextRenderComponent);

	// Bind destruction delegate
	DebugHelpers->OnDestroyTextComponent. BindUObject(this, &ARoomSpawner::DestroyTextRenderComponent);

	DoorwayActorClass = ADoorway::StaticClass();
	
	// Initialize flags
	bIsGenerated = false;
}
bool ARoomSpawner::EnsureGeneratorReady()
{
	UE_LOG(LogTemp, Warning, TEXT("RoomSpawner::EnsureGeneratorReady() called on base class - child should override!"));
	return false;
}

#if WITH_EDITOR
#pragma region In Editor Functions

#pragma region Floor Generation
void ARoomSpawner::GenerateRoomGrid()
{
	DebugHelpers->LogSectionHeader(TEXT("GENERATE ROOM GRID"));
	
	if (!EnsureGeneratorReady())
	{
		DebugHelpers->LogCritical(TEXT("Failed to initialize generator!"));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE ROOM GRID"));
		return;
	}
	
	DebugHelpers->bShowGrid = true;
	DebugHelpers->bShowCellStates = true;
	DebugHelpers->bShowCoordinates = true;
	DebugHelpers->bShowForcedEmptyRegions = true;
	DebugHelpers->bShowForcedEmptyCells = true;
	
	// CREATE DEBUG VISUALIZATION (heavy, debug-only)
	DebugHelpers->LogImportant(TEXT("Creating debug visualization... "));
	
	UpdateVisualization();
	
	// Set flag for external checks
	bIsGenerated = true;
	
	LogRoomStatistics();
	
	DebugHelpers->LogImportant(TEXT("Room grid generated successfully!"));
	DebugHelpers->LogSectionHeader(TEXT("GENERATE ROOM GRID"));
}

void ARoomSpawner::ClearRoomGrid()
{
	DebugHelpers->LogSectionHeader(TEXT("CLEAR ROOM GRID"));

	if (! RoomGenerator || !bIsGenerated)
	{
		DebugHelpers->LogImportant(TEXT("No room grid to clear. "));
		DebugHelpers->LogSectionHeader(TEXT("CLEAR ROOM GRID"));
		return;
	}
	
	DebugHelpers->bShowGrid = false;
	DebugHelpers->bShowCellStates = false;
	DebugHelpers->bShowCoordinates = false;
	DebugHelpers->bShowForcedEmptyRegions = false;
	DebugHelpers->bShowForcedEmptyCells = false;
	
	// Clear floor meshes first
	ClearFloorMeshes();
	
	// Clear wall meshes
	ClearWallMeshes();
	
	// Clear corner meshes
	ClearCornerMeshes();
	
	//Clear doorway meshes
	ClearDoorwayMeshes();
	
	// ✅ NEW: Clear doorway LAYOUT (not just meshes)
	if (RoomGenerator)
	{
		RoomGenerator->ClearPlacedDoorways();
	}

	// Clear the grid
	RoomGenerator->ClearGrid();
	bIsGenerated = false;
	
	// Clear coordinate text components (DebugHelpers manages them)
	DebugHelpers->ClearCoordinateTextComponents();
	
	// Clear debug drawings
	DebugHelpers->ClearDebugDrawings();

	DebugHelpers->LogImportant(TEXT("Room grid cleared. "));
	DebugHelpers->LogSectionHeader(TEXT("CLEAR ROOM GRID"));
}

void ARoomSpawner::GenerateFloorMeshes()
{
	DebugHelpers->LogSectionHeader(TEXT("GENERATE FLOOR MESHES"));
	
	if (!EnsureGeneratorReady())
	{
		DebugHelpers->LogCritical(TEXT("Failed to initialize generator! "));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE FLOOR MESHES"));
		return;
	}
	
	// CLEANUP: Clear existing floor meshes
	ClearFloorMeshes();
	
	// Generate Floor Layout
	DebugHelpers->LogImportant(TEXT("Generating floor layout..."));
	if (!RoomGenerator->GenerateFloor())
	{
		DebugHelpers->LogCritical(TEXT("Floor generation failed!"));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE FLOOR MESHES"));
		return;
	}
	
	// SPAWNING: Get placed meshes from generator
	const TArray<FPlacedMeshInfo>& PlacedMeshes = RoomGenerator->GetPlacedFloorMeshes();
	DebugHelpers->LogImportant(FString::Printf(TEXT("Spawning %d floor mesh instances... "), PlacedMeshes.Num()));
	
	// ✅ REMOVED: No need to get RoomOrigin anymore
	// ISM components are attached relatively, so instances are in local space
	
	// SPAWNING: Create ISM components and add instances
	for (const FPlacedMeshInfo& PlacedMesh : PlacedMeshes)
	{
		// Get or create ISM component for this mesh
		UInstancedStaticMeshComponent* ISM = URoomSpawnerHelpers::GetOrCreateISMComponent(
			this,
			PlacedMesh.MeshInfo.MeshAsset,
			FloorMeshComponents,
			TEXT("FloorISM_"),
			true
		);

		if (ISM)
		{
			// ✅ CHANGED: Pass zero offset - instances are in local space relative to ISM component
			int32 InstanceIndex = URoomSpawnerHelpers::SpawnMeshInstance(
				ISM, 
				PlacedMesh.LocalTransform,  // Actually local transform (misnamed)
				FVector::ZeroVector         // No offset needed
			);

			if (InstanceIndex >= 0)
			{
				DebugHelpers->LogVerbose(FString::Printf(
					TEXT("  Spawned floor mesh at grid position (%d, %d), instance %d"),
					PlacedMesh.GridPosition. X, PlacedMesh. GridPosition.Y, InstanceIndex));
			}
			else
			{
				DebugHelpers->LogVerbose(FString::Printf(
					TEXT("  Failed to spawn floor mesh at grid position (%d, %d)"),
					PlacedMesh.GridPosition.X, PlacedMesh.GridPosition.Y));
			}
		}
	}
	
	DebugHelpers->LogImportant(FString::Printf(TEXT("Floor meshes generated:  %d instances across %d unique meshes"),
		PlacedMeshes.Num(), FloorMeshComponents.Num()));
	DebugHelpers->LogSectionHeader(TEXT("GENERATE FLOOR MESHES"));
}

void ARoomSpawner::ClearFloorMeshes()
{
	// Clear all floor ISM components
	URoomSpawnerHelpers:: ClearISMComponentMap(FloorMeshComponents);

	// Clear generator data AND reset grid state
	if (RoomGenerator)
	{
		// Clear placed mesh array
		RoomGenerator->ClearPlacedFloorMeshes();
		
		// Reset grid cells (ECT_FloorMesh → back to target type)
		RoomGenerator->ResetGridCellStates();
	}
	DebugHelpers->LogImportant(TEXT("Floor meshes cleared"));
}
#pragma endregion

#pragma region Wall Generation
void ARoomSpawner::GenerateWallMeshes()
{
	DebugHelpers->LogSectionHeader(TEXT("GENERATE WALL MESHES"));

	if (!EnsureGeneratorReady())
	{
		DebugHelpers->LogCritical(TEXT("Failed to initialize generator!"));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE FLOOR MESHES"));
		return;
	}
	
	// Clear existing wall meshes
	ClearWallMeshes();
	
	// Generate wall layout (logic only)
	DebugHelpers->LogImportant(TEXT("Generating wall layout..."));
	if (! RoomGenerator->GenerateWalls())
	{
		DebugHelpers->LogCritical(TEXT("Wall generation failed!"));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE WALL MESHES"));
		return;
	}
	
	// Get placed walls and spawn
	const TArray<FPlacedWallInfo>& PlacedWalls = RoomGenerator->GetPlacedWalls();
	DebugHelpers->LogImportant(FString::Printf(TEXT("Spawning %d wall segments...  "), PlacedWalls.Num()));
	
	// Spawn wall segments
	for (const FPlacedWallInfo& PlacedWall : PlacedWalls) 
	{ 
		SpawnWallSegment(PlacedWall, FVector:: ZeroVector);  // ✅ Pass zero offset
	}
	
	DebugHelpers->LogImportant(TEXT("Wall meshes generated successfully!"));
	DebugHelpers->LogSectionHeader(TEXT("GENERATE WALL MESHES"));
}

void ARoomSpawner::SpawnWallSegment(const FPlacedWallInfo& PlacedWall, const FVector& RoomOrigin)
{
	// Delegate to helper
	URoomSpawnerHelpers::SpawnWallSegment(this, PlacedWall,WallMeshComponents, 
	RoomOrigin, TEXT("WallISM_"), DebugHelpers);
}

void ARoomSpawner::ClearWallMeshes()
{
	// Clear all wall ISM components
	URoomSpawnerHelpers::ClearISMComponentMap(WallMeshComponents);

	// Clear generator data
	if (RoomGenerator) { RoomGenerator->ClearPlacedWalls();	}
	DebugHelpers->LogImportant(TEXT("Wall meshes cleared"));
}
#pragma endregion

#pragma region Corner Generation
void ARoomSpawner::GenerateCornerMeshes()
{
	 DebugHelpers->LogSectionHeader(TEXT("GENERATE CORNER MESHES"));

    if (!EnsureGeneratorReady())
    {
        DebugHelpers->LogCritical(TEXT("Failed to initialize generator!"));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE CORNER MESHES"));
        return;
    }

    // Clear existing corners
    ClearCornerMeshes();

    // Generate corner layout (logic only)
    DebugHelpers->LogImportant(TEXT("Generating corner layout..."));
    if (!RoomGenerator->GenerateCorners())
    {
        DebugHelpers->LogCritical(TEXT("Corner generation failed!"));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE CORNER MESHES"));
        return;
    }

    // Get placed corners and spawn
    const TArray<FPlacedCornerInfo>& PlacedCorners = RoomGenerator->GetPlacedCorners();
    
    if (PlacedCorners.Num() == 0)
    {
        DebugHelpers->LogImportant(TEXT("No corners to spawn (no corner mesh assigned)"));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE CORNER MESHES"));
        return;
    }

    DebugHelpers->LogImportant(FString::Printf(TEXT("Spawning %d corner pieces..."), PlacedCorners.Num()));

    // Spawn corner meshes
    for (const FPlacedCornerInfo& PlacedCorner : PlacedCorners)
    {
        // Get or create ISM component for corner mesh
        UInstancedStaticMeshComponent* ISM = URoomSpawnerHelpers::GetOrCreateISMComponent(
            this,
            PlacedCorner. CornerMesh,
            CornerMeshComponents,
            TEXT("CornerISM_"),
            true
        );

        if (ISM)
        {
            int32 InstanceIndex = URoomSpawnerHelpers::SpawnMeshInstance(
                ISM,
                PlacedCorner.Transform,
                FVector::ZeroVector
            );

            if (InstanceIndex >= 0)
            {
                DebugHelpers->LogVerbose(FString::Printf(TEXT("  Spawned %s corner (instance %d)"),
                    *UEnum::GetValueAsString(PlacedCorner.Corner), InstanceIndex));
            }
            else
            {
                DebugHelpers->LogVerbose(FString::Printf(TEXT("  Failed to spawn %s corner"),
                    *UEnum::GetValueAsString(PlacedCorner. Corner)));
            }
        }
    }

    DebugHelpers->LogImportant(TEXT("Corner meshes generated successfully!"));
    DebugHelpers->LogSectionHeader(TEXT("GENERATE CORNER MESHES"));
}

void ARoomSpawner::ClearCornerMeshes()
{
	// Clear all corner ISM components
	URoomSpawnerHelpers::ClearISMComponentMap(CornerMeshComponents);

	// Clear generator data
	if (RoomGenerator)
	{
		RoomGenerator->ClearPlacedCorners();
	}

	DebugHelpers->LogImportant(TEXT("Corner meshes cleared"));
}
#pragma endregion

#pragma region Doorway Generation
void ARoomSpawner::GenerateDoorwayMeshes()
{
    DebugHelpers->LogSectionHeader(TEXT("GENERATE DOORWAY MESHES"));

    if (! EnsureGeneratorReady())
    {
        DebugHelpers->LogCritical(TEXT("Failed to initialize generator! "));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE DOORWAY MESHES"));
        return;
    }

    // Clear existing doorways
    ClearDoorwayMeshes();

    // ✅ CHANGED:   ALWAYS call GenerateDoorways() to recalculate transforms
    DebugHelpers->LogImportant(TEXT("Regenerating doorway transforms with current offsets... "));
    
    if (!RoomGenerator->GenerateDoorways())
    {
        DebugHelpers->LogCritical(TEXT("Doorway generation failed!"));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE DOORWAY MESHES"));
        return;
    }

    // Get doorways (from cache or newly generated)
    const TArray<FPlacedDoorwayInfo>& FinalDoorways = RoomGenerator->GetPlacedDoorways();
    
    if (FinalDoorways. Num() == 0)
    {
        DebugHelpers->LogImportant(TEXT("No doorways to spawn (none configured)"));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE DOORWAY MESHES"));
        return;
    }

    DebugHelpers->LogImportant(FString::Printf(TEXT("Spawning %d doorway actors... "), FinalDoorways.Num()));

    // Validate doorway actor class
    if (! DoorwayActorClass)
    {
        DebugHelpers->LogCritical(TEXT("DoorwayActorClass is not set!"));
        DebugHelpers->LogSectionHeader(TEXT("GENERATE DOORWAY MESHES"));
        return;
    }

    int32 DoorwaysSpawned = 0;
    int32 DoorwaysSkipped = 0;
	
	

    for (const FPlacedDoorwayInfo& PlacedDoor : FinalDoorways)
    {
        // Validate door data
        if (!PlacedDoor.DoorData)
        {
            DebugHelpers->LogVerbose(TEXT("  Doorway has null DoorData - skipping"));
            DoorwaysSkipped++;
            continue;
        }

        // Calculate world transform (room space → world space)
        FTransform LocalTransform  = PlacedDoor.FrameTransform;

        // Spawn parameters
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Spawn doorway actor
        ADoorway* DoorwayActor = GetWorld()->SpawnActor<ADoorway>(
            DoorwayActorClass,
            LocalTransform,
            SpawnParams
        );

        if (DoorwayActor)
        {
        	DoorwayActor->AttachToActor(this, FAttachmentTransformRules:: KeepRelativeTransform);
        	
            // Initialize doorway with configuration
            DoorwayActor->InitializeDoorway(
                PlacedDoor.DoorData,
                PlacedDoor.Edge,
                PlacedDoor.bIsStandardDoorway
            );

            // Store reference
            SpawnedDoorwayActors.Add(DoorwayActor);
            DoorwaysSpawned++;

            FString DoorType = PlacedDoor.bIsStandardDoorway ? TEXT("Standard") : TEXT("Manual");
            DebugHelpers->LogVerbose(FString::Printf(TEXT("  Spawned %s doorway on edge %s"),
                *DoorType, *UEnum::GetValueAsString(PlacedDoor.Edge)));
        }
        else
        {
            DebugHelpers->LogVerbose(FString::Printf(TEXT("  Failed to spawn doorway on edge %s"),
                *UEnum::GetValueAsString(PlacedDoor.Edge)));
            DoorwaysSkipped++;
        }
    }

    DebugHelpers->LogImportant(FString::Printf(TEXT("Doorway spawning complete:  %d actors spawned, %d skipped"),
        DoorwaysSpawned, DoorwaysSkipped));
    DebugHelpers->LogSectionHeader(TEXT("GENERATE DOORWAY MESHES"));
}

void ARoomSpawner::ClearDoorwayMeshes()
{
	// ✅ CHANGED:  Destroy spawned doorway actors instead of clearing ISM components
    
	for (ADoorway* DoorwayActor : SpawnedDoorwayActors)
	{
		if (IsValid(DoorwayActor))
		{
			DoorwayActor->Destroy();
		}
	}

	SpawnedDoorwayActors.Empty();
	
	// Layout is cached and persists until ClearRoomGrid()
	// Transforms will be recalculated with current offsets on next spawn

	DebugHelpers->LogImportant(TEXT("Doorway actors cleared (layout preserved, offsets will update on next spawn)"));
}

// Add these at the end of the file (or with your other generation functions):

void ARoomSpawner::GenerateCeilingMeshes()
{
	DebugHelpers->LogSectionHeader(TEXT("GENERATE CEILING MESHES"));
	
	if (!EnsureGeneratorReady())
	{
		DebugHelpers->LogCritical(TEXT("Failed to initialize generator!"));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE CEILING MESHES"));
		return;
	}
	
	// CLEANUP: Clear existing ceiling meshes
	ClearCeilingMeshes();
	
	// Generate Ceiling Layout
	DebugHelpers->LogImportant(TEXT("Generating ceiling layout... "));
	if (! RoomGenerator->GenerateCeiling())
	{
		DebugHelpers->LogCritical(TEXT("Ceiling generation failed!"));
		DebugHelpers->LogSectionHeader(TEXT("GENERATE CEILING MESHES"));
		return;
	}
	
	// SPAWNING: Get placed meshes from generator
	const TArray<FPlacedCeilingInfo>& PlacedMeshes = RoomGenerator->GetPlacedCeilingTiles();
	DebugHelpers->LogImportant(FString::Printf(TEXT("Spawning %d ceiling mesh instances... "), PlacedMeshes.Num()));
	
	// SPAWNING: Create ISM components and add instances
	for (const FPlacedCeilingInfo& PlacedMesh : PlacedMeshes)
	{
		// Get or create ISM component for this mesh
		UInstancedStaticMeshComponent* ISM = URoomSpawnerHelpers::GetOrCreateISMComponent(this,
		PlacedMesh.MeshInfo.MeshAsset, CeilingMeshComponents, TEXT("CeilingISM_"), true);

		if (ISM)
		{
			int32 InstanceIndex = URoomSpawnerHelpers:: SpawnMeshInstance(ISM, PlacedMesh.LocalTransform, FVector::ZeroVector);

			if (InstanceIndex >= 0)
			{
				DebugHelpers->LogVerbose(FString::Printf(TEXT("  Spawned ceiling mesh at grid position (%d, %d), instance %d"),
				PlacedMesh.GridCoordinate.X, PlacedMesh.GridCoordinate. Y, InstanceIndex));
			}
			else
			{
				DebugHelpers->LogVerbose(FString::Printf(TEXT("  Failed to spawn ceiling mesh at grid position (%d, %d)"),
				PlacedMesh.GridCoordinate. X, PlacedMesh. GridCoordinate.Y));
			}
		}
	}
	
	DebugHelpers->LogImportant(FString::Printf(TEXT("Ceiling meshes generated:  %d instances across %d unique meshes"),
	PlacedMeshes.Num(), CeilingMeshComponents.Num()));
	DebugHelpers->LogSectionHeader(TEXT("GENERATE CEILING MESHES"));
}

void ARoomSpawner::ClearCeilingMeshes()
{
	// Clear all ceiling ISM components
	URoomSpawnerHelpers::ClearISMComponentMap(CeilingMeshComponents);

	// Clear generator data
	if (RoomGenerator)
	{
		// Clear placed mesh array
		RoomGenerator->ClearPlacedCeiling();
	}
	
	DebugHelpers->LogImportant(TEXT("Ceiling meshes cleared"));
}

#pragma region Doorway Side Fill Spawning


#pragma endregion
#pragma endregion

void ARoomSpawner::RefreshVisualization()
{
	DebugHelpers->LogImportant(TEXT("Refreshing visualization..."));

	if (!bIsGenerated || !RoomGenerator)
	{
		DebugHelpers->LogImportant(TEXT("No room to visualize.  Generate a room first.")); return;
	}

	// Clear existing drawings
	DebugHelpers->ClearDebugDrawings();

	// Redraw with current settings
	UpdateVisualization();

	DebugHelpers->LogImportant(TEXT("Visualization refreshed. "));
}
#pragma endregion

#pragma region Debug Functions
void ARoomSpawner::ToggleCoordinates()
{
	DebugHelpers->bShowCoordinates = !DebugHelpers->bShowCoordinates;
	DebugHelpers->LogImportant(FString::Printf(TEXT("Coordinates display: %s"), 
		DebugHelpers->bShowCoordinates ? TEXT("ON") : TEXT("OFF")));
    
	if (!bIsGenerated || !RoomGenerator)
	{
		DebugHelpers->LogImportant(TEXT("No room to visualize. Generate a room first. "));
		return;
	}
    
	// ========================================================================
	// Coordinates use UTextRenderComponent (not debug shapes)
	// They're managed separately and don't require ClearDebugDrawings()
	// Just call the coordinate function directly
	// ========================================================================
    
	// Get grid data
	FVector RoomOrigin = GetActorLocation();
	FIntPoint GridSize = RoomGenerator->GetGridSize();
	float CellSize = RoomGenerator->GetCellSize();
    
	// Toggle coordinates (function handles clearing internally)
	DebugHelpers->DrawGridCoordinatesWithTextComponents(GridSize, CellSize, RoomOrigin);
}

void ARoomSpawner:: ToggleGrid()
{
	DebugHelpers->bShowGrid = !DebugHelpers->bShowGrid;
	DebugHelpers->LogImportant(FString::Printf(TEXT("Grid outline display: %s"), 
		DebugHelpers->bShowGrid ? TEXT("ON") : TEXT("OFF")));
	
	RefreshVisualization();
}

void ARoomSpawner:: ToggleCellStates()
{
	  
	// Toggle cell state visualization
	DebugHelpers->bShowCellStates = ! DebugHelpers->bShowCellStates;
	DebugHelpers->bShowForcedEmptyRegions = DebugHelpers->bShowCellStates;
	DebugHelpers->bShowForcedEmptyCells = DebugHelpers->bShowCellStates;
    
	// ✅ NEW: Also toggle grid lines with cell states
	// Grid provides context for understanding cell visualization
	DebugHelpers->bShowGrid = DebugHelpers->bShowCellStates;
    
	DebugHelpers->LogImportant(FString::Printf(TEXT("Cell states display: %s"), 
		DebugHelpers->bShowCellStates ?  TEXT("ON") : TEXT("OFF")));
    
	if (!bIsGenerated || !RoomGenerator)
	{
		DebugHelpers->LogImportant(TEXT("No room to visualize. Generate a room first."));
		return;
	}
    
	// Use standard refresh (clear and redraw everything based on toggle states)
	RefreshVisualization();
}

UTextRenderComponent* ARoomSpawner::CreateTextRenderComponent(FVector WorldPosition, FString Text, FColor Color, float Scale)
{
	// Create new text render component
	UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(this);
	
	if (! TextComp) return nullptr;

	// Register and attach component
	TextComp->RegisterComponent();
	TextComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	// Set text properties
	TextComp->SetText(FText::FromString(Text));
	TextComp->SetWorldSize(Scale * 10.0f); // Scale for visibility
	TextComp->SetTextRenderColor(Color);
	TextComp->SetHorizontalAlignment(EHTA_Center);
	TextComp->SetVerticalAlignment(EVRTA_TextCenter);
	
	// Set world location
	TextComp->SetWorldLocation(WorldPosition);
	
	// Rotate to face upward (readable from above in editor)
	TextComp->SetWorldRotation(FRotator(45.0f, 180.0f, 0.0f));

	// Make visible in editor
	TextComp->SetVisibility(true);
	TextComp->SetHiddenInGame(true); // Show in PIE too
	
	return TextComp;
}

void ARoomSpawner::DestroyTextRenderComponent(UTextRenderComponent* TextComp)
{
	if (!TextComp || !TextComp->IsValidLowLevel()) return;

	// Destroy the component
	TextComp->DestroyComponent();
}

void ARoomSpawner::LogRoomStatistics()
{
	if (!RoomGenerator) return;

	DebugHelpers->LogSectionHeader(TEXT("ROOM STATISTICS"));
	
	FIntPoint GridSize = RoomGenerator->GetGridSize();
	int32 TotalCells = RoomGenerator->GetTotalCellCount();
	int32 EmptyCells = RoomGenerator->GetCellCountByType(EGridCellType::ECT_Empty);
	int32 OccupiedCells = RoomGenerator->GetCellCountByType(EGridCellType::ECT_FloorMesh);
	float OccupancyPercent = RoomGenerator->GetOccupancyPercentage();

	DebugHelpers->LogStatistic(TEXT("Grid Size"), FString::Printf(TEXT("%d x %d"), GridSize.X, GridSize.Y));
	DebugHelpers->LogStatistic(TEXT("Total Cells"), TotalCells);
	DebugHelpers->LogStatistic(TEXT("Empty Cells"), EmptyCells);
	DebugHelpers->LogStatistic(TEXT("Occupied Cells"), OccupiedCells);
	DebugHelpers->LogStatistic(TEXT("Occupancy"), OccupancyPercent);
	
	DebugHelpers->LogSectionHeader(TEXT("ROOM STATISTICS"));
}

void ARoomSpawner::LogFloorStatistics()
{
	if (!RoomGenerator) return;

	DebugHelpers->LogSectionHeader(TEXT("FLOOR STATISTICS"));

	int32 LargeTiles, MediumTiles, SmallTiles, FillerTiles;
	RoomGenerator->GetFloorStatistics(LargeTiles, MediumTiles, SmallTiles, FillerTiles);

	int32 TotalTiles = LargeTiles + MediumTiles + SmallTiles + FillerTiles;
	float Coverage = RoomGenerator->GetOccupancyPercentage();
	int32 EmptyCells = RoomGenerator->GetCellCountByType(EGridCellType:: ECT_Empty);

	DebugHelpers->LogStatistic(TEXT("Large Tiles (400x400)"), LargeTiles);
	DebugHelpers->LogStatistic(TEXT("Medium Tiles (200x200)"), MediumTiles);
	DebugHelpers->LogStatistic(TEXT("Small Tiles (100x)"), SmallTiles);
	DebugHelpers->LogStatistic(TEXT("Filler Tiles"), FillerTiles);
	DebugHelpers->LogStatistic(TEXT("Total Tiles Placed"), TotalTiles);
	DebugHelpers->LogStatistic(TEXT("Floor Coverage"), Coverage);
	DebugHelpers->LogStatistic(TEXT("Empty Cells Remaining"), EmptyCells);

	DebugHelpers->LogSectionHeader(TEXT("FLOOR STATISTICS"));
}

void ARoomSpawner::UpdateVisualization()
{
	if (!RoomGenerator) return;

	// Get grid data
	FVector RoomOrigin = GetActorLocation();
	FIntPoint GridSize = RoomGenerator->GetGridSize();
	float CellSize = RoomGenerator->GetCellSize();
	const TArray<EGridCellType>& GridState = RoomGenerator->GetGridState();
	DebugHelpers->DrawGrid(GridSize, GridState, CellSize, RoomOrigin);

	// Draw forced empty regions (if any)
	if (RoomData && RoomData->ForcedEmptyRegions.Num() > 0)
	{ DebugHelpers->DrawForcedEmptyRegions(RoomData->ForcedEmptyRegions, GridSize, CellSize, RoomOrigin); }

	// Draw forced empty cells (if any)
	if (RoomData && RoomData->ForcedEmptyFloorCells.Num() > 0)
	{ DebugHelpers->DrawForcedEmptyCells(RoomData->ForcedEmptyFloorCells, GridSize, CellSize, RoomOrigin);}
	DebugHelpers->LogVerbose(TEXT("Visualization updated."));
}
#pragma endregion
#endif // WITH_EDITOR