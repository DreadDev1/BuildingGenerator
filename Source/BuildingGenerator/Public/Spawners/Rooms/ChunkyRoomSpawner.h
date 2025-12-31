// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spawners/Rooms/RoomSpawner.h"
#include "ChunkyRoomSpawner.generated.h"

class UChunkyRoomGenerator;

/**
 * AChunkyRoomSpawner - Spawner for chunky room generation
 * Creates rooms by combining rectangular chunks in irregular patterns
 */
UCLASS()
class BUILDINGGENERATOR_API AChunkyRoomSpawner : public ARoomSpawner
{
	GENERATED_BODY()

public:
	AChunkyRoomSpawner();

#pragma region Chunky Generation Configuration
	/** Minimum number of chunks to place */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Generation", 
		meta = (ClampMin = "1", ClampMax = "20", UIMin = "1", UIMax = "20"))
	int32 MinChunks = 3;

	/** Maximum number of chunks to place */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Generation", 
		meta = (ClampMin = "1", ClampMax = "50", UIMin = "1", UIMax = "50"))
	int32 MaxChunks = 8;

	/** Probability of placing 2x2 chunks (200x200cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Chunk Types", 
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Chunk2x2Chance = 0.4f;

	/** Probability of placing 4x4 chunks (400x400cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Chunk Types", 
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Chunk4x4Chance = 0.3f;

	/** Probability of placing rectangular chunks (2x4 or 4x2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Chunk Types", 
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ChunkRectChance = 0.3f;

	/** Random seed for deterministic generation (-1 = random each time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Generation")
	int32 RandomSeed = -1;

	/** Use a random seed each time (ignores RandomSeed value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Generation")
	bool bUseRandomSeed = false;
#pragma endregion

#pragma region Debug Visualization
	/** Show chunk boundaries in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Debug")
	bool bShowChunkBoundaries = true;

	/** Color for chunk boundary visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Debug")
	FColor ChunkBoundaryColor = FColor::Cyan;

	/** Thickness of chunk boundary lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunky Room|Debug", 
		meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float ChunkBoundaryThickness = 2.0f;
#pragma endregion

protected:
	/** Override to create ChunkyRoomGenerator instead of base RoomGenerator */
	virtual bool EnsureGeneratorReady() override;

	/** Override to add chunk boundary visualization */
	virtual void UpdateVisualization() override;

private:
	UChunkyRoomGenerator* ChunkyGen;
	
	/** Draw chunk boundaries for debugging */
	void DrawChunkBoundaries();
};