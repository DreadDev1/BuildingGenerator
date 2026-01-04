// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Grid/GridData.h"
#include "Data/Room/FloorData.h"
#include "Data/Room/WallData.h"
#include "Data/Room/DoorData.h"
#include "Data/Room/CeilingData.h"
#include "Data/Room/RoomData.h"
#include "RoomGenerator.generated.h"



struct FPlacedMeshInfo;
struct FGeneratorWallSegment;
struct FPlacedCeilingInfo;

/* RoomGenerator - Pure logic class for room generation Handles grid creation, mesh placement algorithms, and room data processing */
UCLASS(Abstract)
class BUILDINGGENERATOR_API URoomGenerator : public UObject
{
	GENERATED_BODY()

public:
	// Initialization flag
	bool bIsInitialized;

protected:
	/** Target cell type for floor placement (ECT_Empty for uniform, ECT_Custom for chunky/shaped) */
	EGridCellType FloorTargetCellType = EGridCellType::ECT_Empty;
	
public:
#pragma region Initialization
	/* Initialize the room generator with room data */
	bool Initialize(URoomData* InRoomData, FIntPoint InGridSize);
	UFUNCTION(BlueprintPure, Category = "Room Generator")
	bool IsInitialized() const { return bIsInitialized; }
#pragma endregion
	
#pragma region public Internal Floor Generation Functions
	/* Select a mesh from pool using weighted random selection */
	FMeshPlacementInfo SelectWeightedMesh(const TArray<FMeshPlacementInfo>& Pool);
	
	/* Calculate footprint size in cells from mesh bounds */
	FIntPoint CalculateFootprint(const FMeshPlacementInfo& MeshInfo) const;
	
	/* Try to place a mesh at specified location */
	bool TryPlaceMesh(FIntPoint StartCoord, FIntPoint Size, const FMeshPlacementInfo& MeshInfo, int32 Rotation = 0);
#pragma endregion
	
#pragma region Room Grid Management
	
	// Grid dimensions in cells
	FIntPoint GridSize;
	
	// Grid state array (row-major order: Index = Y * GridSize.X + X)
	UPROPERTY()
	TArray<EGridCellType> GridState;
	
	UFUNCTION(BlueprintCallable, Category = "Room Generator")
	virtual void CreateGrid() PURE_VIRTUAL(URoomGenerator::CreateGrid, );
	UFUNCTION(BlueprintCallable, Category = "Room Generator")
	void ClearGrid();
	UFUNCTION(BlueprintCallable, Category = "Room Generator")
	void ResetGridCellStates();
	const TArray<EGridCellType>& GetGridState() const { return GridState; }
	FIntPoint GetGridSize() const { return GridSize; }
	float GetCellSize() const { return CellSize; }
	EGridCellType GetCellState(FIntPoint GridCoord) const;
	bool SetCellState(FIntPoint GridCoord, EGridCellType NewState);
	bool IsValidGridCoordinate(FIntPoint GridCoord) const;
	bool IsAreaAvailable(FIntPoint StartCoord, FIntPoint Size) const;

	/** Mark a rectangular area as occupied 
	 * @param StartCoord - Top-left corner of area @param Size - Size in cells (X, Y) @param CellType */
	bool MarkArea(FIntPoint StartCoord, FIntPoint Size, EGridCellType CellType);

	/** Clear a rectangular area (set to Empty) * @param StartCoord - Top-left corner of area @param Size - Size of area in cells (X, Y) */
	bool ClearArea(FIntPoint StartCoord, FIntPoint Size);
#pragma endregion

#pragma region Floor Generation

	UPROPERTY(EditAnywhere)
	UFloorData* FloorData;
	
	/* Generate floor meshes using sequential weighted fill algorithm */
	virtual bool GenerateFloor() PURE_VIRTUAL(URoomGenerator::GenerateFloor, return false;);

	/* Get list of placed floor meshes */
	const TArray<FPlacedMeshInfo>& GetPlacedFloorMeshes() const { return PlacedFloorMeshes; }

