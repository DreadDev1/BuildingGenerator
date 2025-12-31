// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Generators/Room/RoomGenerator.h"
#include "RandWalkGenerator.generated.h"

/** Struct to track active walker state */
USTRUCT()
struct FWalkerState
{
	GENERATED_BODY()

	/** Current position of walker */
	UPROPERTY()
	FIntPoint Position;

	/** Current direction walker is facing (0=North, 1=East, 2=South, 3=West) */
	UPROPERTY()
	int32 Direction;

	/** Number of steps this walker has taken */
	UPROPERTY()
	int32 StepsTaken;

	FWalkerState() : Position(FIntPoint::ZeroValue), Direction(0), StepsTaken(0) {}
	FWalkerState(FIntPoint InPos, int32 InDir) : Position(InPos), Direction(InDir), StepsTaken(0) {}
};

/**
 * URandWalkRoomGenerator - Generates irregular organic room shapes using random walk algorithm
 * Overrides CreateGrid() to create non-rectangular layouts while maintaining compatibility
 * with all existing room generation systems (walls, doors, ceiling, etc.)
 */
UCLASS()
class BUILDINGGENERATOR_API URandWalkGenerator : public URoomGenerator
{
	GENERATED_BODY()

public:
	/** Initialize with random walk parameters */
	void SetRandomWalkParams(float InFillRatio, float InBranchChance, float InDirChangeChance, 
		int32 InMaxWalkers, int32 InSmoothingPasses, bool bInRemoveIslands, int32 InSeed);

#pragma region Floor Generation Override
	/** Override:  Only generate floor in cells marked by random walk */
	virtual bool GenerateFloor() override;
	/** Override:  Create irregular grid pattern using random walk */
	virtual void CreateGrid() override;
#pragma endregion
	


#pragma region Random Walk Algorithm
	/** Execute the random walk algorithm to fill grid */
	void ExecuteRandomWalk();

	/** Process a single walker step */
	bool ProcessWalkerStep(FWalkerState& Walker, TArray<FWalkerState>& ActiveWalkers);

	/** Try to move walker in a direction */
	bool TryMoveWalker(FWalkerState& Walker, int32 Direction);

	/** Get valid directions walker can move from current position */
	TArray<int32> GetValidDirections(FIntPoint Position) const;

	/** Create a new walker (branching) */
	void CreateBranch(FIntPoint Position, TArray<FWalkerState>& ActiveWalkers);
#pragma endregion

#pragma region Post-Processing
	/** Smooth jagged edges using cellular automata rules */
	void SmoothGridEdges(int32 Iterations);

	/** Remove disconnected regions (keep largest connected component) */
	void RemoveDisconnectedRegions();

	/** Restore irregular boundary after floor generation */
	void RestoreIrregularBoundary(const TArray<EGridCellType>& OriginalGridState);
	
	/** Validate minimum room size requirements */
	bool ValidateMinimumSize();
#pragma endregion

#pragma region Helper Functions
	/** Count occupied neighbors of a cell (for smoothing) */
	int32 CountOccupiedNeighbors(FIntPoint Cell, int32 Radius = 1) const;

	/** Flood fill to find connected region */
	void FloodFill(FIntPoint StartCell, TSet<FIntPoint>& OutRegion) const;

	/** Get all occupied cells in grid */
	TArray<FIntPoint> GetOccupiedCells() const;

	/** Get direction vectors (North, East, South, West) */
	static FIntPoint GetDirectionVector(int32 Direction);

	/** Get opposite direction */
	static int32 GetOppositeDirection(int32 Direction);
#pragma endregion

private:
	// Random walk parameters
	float TargetFillRatio;
	float BranchingChance;
	float DirectionChangeChance;
	int32 MaxActiveWalkers;
	int32 SmoothingPasses;
	bool bRemoveIslands;
	int32 RandomSeed;

	// Random stream for deterministic generation
	FRandomStream RandomStream;

	// Direction vectors (North, East, South, West)
	static const TArray<FIntPoint> DirectionVectors;
};