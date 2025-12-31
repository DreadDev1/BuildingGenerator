// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawners/Rooms/PresetRoomSpawner.h"
#include "Generators/Room/PresetRoomGenerator.h"
#include "Data/Room/RoomData.h"
#include "Data/Presets/RoomPreset.h"
#include "Utilities/Generation/RoomGenerationHelpers.h"
#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"


APresetRoomSpawner::APresetRoomSpawner()
{
	// Default values for preset room spawner
	bShowRegionBoundaries = true;
	bShowRegionNames = true;
	RegionBoundaryHeight = 10.0f;
	RegionBoundaryThickness = 3.0f;
}

#pragma region Room Generation Properties
bool APresetRoomSpawner::EnsureGeneratorReady()
{
	// Validate RoomData
	if (!RoomData)
	{ DebugHelpers->LogCritical(TEXT("RoomData is not assigned!")); return false; }

	if (RoomGridSize.X < 4 || RoomGridSize.Y < 4)
	{ DebugHelpers->LogCritical(TEXT("GridSize is too small (min 4x4)!")); return false; }

	// KEY DIFFERENCE: Create PresetRoomGenerator instead of RoomGenerator

	// Create PresetRoomGenerator if needed
	if (!RoomGenerator)
	{
		DebugHelpers->LogVerbose(TEXT("Creating PresetRoomGenerator..."));
		RoomGenerator = NewObject<UPresetRoomGenerator>(this, TEXT("PresetRoomGenerator"));
		
		if (!RoomGenerator)
		{ DebugHelpers->LogCritical(TEXT("Failed to create PresetRoomGenerator! ")); return false; }
	}

	// Initialize if needed
	if (!RoomGenerator->IsInitialized())
	{
		DebugHelpers->LogVerbose(TEXT("Initializing PresetRoomGenerator..."));
		
		if (!RoomGenerator->Initialize(RoomData, RoomGridSize))
		{ DebugHelpers->LogCritical(TEXT("Failed to initialize PresetRoomGenerator! ")); return false; }

		DebugHelpers->LogVerbose(TEXT("Creating grid cells..."));
		RoomGenerator->CreateGrid();
	}

	return true;
}

void APresetRoomSpawner::UpdateVisualization()
{
	UE_LOG(LogTemp, Warning, TEXT("=== PresetRoomSpawner::UpdateVisualization CALLED ===")); // ADD THIS
	
	// Call base implementation first (draws grid, coordinates, cell states)
	Super::UpdateVisualization();

	UE_LOG(LogTemp, Warning, TEXT("Base visualization complete, checking region settings...")); // ADD THIS
	UE_LOG(LogTemp, Warning, TEXT("  bShowRegionBoundaries: %s"), bShowRegionBoundaries ? TEXT("TRUE") : TEXT("FALSE")); // ADD THIS
	UE_LOG(LogTemp, Warning, TEXT("  bShowRegionNames:  %s"), bShowRegionNames ? TEXT("TRUE") : TEXT("FALSE")); // ADD THIS

	// Add region-specific visualization
	if (bShowRegionBoundaries || bShowRegionNames)
	{
		DrawRegionBoundaries();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Skipping region boundaries (both flags false)")); // ADD THIS
	}
}
#pragma endregion

#pragma region Debug Drawing Functions
void APresetRoomSpawner::DrawRegionBoundaries()
{
	UE_LOG(LogTemp, Warning, TEXT("DrawRegionBoundaries() - START")); // ADD THIS
	
	if (!RoomGenerator || ! RoomData)
	{
		UE_LOG(LogTemp, Error, TEXT("  FAILED: RoomGenerator=%s, RoomData=%s"), 
			RoomGenerator ? TEXT("Valid") : TEXT("NULL"),
			RoomData ? TEXT("Valid") : TEXT("NULL")); // ADD THIS
		return;
	}

	// Check if using preset layout
	if (! RoomData->bUsePresetLayout || RoomData->PresetLayout.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("  FAILED: Not using preset layout (bUsePresetLayout=%s, PresetLayout=%s)"),
			RoomData->bUsePresetLayout ? TEXT("TRUE") : TEXT("FALSE"),
			RoomData->PresetLayout. IsNull() ? TEXT("NULL") : TEXT("Valid")); // ADD THIS
		return;
	}

	URoomPreset* PresetLayout = RoomData->PresetLayout. LoadSynchronous();
	if (!PresetLayout)
	{
		UE_LOG(LogTemp, Error, TEXT("  FAILED: Could not load PresetLayout")); // ADD THIS
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("  Drawing %d region boundaries"), PresetLayout->Regions.Num()); // ADD THIS

	// Draw each region
	for (int32 i = 0; i < PresetLayout->Regions.Num(); ++i)
	{
		const FPresetRegion& Region = PresetLayout->Regions[i];
		
		UE_LOG(LogTemp, Warning, TEXT("  Region [%d]:  '%s' - Start(%d,%d) End(%d,%d)"),
			i, *Region.RegionName,
			Region.StartCell.X, Region.StartCell.Y,
			Region.EndCell.X, Region.EndCell.Y); // ADD THIS

		if (bShowRegionBoundaries)
		{
			UE_LOG(LogTemp, Warning, TEXT("    Drawing boundary...")); // ADD THIS
			DrawRegionBoundary(Region);
		}

		if (bShowRegionNames)
		{
			UE_LOG(LogTemp, Warning, TEXT("    Drawing label...")); // ADD THIS
			DrawRegionLabel(Region);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("DrawRegionBoundaries() - END")); // ADD THIS
}