	/* Clear all placed floor meshes */
	void ClearPlacedFloorMeshes();

	/* Get floor generation statistics */
	void GetFloorStatistics(int32& OutLargeTiles, int32& OutMediumTiles, int32& OutSmallTiles, int32& OutFillerTiles) const;

	/* Execute forced placements from RoomData
	 * Places designer-specified meshes at exact coordinates before random fill */
	int32 ExecuteForcedPlacements();

	/* Fill remaining empty cells with meshes from the pool */
	int32 FillRemainingGaps(const TArray<FMeshPlacementInfo>& TilePool, int32& OutLargeTiles,
	int32& OutMediumTiles, int32& OutSmallTiles, int32& OutFillerTiles); 
	
	/**
	 * Expand forced empty regions into individual cell list
	 * Combines rectangular regions and individual cells into unified list */
	TArray<FIntPoint> ExpandForcedEmptyRegions() const;

	/* Mark forced empty cells as reserved (blocked from placement) */
	void MarkForcedEmptyCells(const TArray<FIntPoint>& EmptyCells);
#pragma endregion

#pragma region Wall Generation
	
	UPROPERTY(EditAnywhere)
	UWallData* WallData;
	
	/* Generate walls for all four edges Uses greedy bin packing (largest modules first) */
	virtual bool GenerateWalls() PURE_VIRTUAL(URoomGenerator::GenerateWalls, return false;);

	/* Get list of placed walls */
	const TArray<FPlacedWallInfo>& GetPlacedWalls() const { return PlacedWallMeshes; }

	int32 ExecuteForcedWallPlacements();

	bool IsCellRangeOccupied(EWallEdge Edge, int32 StartCell, int32 Length) const;
	
	/* Clear all placed walls */
	void ClearPlacedWalls();

	/* Called after base walls are placed */
	void SpawnMiddleWallLayers();

	/* Called after middle walls are placed */
	void SpawnTopWallLayer();

#pragma endregion
	
#pragma region Corner Generation

	/* Generate corner pieces for all 4 corners */
	virtual bool GenerateCorners() PURE_VIRTUAL(URoomGenerator::GenerateCorners, return false;);


	/* Get list of placed corners */
	const TArray<FPlacedCornerInfo>& GetPlacedCorners() const { return PlacedCornerMeshes; }

	/* Clear all placed corners */
	void ClearPlacedCorners();

#pragma endregion
	
#pragma region Doorway Generation

	UPROPERTY(EditAnywhere)
	UDoorData* DoorData;
	
	/* Generate doorways (manual + automatic standard doorway) */
	virtual bool GenerateDoorways() PURE_VIRTUAL(URoomGenerator::GenerateDoorways, return false;);

	/* Mark doorway cells as occupied (called before wall generation) */
	void MarkDoorwayCells();

	/* Check if a cell is part of any doorway */
	bool IsCellPartOfDoorway(FIntPoint Cell) const;

	/* Get list of placed doorways */
	const TArray<FPlacedDoorwayInfo>& GetPlacedDoorways() const { return PlacedDoorwayMeshes; }

	/* Clear all placed doorways */
	void ClearPlacedDoorways();

#pragma endregion
	
#pragma region Ceiling Generation
	
	UPROPERTY(EditAnywhere)
	UCeilingData* CeilingData;
	
	/* Generate ceiling tile layout */
	UFUNCTION(BlueprintCallable, Category = "Room Generation")
	virtual bool GenerateCeiling() PURE_VIRTUAL(URoomGenerator::GenerateCeiling, return false;);
	
	/* Get placed ceiling tiles (for spawner) */
	UFUNCTION(BlueprintPure, Category = "Room Generation")
	const TArray<FPlacedCeilingInfo>& GetPlacedCeilingTiles() const { return PlacedCeilingTiles; }

	/* Clear ceiling data */
	void ClearPlacedCeiling() { PlacedCeilingTiles.Empty(); }
#pragma endregion
	
#pragma region Coordinate Conversion
	/* Convert grid coordinates to local position (center of cell) */
	FVector GridToLocalPosition(FIntPoint GridCoord) const;

