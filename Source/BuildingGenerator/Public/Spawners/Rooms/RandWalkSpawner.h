// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RoomSpawner.h"
#include "RandWalkSpawner.generated.h"

/* Generates organic irregular room shapes using random walk algorithm */
UCLASS()
class BUILDINGGENERATOR_API ARandWalkSpawner : public ARoomSpawner
{
	GENERATED_BODY()

public:
	
	ARandWalkSpawner();
	
#pragma region Random Walk Configuration
	/** Target percentage of grid to fill (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Shape", 
		meta = (ClampMin = "0.2", ClampMax = "0.9", UIMin = "0.2", UIMax = "0.9"))
	float TargetFillRatio = 0.6f;

	/** Chance for walker to branch and create new walker (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Shape", 
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float BranchingChance = 0.3f;

	/** Chance for walker to change direction each step (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Shape", 
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DirectionChangeChance = 0.4f;

	/** Maximum number of active walkers at once */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Shape", 
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxActiveWalkers = 3;

	/** Number of smoothing iterations to apply (0 = no smoothing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Post-Processing", 
		meta = (ClampMin = "0", ClampMax = "10"))
	int32 SmoothingIterations = 2;

	/** Remove disconnected regions (keep only largest connected area) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Post-Processing")
	bool bRemoveDisconnectedRegions = true;

	/** Random seed for deterministic generation (-1 = random each time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Generation")
	int32 RandomSeed = -1;

	/** Use a random seed each time (ignores RandomSeed value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Generation")
	bool bUseRandomSeed = false;
#pragma endregion

#pragma region Debug Visualization
	/** Show the irregular room boundary in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Debug")
	bool bShowIrregularBoundary = true;

	/** Color for irregular boundary visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Debug")
	FColor IrregularBoundaryColor = FColor::Orange;

	/** Thickness of boundary lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Walk|Debug", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float BoundaryThickness = 3.0f;
#pragma endregion

protected:
	/** Override to create RandWalkRoomGenerator instead of base RoomGenerator */
	virtual bool EnsureGeneratorReady() override;

	/** Override to add irregular boundary visualization */
	virtual void UpdateVisualization() override;

private:
	/** Draw the irregular room boundary */
	void DrawIrregularBoundary();
};