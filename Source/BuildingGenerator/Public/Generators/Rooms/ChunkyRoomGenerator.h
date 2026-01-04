// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "ChunkyRoomGenerator.generated.h"

UCLASS()
class BUILDINGGENERATOR_API UChunkyRoomGenerator : public URoomGenerator
{
	GENERATED_BODY()

public:
	/** Override:  Create chunky grid pattern */
	virtual void CreateGrid() override;
	
#pragma region Room Generation Interface
	/** Generate floor meshes - uses base implementation (fills all ECT_FloorMesh cells) */
	virtual bool GenerateFloor() override;
	
	/** Generate walls along irregular perimeter */
	virtual bool GenerateWalls() override;
	
	/** Generate corners - may skip or use algorithmic detection */
	virtual bool GenerateCorners() override;
	
	/** Generate doorways - uses base implementation */
	virtual bool GenerateDoorways() override;
	
	/** Generate ceiling - uses base implementation (fills all ECT_FloorMesh cells) */
	virtual bool GenerateCeiling() override;
#pragma endregion

#pragma region ChunkyRoom generation parameters
	/** Minimum number of protrusions to add */
	UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "0", ClampMax = "20"))
	int32 MinProtrusions = 3;
	
	/** Maximum number of protrusions to add */
	UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "0", ClampMax = "20"))
	int32 MaxProtrusions = 8;
	
	/** Minimum size for protrusion width/height (cells) */
	UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "2", ClampMax = "10"))
	int32 MinProtrusionSize = 2;
	
	/** Maximum size for protrusion width/height (cells) */
	UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "2", ClampMax = "20"))
	int32 MaxProtrusionSize = 6;
	
	/** Percentage of grid used for base room (0.5 = 50%, 0.8 = 80%) */
	UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "0.3", ClampMax = "0.95"))
	float BaseRoomPercentage = 0.7f;
	
	/** Random seed for generation (-1 = random each time, 0+ = deterministic) */
	UPROPERTY(EditAnywhere, Category = "Chunky Generation")
	int32 RandomSeed = -1;
#pragma endregion
	
#pragma region Internal Helpers
	/** Random stream for deterministic generation */
	FRandomStream RandomStream;
	
	/** Base room bounds (calculated during CreateGrid) */
	FIntPoint BaseRoomStart;
	FIntPoint BaseRoomSize;
	
	/** Mark a rectangular area as floor cells */
	void MarkRectangle(int32 StartX, int32 StartY, int32 Width, int32 Height);
	
	/** Add a random protrusion extending from the base room */
	void AddRandomProtrusion();
	
	/** Check if a cell has a floor neighbor in a specific direction */
	bool HasFloorNeighbor(FIntPoint Cell, FIntPoint Direction) const;
	
	/** Get all perimeter cells (floor cells adjacent to empty cells) */
	TArray<FIntPoint> GetPerimeterCells() const;
#pragma endregion

	
};