// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "ChunkyRoomGenerator.generated.h"

/** Struct to represent a rectangular chunk */
USTRUCT()
struct FRoomChunk
{
	GENERATED_BODY()

	/** Top-left position of chunk in grid coordinates */
	UPROPERTY()
	FIntPoint Position;

	/** Size of chunk (Width x Height in cells) */
	UPROPERTY()
	FIntPoint Size;

	FRoomChunk() :  Position(FIntPoint::ZeroValue), Size(FIntPoint::ZeroValue) {}
	FRoomChunk(FIntPoint InPos, FIntPoint InSize) : Position(InPos), Size(InSize) {}

	/** Check if this chunk overlaps with another */
	bool Overlaps(const FRoomChunk& Other) const
	{
		return !(Position.X >= Other.Position.X + Other. Size.X ||
		         Position.X + Size.X <= Other.Position.X ||
		         Position.Y >= Other. Position.Y + Other.Size. Y ||
		         Position.Y + Size.Y <= Other.Position.Y);
	}

	/** Check if this chunk is adjacent to another (shares edge) */
	bool IsAdjacentTo(const FRoomChunk& Other) const
	{
		// Check if they share an edge (touching but not overlapping)
		bool SharesVerticalEdge = (Position.X + Size.X == Other.Position.X || Other.Position.X + Other.Size.X == Position.X) &&
		                          !(Position.Y >= Other.Position.Y + Other.Size.Y || Position.Y + Size.Y <= Other.Position.Y);
		
		bool SharesHorizontalEdge = (Position.Y + Size.Y == Other.Position.Y || Other.Position.Y + Other.Size.Y == Position.Y) &&
		                            !(Position.X >= Other. Position.X + Other.Size. X || Position.X + Size.X <= Other.Position.X);

		return SharesVerticalEdge || SharesHorizontalEdge;
	}
};

/**
 * UChunkyRoomGenerator - Generates irregular room shapes by combining rectangular chunks
 * Creates rooms with straight edges and 2-cell or 4-cell wall segments
 */
UCLASS()
class BUILDINGGENERATOR_API UChunkyRoomGenerator : public URoomGenerator
{
	GENERATED_BODY()

public:
	/** Initialize with chunky generation parameters */
	void SetChunkyParams(int32 InMinChunks, int32 InMaxChunks, float InChunk2x2Chance, 
	                     float InChunk4x4Chance, float InChunkRectChance, int32 InSeed);

	/** Override:  Create chunky grid pattern */
	virtual void CreateGrid() override;

private:
	// Chunky generation parameters
	int32 MinChunks;           // Minimum number of chunks to place
	int32 MaxChunks;           // Maximum number of chunks to place
	float Chunk2x2Chance;      // Probability of 2x2 chunk
	float Chunk4x4Chance;      // Probability of 4x4 chunk
	float ChunkRectChance;     // Probability of rectangular chunk (2x4, 4x2)
	int32 RandomSeed;          // Seed for deterministic generation

	// Random stream for deterministic generation
	FRandomStream RandomStream;

	// Placed chunks
	TArray<FRoomChunk> PlacedChunks;

	/** Execute the chunky generation algorithm */
	void ExecuteChunkyGeneration();

	/** Get random chunk size based on probabilities */
	FIntPoint GetRandomChunkSize();

	/** Try to place a chunk adjacent to existing chunks */
	bool TryPlaceAdjacentChunk(const FRoomChunk& NewChunk);

	/** Check if chunk placement is valid (within bounds, no overlap) */
	bool IsValidChunkPlacement(const FRoomChunk& Chunk) const;

	/** Mark all cells within a chunk as floor */
	void MarkChunkCells(const FRoomChunk& Chunk);

	/** Get potential positions adjacent to placed chunks */
	TArray<FIntPoint> GetAdjacentPositions() const;

	/** Validate minimum room size */
	bool ValidateMinimumSize();
};