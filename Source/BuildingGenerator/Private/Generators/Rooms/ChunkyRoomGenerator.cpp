// Fill out your copyright notice in the Description page of Project Settings.

#include "Generators/Rooms/ChunkyRoomGenerator.h"
#include "Utilities/Generation/RoomGenerationHelpers.h"

void UChunkyRoomGenerator::SetChunkyParams(int32 InMinChunks, int32 InMaxChunks, 
    float InChunk2x2Chance, float InChunk4x4Chance, float InChunkRectChance, int32 InSeed)
{
	MinChunks = FMath::Max(1, InMinChunks);
	MaxChunks = FMath::Max(MinChunks, InMaxChunks);
	Chunk2x2Chance = FMath:: Clamp(InChunk2x2Chance, 0.0f, 1.0f);
	Chunk4x4Chance = FMath::Clamp(InChunk4x4Chance, 0.0f, 1.0f);
	ChunkRectChance = FMath::Clamp(InChunkRectChance, 0.0f, 1.0f);
	RandomSeed = InSeed;

	// Normalize probabilities
	float Total = Chunk2x2Chance + Chunk4x4Chance + ChunkRectChance;
	if (Total > 0.0f)
	{
		Chunk2x2Chance /= Total;
		Chunk4x4Chance /= Total;
		ChunkRectChance /= Total;
	}
	else
	{
		// Default to equal distribution
		Chunk2x2Chance = 0.33f;
		Chunk4x4Chance = 0.33f;
		ChunkRectChance = 0.34f;
	}

	// Initialize random stream
	RandomStream.Initialize(RandomSeed);

	UE_LOG(LogTemp, Log, TEXT("ChunkyRoomGenerator:  Parameters set - 2x2: %.2f, 4x4: %.2f, Rect: %.2f, Chunks: %d-%d, Seed: %d"),
		Chunk2x2Chance, Chunk4x4Chance, ChunkRectChance, MinChunks, MaxChunks, RandomSeed);
}

void UChunkyRoomGenerator::CreateGrid()
{
	if (! bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("ChunkyRoomGenerator:: CreateGrid - Generator not initialized! "));
		return;
	}

	// Initialize grid with ALL cells as BOUNDARY (non-fillable)
	int32 TotalCells = GridSize.X * GridSize.Y;
	GridState.Empty(TotalCells);
	GridState.AddUninitialized(TotalCells);
	
	// Mark all cells as boundary (non-fillable) by default
	for (int32 i = 0; i < TotalCells; ++i)
	{
		GridState[i] = EGridCellType:: ECT_WallMesh;  // ← Non-chunk cells marked as boundary
	}
	
	UE_LOG(LogTemp, Log, TEXT("ChunkyRoomGenerator::CreateGrid - Starting chunky generation (Grid:  %dx%d, Total cells: %d)"),
			GridSize.X, GridSize.Y, TotalCells);
	
	// Execute chunky generation algorithm
	ExecuteChunkyGeneration();
	
	// Validate result
	if (! ValidateMinimumSize())
	{
		UE_LOG(LogTemp, Warning, TEXT("ChunkyRoomGenerator::CreateGrid - Generated room is too small!"));
	}

	// ✅ NEW: Log detailed statistics
	int32 FillableCells = GetCellCountByType(EGridCellType:: ECT_Empty);
	int32 BoundaryCells = GetCellCountByType(EGridCellType::ECT_WallMesh);
	float FillRatio = (float)FillableCells / (float)TotalCells;
	
	UE_LOG(LogTemp, Log, TEXT("ChunkyRoomGenerator::CreateGrid - Complete: "));
	UE_LOG(LogTemp, Log, TEXT("  Chunks placed: %d"), PlacedChunks.Num());
	UE_LOG(LogTemp, Log, TEXT("  Fillable cells (ECT_Empty): %d (%.2f%%)"), FillableCells, FillRatio * 100.0f);
	UE_LOG(LogTemp, Log, TEXT("  Boundary cells (ECT_WallMesh): %d"), BoundaryCells);
	UE_LOG(LogTemp, Log, TEXT("  Total cells:  %d"), TotalCells);
}

void UChunkyRoomGenerator::MarkChunkCells(const FRoomChunk& Chunk)
{
	for (int32 X = 0; X < Chunk.Size.X; ++X)
	{
		for (int32 Y = 0; Y < Chunk.Size.Y; ++Y)
		{
			FIntPoint Cell = Chunk.Position + FIntPoint(X, Y);
			SetCellState(Cell, EGridCellType::ECT_Empty);
		}
	}
	UE_LOG(LogTemp, Verbose, TEXT("    Marked chunk at (%d,%d) size %dx%d as fillable"),
		Chunk.Position.X, Chunk.Position. Y, Chunk.Size.X, Chunk.Size.Y);
}

bool UChunkyRoomGenerator::ValidateMinimumSize()
{
	// ✅ CHANGED: Check for ECT_Empty (fillable cells) instead of ECT_FloorMesh
	int32 FillableCells = GetCellCountByType(EGridCellType::ECT_Empty);
	const int32 MinCells = 16;  // Minimum 4x4 equivalent

	bool bValid = FillableCells >= MinCells;
	if (!bValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("  Validation failed: Only %d fillable cells (minimum:  %d)"),
			FillableCells, MinCells);
	}

	return bValid;
}

