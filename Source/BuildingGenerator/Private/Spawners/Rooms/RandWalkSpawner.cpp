// Fill out your copyright notice in the Description page of Project Settings.

#include "Spawners/Rooms/RandWalkSpawner.h"
#include "Generators/Room/RandWalkGenerator.h"
#include "DrawDebugHelpers.h"

ARandWalkSpawner::ARandWalkSpawner()
{
	// Set default values
	TargetFillRatio = 0.6f;
	BranchingChance = 0.3f;
	DirectionChangeChance = 0.4f;
	MaxActiveWalkers = 3;
	SmoothingIterations = 2;
	bRemoveDisconnectedRegions = true;
	RandomSeed = -1;
	bUseRandomSeed = false;

	bShowIrregularBoundary = true;
	IrregularBoundaryColor = FColor::Orange;
	BoundaryThickness = 3.0f;
}

bool ARandWalkSpawner::EnsureGeneratorReady()
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

	// KEY DIFFERENCE: Create RandWalkRoomGenerator instead of RoomGenerator
	if (! RoomGenerator)
	{
		DebugHelpers->LogVerbose(TEXT("Creating RandWalkRoomGenerator... "));
		RoomGenerator = NewObject<URandWalkGenerator>(this, TEXT("RandWalkRoomGenerator"));

		if (!RoomGenerator)
		{
			DebugHelpers->LogCritical(TEXT("Failed to create RandWalkRoomGenerator!"));
			return false;
		}
	}

	// Initialize if needed
	if (!RoomGenerator->IsInitialized())
	{
		DebugHelpers->LogVerbose(TEXT("Initializing RandWalkRoomGenerator..."));

		if (! RoomGenerator->Initialize(RoomData, RoomGridSize))
		{
			DebugHelpers->LogCritical(TEXT("Failed to initialize RandWalkRoomGenerator!"));
			return false;
		}

		// Set random walk parameters
		URandWalkGenerator* RandWalkGen = Cast<URandWalkGenerator>(RoomGenerator);
		if (RandWalkGen)
		{
			// Determine seed
			int32 SeedToUse = bUseRandomSeed ? FMath:: Rand() : (RandomSeed == -1 ? FMath:: Rand() : RandomSeed);

			DebugHelpers->LogVerbose(TEXT("Setting random walk parameters..."));
			DebugHelpers->LogVerbose(FString::Printf(TEXT("  Fill Ratio: %.2f"), TargetFillRatio));
			DebugHelpers->LogVerbose(FString::Printf(TEXT("  Branching Chance: %.2f"), BranchingChance));
			DebugHelpers->LogVerbose(FString:: Printf(TEXT("  Direction Change Chance: %.2f"), DirectionChangeChance));
			DebugHelpers->LogVerbose(FString::Printf(TEXT("  Max Walkers: %d"), MaxActiveWalkers));
			DebugHelpers->LogVerbose(FString::Printf(TEXT("  Smoothing Iterations: %d"), SmoothingIterations));
			DebugHelpers->LogVerbose(FString::Printf(TEXT("  Random Seed: %d"), SeedToUse));

			RandWalkGen->SetRandomWalkParams(
				TargetFillRatio,
				BranchingChance,
				DirectionChangeChance,
				MaxActiveWalkers,
				SmoothingIterations,
				bRemoveDisconnectedRegions,
				SeedToUse
			);
		}

		DebugHelpers->LogVerbose(TEXT("Creating irregular grid..."));
		RoomGenerator->CreateGrid();
	}

	return true;
}

void ARandWalkSpawner::UpdateVisualization()
{
	// Call base implementation first (draws grid, coordinates, cell states)
	Super::UpdateVisualization();

	// Add irregular boundary visualization
	if (bShowIrregularBoundary && RoomGenerator)
	{
		DrawIrregularBoundary();
	}
}

void ARandWalkSpawner::DrawIrregularBoundary()
{
	if (!RoomGenerator || !GetWorld())
		return;

	float CellSize = RoomGenerator->GetCellSize();
	FVector RoomOrigin = GetActorLocation();
	FIntPoint GridSize = RoomGenerator->GetGridSize();
	const TArray<EGridCellType>& GridState = RoomGenerator->GetGridState();

	// Draw boundary lines between occupied and empty cells
	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);
			EGridCellType CellState = RoomGenerator->GetCellState(Cell);

			if (CellState == EGridCellType::ECT_FloorMesh)
			{
				// Check each neighbor to draw boundary
				TArray<FIntPoint> Neighbors = {
					FIntPoint(X + 1, Y),  // East
					FIntPoint(X, Y + 1),  // North
					FIntPoint(X - 1, Y),  // West
					FIntPoint(X, Y - 1)   // South
				};

				for (const FIntPoint& Neighbor :  Neighbors)
				{
					// Draw line if neighbor is empty or out of bounds
					if (! RoomGenerator->IsValidGridCoordinate(Neighbor) ||
						RoomGenerator->GetCellState(Neighbor) == EGridCellType::ECT_Empty)
					{
						// Calculate line endpoints
						FVector CellCenter = RoomOrigin + FVector(X * CellSize + CellSize * 0.5f, Y * CellSize + CellSize * 0.5f, 10.0f);
						FVector NeighborCenter = RoomOrigin + FVector(Neighbor.X * CellSize + CellSize * 0.5f, Neighbor.Y * CellSize + CellSize * 0.5f, 10.0f);
						
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
							IrregularBoundaryColor,
							true,  // Persistent
							-1.0f,  // Lifetime
							0,  // Depth priority
							BoundaryThickness
						);
					}
				}
			}
		}
	}
}