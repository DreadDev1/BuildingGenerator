// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Generation/RoomGenerationTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Grid/GridData.h"
#include "RoomGenerationHelpers.generated.h"

UCLASS()
class BUILDINGGENERATOR_API URoomGenerationHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
#pragma region Grid & Cell Operations 
	/** Get cell indices for a specific wall edge
	 * @param Edge - Which edge to get cells for @param GridSize - Size of the grid  @return Array of cell indices along that edge */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Grid")
	static TArray<FIntPoint> GetEdgeCellIndices(EWallEdge Edge, FIntPoint GridSize);

	/** Check if a coordinate is within grid bounds
	* @param Coord - Coordinate to check @param GridSize - Size of the grid @return True if coordinate is valid */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Grid")
	static bool IsValidGridCoordinate(FIntPoint Coord, FIntPoint GridSize);

	/** Convert 1D index to 2D grid coordinate 
	 * @param Index - 1D array index @param GridWidth - Width of the grid (X dimension) @return 2D grid coordinate */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Grid")
	static FIntPoint IndexToCoordinate(int32 Index, int32 GridWidth);

	/** Convert 2D grid coordinate to 1D index
	* @param Coord - 2D grid coordinate @param GridWidth - Width of the grid (X dimension) @return 1D array index */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Grid")
	static int32 CoordinateToIndex(FIntPoint Coord, int32 GridWidth);
#pragma endregion
	 
#pragma region Grid Placement Utilities
	/** Check if an area is available for placement
	* @param GridState - Current grid state array @param GridSize - Size of the grid @param StartCoord - Top-left corner to check
	* @param Size - Size of area in cells @param RequiredType - Cell type required (default: Empty) @return True if entire area is available */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Grid")
	static bool IsAreaAvailable(const TArray<EGridCellType>& GridState,	FIntPoint GridSize, FIntPoint StartCoord,
	FIntPoint Size,	EGridCellType RequiredType = EGridCellType::ECT_Empty);

	/** Mark cells in an area as occupied
	* @param GridState - Grid state array to modify @param GridSize - Size of the grid @param StartCoord - Top-left corner
	* @param Size - Size of area in cells @param CellType - Type to mark cells as */
	UFUNCTION(BlueprintCallable, Category = "Dungeon Generation|Grid")
	static void MarkCellsOccupied(TArray<EGridCellType>& GridState, FIntPoint GridSize, FIntPoint StartCoord,
	FIntPoint Size, EGridCellType CellType = EGridCellType::ECT_FloorMesh);

	/** Try to place a mesh in the grid (check availability + mark cells)
	* @param GridState - Grid state array to modify @param GridSize - Size of the grid @param StartCoord - Top-left corner for placement
	* @param Size - Size of mesh footprint in cells @param PlacementType - Cell type to mark as (default: FloorMesh)
	* @return True if placement succeeded, false if area unavailable */
	UFUNCTION(BlueprintCallable, Category = "Dungeon Generation|Grid")
	static bool TryPlaceMeshInGrid(TArray<EGridCellType>& GridState, FIntPoint GridSize, FIntPoint StartCoord,
	FIntPoint Size,	EGridCellType TargetCellType, EGridCellType PlacementType = EGridCellType::ECT_FloorMesh);
#pragma endregion

#pragma region Rotation & Footprint Operations
	/** Calculate rotated footprint for a mesh
	* @param OriginalFootprint - footprint dimensions@param RotationDegrees -  (0, 90, 180, 270)* @return Rotated footprint */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Rotation")
	static FIntPoint GetRotatedFootprint(FIntPoint OriginalFootprint, int32 RotationDegrees);

	/** Check if rotation swaps X and Y dimensions
	* @param RotationDegrees - Rotation in degrees @return True if 90° or 270° rotation */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Rotation")
	static bool DoesRotationSwapDimensions(int32 RotationDegrees);
#pragma endregion
	 
