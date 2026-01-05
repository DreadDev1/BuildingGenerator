// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "ChunkyRoomGenerator.generated.h"

// Forward declarations
enum class EWallEdge : uint8;

/* Stores a perimeter cell with information about which direction it needs walls */
USTRUCT()
struct FPerimeterCellInfo
{
	GENERATED_BODY()

	/** Grid coordinate of the perimeter cell */
	UPROPERTY()
	FIntPoint Cell = FIntPoint::ZeroValue;

	/** Which direction the wall should face (inward toward room) */
	UPROPERTY()
	EWallEdge FacingDirection = EWallEdge::North;

	/** Whether this cell is a corner (needs walls on multiple sides) */
	UPROPERTY()
	bool bIsCorner = false;

	FPerimeterCellInfo() {}
    
	FPerimeterCellInfo(FIntPoint InCell, EWallEdge InDirection, bool bInIsCorner = false)
		: Cell(InCell), FacingDirection(InDirection), bIsCorner(bInIsCorner)
	{}
};

/* Represents a continuous segment of walls facing the same direction */
USTRUCT()
struct FChunkyWallSegment
{
	GENERATED_BODY()

	/** Which direction walls in this segment face */
	UPROPERTY()
	EWallEdge Direction = EWallEdge::North;

	/** All cells that are part of this segment */
	UPROPERTY()
	TArray<FIntPoint> Cells;

	/** First cell in the segment */
	UPROPERTY()
	FIntPoint StartCell = FIntPoint::ZeroValue;

	/** Number of cells in this segment */
	UPROPERTY()
	int32 Length = 0;

	FChunkyWallSegment() {}
};

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
	
	/** Get perimeter cells with wall facing direction information */
	TArray<FPerimeterCellInfo> GetPerimeterCellsWithDirection() const;
	
	/** Determine which direction a wall should face for a perimeter cell */
	EWallEdge DetermineWallDirection(FIntPoint Cell) const;
	
	/** Check if a perimeter cell is a corner (needs walls on multiple sides) */
	bool IsCornerCell(FIntPoint Cell) const;
	
	/** Group perimeter cells into continuous wall segments */
	TArray<FChunkyWallSegment> GroupPerimeterIntoSegments(const TArray<FPerimeterCellInfo>& PerimeterCells) const;
	
	/** Place walls for a single segment */
	void PlaceWallSegment(const FChunkyWallSegment& Segment);
	
	/** Get directional offset for a wall edge */
	FIntPoint GetDirectionOffset(EWallEdge Direction) const;
#pragma endregion

	
};