void APresetRoomSpawner::DrawRegionBoundary(const FPresetRegion& Region)
{
	if (!RoomGenerator) return;

	float CellSize = RoomGenerator->GetCellSize();
	FVector RoomOrigin = GetActorLocation();

	// Calculate world positions for region corners
	FVector StartWorld = RoomOrigin + FVector( 
Region.StartCell.X * CellSize,Region.StartCell.Y * CellSize, RegionBoundaryHeight);
	
	// +1 because EndCell is inclusive
	FVector EndWorld = RoomOrigin + FVector(
(Region.EndCell.X + 1) * CellSize, (Region.EndCell. Y + 1) * CellSize, RegionBoundaryHeight);

	// Draw box outline
	FVector BoxCenter = (StartWorld + EndWorld) * 0.5f;
	FVector BoxExtent = (EndWorld - StartWorld) * 0.5f;

	DrawDebugBox(GetWorld(), BoxCenter,	BoxExtent, Region.DebugColor, true, -1.0f, 0, RegionBoundaryThickness);
}

void APresetRoomSpawner::DrawRegionLabel(const FPresetRegion& Region)
{
	if (!RoomGenerator)
	{ 
		UE_LOG(LogTemp, Error, TEXT("      DrawRegionLabel FAILED: No RoomGenerator")); 
		return; 
	}

	float CellSize = RoomGenerator->GetCellSize();
	FVector RoomOrigin = GetActorLocation();

	// Calculate world position for region center
	FVector RegionCenterWorld = RoomOrigin + FVector(
		(Region.StartCell.X + Region.EndCell.X + 1) * CellSize * 0.5f, 
		(Region.StartCell.Y + Region.EndCell.Y + 1) * CellSize * 0.5f,
		RegionBoundaryHeight + 50.0f); // Higher up for visibility

	// CREATE REGION NAME LABEL (using base class function)
	UTextRenderComponent* NameText = CreateTextRenderComponent(
		RegionCenterWorld,
		Region.RegionName,
		Region.DebugColor,
		RegionNameTextScale
	);

	// ✅ STORE IT! 
	if (NameText)
	{
		RegionLabelComponents. Add(NameText);
		UE_LOG(LogTemp, Log, TEXT("      Added region name label to array"));
	}

	// CREATE REGION INFO LABEL (size, priority)
	FIntPoint RegionSize = URoomGenerationHelpers::GetRegionSize(Region);
	FString InfoText = FString:: Printf(TEXT("Size:  %dx%d | Priority: %d"),
		RegionSize.X, RegionSize.Y, Region.FillPriority);

	UTextRenderComponent* InfoTextComp = CreateTextRenderComponent(
		RegionCenterWorld - FVector(0, 0, 30.0f), // Slightly below name
		InfoText,
		FColor:: White,
		RegionInfoTextScale
	);

	// ✅ STORE IT TOO!
	if (InfoTextComp)
	{
		RegionLabelComponents.Add(InfoTextComp);
		UE_LOG(LogTemp, Log, TEXT("      Added region info label to array"));
	}
}

void APresetRoomSpawner::ClearRoomGrid()
{
	UE_LOG(LogTemp, Warning, TEXT("PresetRoomSpawner::ClearRoomGrid - START"));
	
	// Clear region labels FIRST
	ClearRegionLabels();
	
	// Then call base implementation
	Super::ClearRoomGrid();
	
	UE_LOG(LogTemp, Warning, TEXT("PresetRoomSpawner::ClearRoomGrid - END"));
}

void APresetRoomSpawner::ClearRegionLabels()
{
	UE_LOG(LogTemp, Log, TEXT("Clearing %d region label components"), RegionLabelComponents. Num());
	
	for (UTextRenderComponent* TextComp : RegionLabelComponents)
	{
		if (IsValid(TextComp))
		{
			DestroyTextRenderComponent(TextComp); // Use base class function
		}
	}
	
	RegionLabelComponents.Empty();
}
#pragma endregion