#pragma region Wall Edge Operations
	/**Get rotation for walls on a specific edge (all face inward)
	* @param Edge - Wall edge @return Rotation for walls on that edge */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Walls")
	static FRotator GetWallRotationForEdge(EWallEdge Edge);

	/** Calculate world position for a wall segment 
	* @param Edge - Wall edge @param StartCell - Starting cell index
	* @param SpanLength - Number of cells wall spans @param GridSize - Size of the grid @param CellSize - each cell in world units
	* @param NorthOffset - North wall offset (X-axis) @param SouthOffset - South wall offset (X-axis)
	* @param EastOffset - East wall offset (Y-axis)  @param WestOffset - West wall offset (Y-axis) @return World position for the wall */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Walls")
	static FVector CalculateWallPosition( EWallEdge Edge, int32 StartCell, int32 SpanLength, FIntPoint GridSize, float CellSize,
	float NorthOffset, float SouthOffset, float EastOffset, float WestOffset);

	/* Calculate doorway center position (for frame/actor placement) */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Doorways")
	static FVector CalculateDoorwayPosition(EWallEdge Edge, int32 StartCell, 
	int32 WidthInCells, FIntPoint GridSize, float CellSize);
#pragma endregion
	
#pragma region Mesh Operations
	/** Load and validate a static mesh with error logging
	* @param MeshAsset - Soft pointer to mesh @param ContextName - Name for logging context
	* @param bLogWarning - Log if mesh fails to load @return Loaded mesh or nullptr if failed */
	static UStaticMesh* LoadAndValidateMesh(
	const TSoftObjectPtr<UStaticMesh>& MeshAsset, const FString& ContextName, bool bLogWarning = true);
	
	/** Calculate world transform for a mesh at grid position
	* @param GridPosition - Grid coordinate @param MeshSize - Size of mesh @param CellSize - Size of cell in world units
	* @param Rotation - Rotation (0, 90, 180, 270) @param ZOffset - Vertical offset (default 0) @return World transform for the mesh */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Transform")
	static FTransform CalculateMeshTransform(FIntPoint GridPosition, FIntPoint MeshSize, float CellSize,
	int32 Rotation = 0,	float ZOffset = 0.0f);
#pragma endregion
	 
#pragma region Transform Operations
	 /** Get socket transform from a static mesh */
	UFUNCTION(BlueprintPure, Category = "Dungeon Generation|Mesh")
	static bool GetMeshSocketTransform(UStaticMesh* Mesh, FName SocketName, FVector& OutLocation, FRotator& OutRotation);

	/** Get socket transform with fallback */
	static bool GetMeshSocketTransformWithFallback(UStaticMesh* Mesh, FName SocketName, FVector& OutLocation, 
	FRotator& OutRotation, FVector FallbackLocation = FVector::ZeroVector, FRotator FallbackRotation = FRotator::ZeroRotator);

	/* Calculate world transform for a mesh stacked on a socket */
	static FTransform CalculateSocketWorldTransform(UStaticMesh* Mesh, FName SocketName, const FTransform& ParentTransform,
	FVector FallbackOffset = FVector::ZeroVector);
#pragma endregion
	 
#pragma region Weighted Selection
	/* Select random item from array using weighted selection */
	template<typename T>
	static const T* SelectWeightedRandom(const TArray<T>& Items, TFunction<float(const T&)> GetWeightFunc);

	/* Select random wall module using weighted selection */
	static const FWallModule* SelectWeightedWallModule(const TArray<FWallModule>& Modules);

	/* Select random mesh placement info using weighted selection */
	static const FMeshPlacementInfo* SelectWeightedMeshPlacement(const TArray<FMeshPlacementInfo>& MeshPool);
#pragma endregion
};

// TEMPLATE IMPLEMENTATIONS (Must be in header)
template<typename T>
const T* URoomGenerationHelpers::SelectWeightedRandom(const TArray<T>& Items, TFunction<float(const T&)> GetWeightFunc)
{
	if (Items.Num() == 0) return nullptr;

	// Calculate total weight
	float TotalWeight = 0.0f;
	for (const T& Item :  Items) { TotalWeight += GetWeightFunc(Item); }

	// If all weights are zero, select uniformly
	if (TotalWeight <= 0.0f)
	{
		int32 RandomIndex = FMath::RandRange(0, Items.Num() - 1);
		return &Items[RandomIndex];
	}

	// Weighted random selection
	float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
	float CurrentWeight = 0.0f;

	for (const T& Item :  Items)
	{
		CurrentWeight += GetWeightFunc(Item);
		if (RandomValue <= CurrentWeight) { return &Item; }
	}

	// Fallback (should never reach here)
	return &Items. Last();
};
