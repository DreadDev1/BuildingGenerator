// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WallData.generated.h"

struct FWallModule;

UCLASS()
class BUILDINGGENERATOR_API UWallData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// The collection of all available wall modules (Base/Middle/Top components)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Modules")
	TArray<FWallModule> AvailableWallModules;

	// The default static mesh to use for the floor in the room (e.g., a simple square tile)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Defaults")
	TSoftObjectPtr<UStaticMesh> DefaultCornerMesh; 
	
	// Per-corner position offsets (clockwise from bottom-left)	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Position Adjustments")
	FVector SouthWestCornerOffset = FVector::ZeroVector;  // Corner at (0, 0)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Position Adjustments")
	FVector NorthWestCornerOffset = FVector::ZeroVector;  // Corner at (0, GridSize.Y)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Position Adjustments")
	FVector NorthEastCornerOffset = FVector::ZeroVector;  // Corner at (GridSize.X, GridSize.Y)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Position Adjustments")
	FVector SouthEastCornerOffset = FVector::ZeroVector;  // Corner at (GridSize.X, 0)
	
	// Per-corner rotation adjustments
	// Allows independent rotation of each corner based on mesh orientation
	// Default rotations assume mesh faces inward (clockwise from SouthWest)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Rotation Adjustments")
	FRotator SouthWestCornerRotation = FRotator::ZeroRotator;  // 0° - Bottom-left
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Rotation Adjustments")
	FRotator SouthEastCornerRotation = FRotator(0.0f, 90.0f, 0.0f);  // 90° - Bottom-right
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Rotation Adjustments")
	FRotator NorthEastCornerRotation = FRotator(0.0f, 180.0f, 0.0f);  // 180° - Top-right
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corner Rotation Adjustments")
	FRotator NorthWestCornerRotation = FRotator(0.0f, 270.0f, 0.0f);  // 270° - Top-left
	
	// Wall position adjustments for fine-tuning alignment
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position Adjustments")
	float NorthWallOffsetX = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position Adjustments")
	float SouthWallOffsetX = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position Adjustments")
	float EastWallOffsetY = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position Adjustments")
	float WestWallOffsetY = 0.0f;
	
	// Default height for the wall geometry, based on the middle mesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Defaults")
	float WallHeight = 400.0f;
	

	// WALL COLUMNS / PILLARS	
	// Enable wall-mounted columns/pillars as decorative elements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations")
	bool bEnableWallColumns = false;
	
	// Static mesh for wall columns
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns"))
	TSoftObjectPtr<UStaticMesh> WallColumnMesh;
	
	// Place columns at door frames
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns"))
	bool bPlaceColumnsAtDoors = true;
	
	// Place columns at regular intervals along walls
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns"))
	bool bPlaceColumnsAtIntervals = true;
	
	// Distance between columns (in cm) - Default 800cm = 8 cells
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns && bPlaceColumnsAtIntervals", ClampMin = "100.0"))
	float ColumnSpacing = 800.0f;
	
	// Minimum distance between any two columns (prevents clustering)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns", ClampMin = "100.0"))
	float MinColumnDistance = 800.0f;
	
	// Position offset from wall surface (for fine-tuning)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns"))
	FVector ColumnPositionOffset = FVector::ZeroVector;
	
	// Rotation offset for columns (if needed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Decorations", meta = (EditCondition = "bEnableWallColumns"))
	FRotator ColumnRotationOffset = FRotator::ZeroRotator;
};
