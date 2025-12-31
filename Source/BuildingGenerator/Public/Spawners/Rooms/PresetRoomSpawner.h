// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RoomSpawner.h"
#include "PresetRoomSpawner.generated.h"

UCLASS()
class BUILDINGGENERATOR_API APresetRoomSpawner : public ARoomSpawner
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APresetRoomSpawner();
	
#pragma region Debug Visualization
	/** Show region boundaries in the editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Room|Debug")
	bool bShowRegionBoundaries = true;

	/** Show region names as text labels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Room|Debug")
	bool bShowRegionNames = true;

	/** Height offset for region boundary visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Room|Debug", meta=(ClampMin="0", ClampMax="500"))
	float RegionBoundaryHeight = 10.0f;

	/** Thickness of region boundary lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Room|Debug", meta=(ClampMin="1", ClampMax="20"))
	float RegionBoundaryThickness = 3.0f;
	
	/** Text scale for region names */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Room|Debug", meta=(ClampMin="1", ClampMax="10"))
	float RegionNameTextScale = 5.0f;

	/** Text scale for region info (size/priority) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Room|Debug", meta=(ClampMin="1", ClampMax="10"))
	float RegionInfoTextScale = 5.0f;
#pragma endregion
	
#pragma region Room Generation Properties
	/** Ensure generator is ready - Creates PresetRoomGenerator instead of base
	 * Overrides ARoomSpawner::EnsureGeneratorReady() */
	virtual bool EnsureGeneratorReady() override;

	/** Update visualization - Adds region boundary drawing
	 * Overrides ARoomSpawner::UpdateVisualization() */
	virtual void UpdateVisualization() override;
#pragma endregion
	
#pragma region Debug Drawing Functions
	/** Draw region boundaries for debugging */
	void DrawRegionBoundaries();

	/** Draw a single region boundary */
	void DrawRegionBoundary(const struct FPresetRegion& Region);

	/** Draw region name label */
	void DrawRegionLabel(const struct FPresetRegion& Region);
	
	virtual void ClearRoomGrid() override;
	void ClearRegionLabels();
	
	
	UPROPERTY()
	TArray<UTextRenderComponent*> RegionLabelComponents;
#pragma endregion
};
