// Fill out your copyright notice in the Description page of Project Settings.

#include "Spawners/Rooms/ChunkyRoomSpawner.h"
#include "Generators/Rooms/ChunkyRoomGenerator.h"
#include "DrawDebugHelpers.h"

AChunkyRoomSpawner::AChunkyRoomSpawner()
{
	// Set default values
	MinChunks = 3;
	MaxChunks = 8;
	Chunk2x2Chance = 0.4f;
	Chunk4x4Chance = 0.3f;
	ChunkRectChance = 0.3f;
	RandomSeed = -1;
	bUseRandomSeed = false;

	// Debug visualization
	bShowChunkBoundaries = true;
	ChunkBoundaryColor = FColor::Cyan;
	ChunkBoundaryThickness = 2.0f;
}

bool AChunkyRoomSpawner::EnsureGeneratorReady()
{
	// Validate RoomData
	if (!RoomData)
	{
		DebugHelpers->LogCritical(TEXT("RoomData is not assigned!"));
		return false;
	}

	if (RoomGridSize.X < 4 || RoomGridSize.Y < 4)
	{
		DebugHelpers->LogCritical(TEXT("GridSize is too small (min 4x4)!"));
		return false;
	}

	// Create ChunkyRoomGenerator if needed
	if (!RoomGenerator)
	{
		DebugHelpers->LogVerbose(TEXT("Creating ChunkyRoomGenerator..."));
		RoomGenerator = NewObject<UChunkyRoomGenerator>(this, TEXT("ChunkyRoomGenerator"));

		if (!RoomGenerator)
		{
			DebugHelpers->LogCritical(TEXT("Failed to create ChunkyRoomGenerator!"));
			return false;
		}
	}

	ChunkyGen = Cast<UChunkyRoomGenerator>(RoomGenerator);
	if (!ChunkyGen)
	{
		DebugHelpers->LogCritical(TEXT("RoomGenerator is not a ChunkyRoomGenerator!"));
		return false;
	}

	// Initialize if needed
	if (!ChunkyGen->IsInitialized())
	{
		DebugHelpers->LogVerbose(TEXT("Initializing ChunkyRoomGenerator..."));

		if (!ChunkyGen->Initialize(RoomData, RoomGridSize))
		{
			DebugHelpers->LogCritical(TEXT("Failed to initialize ChunkyRoomGenerator!"));
			return false;
		}

		// Calculate seed
		int32 FinalSeed = bUseRandomSeed ? FMath::Rand() : (RandomSeed == -1 ? FMath:: Rand() : RandomSeed);

		// Set chunky parameters
		DebugHelpers->LogVerbose(TEXT("Setting chunky generation parameters..."));
		DebugHelpers->LogVerbose(FString::Printf(TEXT("  Chunks:  %d-%d"), MinChunks, MaxChunks));
		DebugHelpers->LogVerbose(FString::Printf(TEXT("  2x2 Chance: %.2f"), Chunk2x2Chance));
		DebugHelpers->LogVerbose(FString::Printf(TEXT("  4x4 Chance: %.2f"), Chunk4x4Chance));
		DebugHelpers->LogVerbose(FString::Printf(TEXT("  Rect Chance: %.2f"), ChunkRectChance));
		DebugHelpers->LogVerbose(FString::Printf(TEXT("  Random Seed: %d"), FinalSeed));

		ChunkyGen->SetChunkyParams(
			MinChunks,
			MaxChunks,
			Chunk2x2Chance,
			Chunk4x4Chance,
			ChunkRectChance,
			FinalSeed
		);

		DebugHelpers->LogVerbose(TEXT("Creating chunky grid..."));
		ChunkyGen->CreateGrid();
	}

	return true;
}

void AChunkyRoomSpawner::UpdateVisualization()
{
	// Call base implementation first (draws grid, coordinates, cell states)
	Super::UpdateVisualization();

	// Add chunk boundary visualization
	if (bShowChunkBoundaries && ChunkyGen)
	{
		DrawChunkBoundaries();
	}
}

void AChunkyRoomSpawner::DrawChunkBoundaries()
{
	if (!ChunkyGen || !GetWorld())
		return;

	float CellSize = ChunkyGen->GetCellSize();
	FVector RoomOrigin = GetActorLocation();

	// Note: You'll need to add a getter for PlacedChunks in ChunkyRoomGenerator
	// For now, we'll draw based on floor cell groups

	FIntPoint GridSize = ChunkyGen->GetGridSize();
	
	// Draw boundaries between floor cells and empty cells
	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);
			EGridCellType CellState = ChunkyGen->GetCellState(Cell);

			if (CellState == EGridCellType::ECT_FloorMesh)
			{
				// Check each cardinal neighbor
				TArray<FIntPoint> Neighbors = {
					FIntPoint(X + 1, Y),  // East
					FIntPoint(X, Y + 1),  // North
					FIntPoint(X - 1, Y),  // West
					FIntPoint(X, Y - 1)   // South
				};

				for (const FIntPoint& Neighbor :  Neighbors)
				{
					// Draw line if neighbor is empty or out of bounds
					if (! ChunkyGen->IsValidGridCoordinate(Neighbor) ||
						ChunkyGen->GetCellState(Neighbor) == EGridCellType::ECT_Empty)
					{
						// Calculate line endpoints
						FVector CellCenter = RoomOrigin + FVector(
							X * CellSize + CellSize * 0.5f, 
							Y * CellSize + CellSize * 0.5f, 
							10.0f
						);
						
						FVector NeighborCenter = RoomOrigin + FVector(
							Neighbor.X * CellSize + CellSize * 0.5f, 
							Neighbor. Y * CellSize + CellSize * 0.5f, 
							10.0f
						);
						
						FVector MidPoint = (CellCenter + NeighborCenter) * 0.5f;

						// Draw line perpendicular to connection
						FVector Direction = (NeighborCenter - CellCenter).GetSafeNormal();
						FVector Perpendicular = FVector(-Direction.Y, Direction.X, 0.0f) * (CellSize * 0.5f);

						FVector LineStart = MidPoint + Perpendicular;
						FVector LineEnd = MidPoint - Perpendicular;

						DrawDebugLine(
							GetWorld(),
							LineStart,
							LineEnd,
							ChunkBoundaryColor,
							true,  // Persistent
							-1.0f, // Lifetime
							0,     // Depth priority
							ChunkBoundaryThickness
						);
					}
				}
			}
		}
	}
}