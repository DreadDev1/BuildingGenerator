// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "ChunkyRoomGenerator.generated.h"

// Forward declarations
enum class EWallEdge : uint8;

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
#pragma region Internal Helper variables
	/** Random stream for deterministic generation */
	FRandomStream RandomStream;
	
	/** Base room bounds (calculated during CreateGrid) */
	FIntPoint BaseRoomStart;
	FIntPoint BaseRoomSize;
	
	/** Corner cells marked during GenerateCorners (walls will skip these) */
	TSet<FIntPoint> CornerCells;
#pragma endregion
	/** Mark a rectangular area as floor cells */
	void MarkRectangle(int32 StartX, int32 StartY, int32 Width, int32 Height);
	
	/** Add a random protrusion extending from the base room */
	void AddRandomProtrusion();
	
	/** Check if a cell has a floor neighbor in a specific direction */
	bool HasFloorNeighbor(FIntPoint Cell, FIntPoint Direction) const;
	
	/** Get all perimeter cells (floor cells adjacent to empty cells) */
	TArray<FIntPoint> GetPerimeterCells() const;
#pragma endregion

#pragma region Wall Generation Helper functions
	/** Get directional offset for a wall edge */
	FIntPoint GetDirectionOffset(EWallEdge Direction) const;
	
	/** Get perimeter cells for a specific edge direction (replaces GetEdgeCellIndices for chunky rooms) */
	TArray<FIntPoint> GetPerimeterCellsForEdge(EWallEdge Edge) const;
	
	/** Fill wall edge using base class greedy logic adapted for chunky perimeter */
	void FillChunkyWallEdge(EWallEdge Edge);
	
	/** Calculate wall position for a segment starting at a specific cell */
	FVector CalculateWallPositionForSegment(EWallEdge Direction, FIntPoint StartCell, int32 ModuleFootprint,
	float NorthOffset, float SouthOffset, float EastOffset, float WestOffset) const;
#pragma endregion
	
#pragma region Corner Generation Helper functions
	/** Check if a floor cell is a corner (has void on 2+ adjacent sides) */
	bool IsCornerCell(FIntPoint Cell, TArray<EWallEdge>& OutAdjacentVoidEdges) const;
	
	/** Determine corner position (NE/NW/SE/SW) from adjacent void edges */
	ECornerPosition GetCornerPositionFromEdges(const TArray<EWallEdge>& AdjacentEdges) const;

	/** Get position offset for a specific corner type from WallData */
	FVector GetCornerPositionOffset(ECornerPosition  CornerType) const;
	
	/** Check if this cell is marked as a corner (for wall generation to skip) */
	bool IsCellMarkedAsCorner(FIntPoint Cell) const;
#pragma endregion
};