	/* Convert local position to grid coordinates */
	FIntPoint LocalToGridPosition(FVector LocalPos) const;

	/* Get rotated footprint based on rotation angle */
	static FIntPoint GetRotatedFootprint(FIntPoint OriginalFootprint, int32 Rotation);
#pragma endregion

#pragma region Room Statistics

	/* Get count of cells by type */
	int32 GetCellCountByType(EGridCellType CellType) const;

	/* Get percentage of grid occupied */
	float GetOccupancyPercentage() const;

	/* Get total cell count */
	int32 GetTotalCellCount() const { return GridSize.X * GridSize.Y; }
#pragma endregion

#pragma region Internal Data

	// Reference to room configuration data
	UPROPERTY()
	URoomData* RoomData;
	
	// Cell size in cm (from CELL_SIZE constant)
	float CellSize;
	
	// Placed floor meshes
	UPROPERTY()
	TArray<FPlacedMeshInfo> PlacedFloorMeshes;

	// Placed walls
	UPROPERTY()
	TArray<FPlacedWallInfo> PlacedWallMeshes;
	// Placed corners
	UPROPERTY()
	TArray<FPlacedCornerInfo> PlacedCornerMeshes;

	// Tracked base wall segments for Middle/Top spawning
	UPROPERTY()
	TArray<FGeneratorWallSegment> PlacedBaseWallSegments;
	
	// Placed doorways
	UPROPERTY()
	TArray<FPlacedDoorwayInfo> PlacedDoorwayMeshes;
	
	/* Placed ceiling tiles (output of GenerateCeiling) */
	UPROPERTY()
	TArray<FPlacedCeilingInfo> PlacedCeilingTiles;
	
	// Statistics tracking
	int32 LargeTilesPlaced;
	int32 MediumTilesPlaced;
	int32 SmallTilesPlaced;
	int32 FillerTilesPlaced;

	// Cached doorway layouts (persistent until ClearRoomGrid)
	UPROPERTY()
	TArray<FDoorwayLayoutInfo> CachedDoorwayLayouts;

	// Helper to calculate transforms from layout
	FPlacedDoorwayInfo CalculateDoorwayTransforms(const FDoorwayLayoutInfo& Layout);
#pragma endregion

#pragma region private Internal Floor Generation Functions
	/* Fill grid with tiles of specific size */
	void FillWithTileSize(const TArray<FMeshPlacementInfo>& TilePool, FIntPoint TargetSize, 
	int32& OutLargeTiles, int32& OutMediumTiles, int32& OutSmallTiles, int32& OutFillerTiles);
#pragma endregion

#pragma region Internal Ceiling Generation Functions
	// Ceiling generation helpers
	void FillCeilingWithTileSize(const TArray<FMeshPlacementInfo>& TilePool, TArray<bool>& CeilingOccupied, 
	FIntPoint TargetSize, const FRotator& CeilingRotation, float CeilingHeight, int32& OutTilesPlaced);

	int32 FillRemainingCeilingGaps(const TArray<FMeshPlacementInfo>& TilePool, TArray<bool>& CeilingOccupied, const FRotator& CeilingRotation,
	float CeilingHeight, int32& OutLargeTiles, int32& OutMediumTiles, int32& OutSmallTiles, int32& OutFillerTiles);

	int32 ExecuteForcedCeilingPlacements(TArray<bool>& CeilingOccupied);
#pragma endregion
	
#pragma region Internal Helpers
	/* Convert 2D grid coordinate to 1D array index */
	int32 GridCoordToIndex(FIntPoint GridCoord) const;

	/* Convert 1D array index to 2D grid coordinate */
	FIntPoint IndexToGridCoord(int32 Index) const;

	/* Fill one edge with wall modules using greedy bin packing */
	void FillWallEdge(EWallEdge Edge);
#pragma endregion
	
};