FIntPoint UChunkyRoomGenerator::GetRandomChunkSize()
{
	float Roll = RandomStream.FRand();

	if (Roll < Chunk2x2Chance)
	{
		return FIntPoint(2, 2);  // 2x2 chunk (200x200cm)
	}
	else if (Roll < Chunk2x2Chance + Chunk4x4Chance)
	{
		return FIntPoint(4, 4);  // 4x4 chunk (400x400cm)
	}
	else
	{
		// Rectangular chunk (2x4 or 4x2)
		bool bHorizontal = RandomStream.FRand() > 0.5f;
		return bHorizontal ? FIntPoint(4, 2) : FIntPoint(2, 4);
	}
}

bool UChunkyRoomGenerator::IsValidChunkPlacement(const FRoomChunk& Chunk) const
{
	// Check bounds
	if (Chunk.Position.X < 0 || Chunk.Position.Y < 0 ||
		Chunk.Position. X + Chunk.Size.X > GridSize.X ||
		Chunk.Position.Y + Chunk.Size.Y > GridSize. Y)
	{
		return false;
	}

	// Check for overlap with existing chunks
	for (const FRoomChunk& ExistingChunk : PlacedChunks)
	{
		if (Chunk. Overlaps(ExistingChunk))
		{
			return false;
		}
	}

	return true;
}

bool UChunkyRoomGenerator::TryPlaceAdjacentChunk(const FRoomChunk& NewChunk)
{
	// Check if valid placement
	if (!IsValidChunkPlacement(NewChunk))
	{
		return false;
	}

	// Check if adjacent to at least one existing chunk
	for (const FRoomChunk& ExistingChunk : PlacedChunks)
	{
		if (NewChunk.IsAdjacentTo(ExistingChunk))
		{
			return true;
		}
	}

	return false;
}

TArray<FIntPoint> UChunkyRoomGenerator::GetAdjacentPositions() const
{
	TArray<FIntPoint> Positions;
	TSet<FIntPoint> UniquePositions;

	// For each placed chunk, get positions around it
	for (const FRoomChunk& Chunk : PlacedChunks)
	{
		// Possible adjacent positions (cardinal directions)
		TArray<FIntPoint> Offsets = {
			FIntPoint(Chunk.Size.X, 0),  // East
			FIntPoint(-2, 0),             // West (assuming min chunk size 2)
			FIntPoint(0, Chunk.Size.Y),  // North
			FIntPoint(0, -2)              // South
		};

		for (const FIntPoint& Offset : Offsets)
		{
			FIntPoint NewPos = Chunk. Position + Offset;
			
			// Basic bounds check
			if (NewPos. X >= 0 && NewPos.Y >= 0 &&
				NewPos.X < GridSize.X - 2 && NewPos.Y < GridSize.Y - 2)
			{
				UniquePositions.Add(NewPos);
			}
		}
	}

	return UniquePositions.Array();
}

void UChunkyRoomGenerator:: ExecuteChunkyGeneration()
{
	PlacedChunks.Empty();

	// Determine number of chunks to place
	int32 TargetChunks = RandomStream.RandRange(MinChunks, MaxChunks);

	UE_LOG(LogTemp, Log, TEXT("  Target chunks to place: %d"), TargetChunks);

	// Place initial chunk at center of grid
	FIntPoint StartPos(0, 0);
	FIntPoint InitialSize = GetRandomChunkSize();
	FRoomChunk InitialChunk(StartPos, InitialSize);

	if (IsValidChunkPlacement(InitialChunk))
	{
		PlacedChunks.Add(InitialChunk);
		MarkChunkCells(InitialChunk);
		UE_LOG(LogTemp, Log, TEXT("    Placed initial chunk at (0,0) size %dx%d"), InitialSize.X, InitialSize.Y);
	}

	// Place additional chunks
	int32 AttemptsPerChunk = 20;
	int32 ChunksPlaced = 1;

	while (ChunksPlaced < TargetChunks)
	{
		bool bPlacedThisIteration = false;

		for (int32 Attempt = 0; Attempt < AttemptsPerChunk; ++Attempt)
		{
			// Get random adjacent position
			TArray<FIntPoint> AdjacentPositions = GetAdjacentPositions();
			
			if (AdjacentPositions.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("  No valid adjacent positions found, stopping"));
				break;
			}

			int32 RandomIndex = RandomStream.RandRange(0, AdjacentPositions.Num() - 1);
			FIntPoint NewPos = AdjacentPositions[RandomIndex];
			FIntPoint NewSize = GetRandomChunkSize();
			FRoomChunk NewChunk(NewPos, NewSize);

			if (TryPlaceAdjacentChunk(NewChunk))
			{
				PlacedChunks.Add(NewChunk);
				MarkChunkCells(NewChunk);
				ChunksPlaced++;
				bPlacedThisIteration = true;

				UE_LOG(LogTemp, Verbose, TEXT("    Placed chunk %d at (%d,%d) size %dx%d"),
					ChunksPlaced, NewPos.X, NewPos.Y, NewSize.X, NewSize. Y);
				break;
			}
		}

		if (!bPlacedThisIteration)
		{
			UE_LOG(LogTemp, Log, TEXT("  Could not place more chunks, stopping at %d"), ChunksPlaced);
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("  Chunky generation complete - Placed %d chunks"), PlacedChunks.Num());
}











