// Fill out your copyright notice in the Description page of Project Settings.

#include "Generators/Rooms/ChunkyRoomGenerator.h"





void UChunkyRoomGenerator::CreateGrid()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("UChunkyRoomGenerator::CreateGrid - Generator not initialized!"));
		return;
	}

	// Initialize random stream
	if (RandomSeed == -1)
	{
		RandomStream.Initialize(FMath::Rand());
	}
	else
	{
		RandomStream.Initialize(RandomSeed);
	}

	UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::CreateGrid - Creating chunky room..."));

	// Step 1: Initialize grid (all cells empty)
	int32 TotalCells = GridSize.X * GridSize.Y;
	GridState.SetNum(TotalCells);
	for (EGridCellType& Cell : GridState)
	{
		Cell = EGridCellType::ECT_Empty;
	}

	// Step 2: Calculate base room bounds (centered in grid)
	BaseRoomSize.X = FMath::Max(4, (int32)(GridSize.X * BaseRoomPercentage));
	BaseRoomSize.Y = FMath::Max(4, (int32)(GridSize.Y * BaseRoomPercentage));
	BaseRoomStart.X = 0;
	BaseRoomStart.Y = 0;

	UE_LOG(LogTemp, Verbose, TEXT("  Base room: Start(%d, %d), Size(%d, %d)"),
		BaseRoomStart.X, BaseRoomStart.Y, BaseRoomSize.X, BaseRoomSize.Y);

	// Step 3: Mark base room cells as floor
	MarkRectangle(BaseRoomStart.X, BaseRoomStart.Y, BaseRoomSize.X, BaseRoomSize.Y);

	// Step 4: Add random protrusions
	int32 NumProtrusions = RandomStream.RandRange(MinProtrusions, MaxProtrusions);
	UE_LOG(LogTemp, Verbose, TEXT("  Adding %d protrusions..."), NumProtrusions);

	for (int32 i = 0; i < NumProtrusions; ++i)
	{
		AddRandomProtrusion();
	}

	// Step 5: Log statistics
	int32 FloorCells = GetCellCountByType(EGridCellType::ECT_FloorMesh);
	int32 EmptyCells = GetCellCountByType(EGridCellType::ECT_Empty);
	float Occupancy = GetOccupancyPercentage();

	UE_LOG(LogTemp, Log, TEXT("UChunkyRoomGenerator::CreateGrid - Complete"));
	UE_LOG(LogTemp, Log, TEXT("  Grid: %d x %d (%d cells)"), GridSize.X, GridSize.Y, TotalCells);
	UE_LOG(LogTemp, Log, TEXT("  Floor: %d cells (%.1f%%)"), FloorCells, Occupancy);
	UE_LOG(LogTemp, Log, TEXT("  Empty: %d cells"), EmptyCells);
	UE_LOG(LogTemp, Log, TEXT("  Protrusions: %d"), NumProtrusions);
}

bool UChunkyRoomGenerator::GenerateFloor()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateWalls()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateCorners()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateDoorways()
{
	return false;
}

bool UChunkyRoomGenerator::GenerateCeiling()
{
	return false;
}

#pragma region Internal Helpers
void UChunkyRoomGenerator::MarkRectangle(int32 StartX, int32 StartY, int32 Width, int32 Height)
{
	// Mark all cells in rectangle as floor
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			FIntPoint Cell(StartX + X, StartY + Y);
			
			// Bounds check
			if (IsValidGridCoordinate(Cell))
			{
				SetCellState(Cell, EGridCellType::ECT_FloorMesh);
			}
		}
	}
}

void UChunkyRoomGenerator::AddRandomProtrusion()
{
	// Pick random edge (0=North, 1=South, 2=East, 3=West)
	int32 EdgeIndex = RandomStream.RandRange(0, 3);
	EWallEdge Edge = (EWallEdge)EdgeIndex;

	// Pick random protrusion dimensions
	int32 ProtrusionWidth = RandomStream.RandRange(MinProtrusionSize, MaxProtrusionSize);
	int32 ProtrusionDepth = RandomStream.RandRange(MinProtrusionSize, MaxProtrusionSize);

	// Calculate protrusion rectangle based on edge
	FIntPoint Start;
	FIntPoint Size;

	switch (Edge)
	{
		case EWallEdge::North: // Extend upward (positive Y)
		{
			// Pick random position along top edge
			int32 EdgeLength = BaseRoomSize.X;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X + Position, BaseRoomStart.Y + BaseRoomSize.Y);
			Size = FIntPoint(ProtrusionWidth, ProtrusionDepth);
			break;
		}

		case EWallEdge::South: // Extend downward (negative Y)
		{
			// Pick random position along bottom edge
			int32 EdgeLength = BaseRoomSize.X;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X + Position, BaseRoomStart.Y - ProtrusionDepth);
			Size = FIntPoint(ProtrusionWidth, ProtrusionDepth);
			break;
		}

		case EWallEdge::East: // Extend right (positive Y direction in grid)
		{
			// Pick random position along right edge
			int32 EdgeLength = BaseRoomSize.Y;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X + BaseRoomSize.X, BaseRoomStart.Y + Position);
			Size = FIntPoint(ProtrusionDepth, ProtrusionWidth);
			break;
		}

		case EWallEdge::West: // Extend left (negative X)
		{
			// Pick random position along left edge
			int32 EdgeLength = BaseRoomSize.Y;
			int32 Position = RandomStream.RandRange(0, FMath::Max(1, EdgeLength - ProtrusionWidth));
			
			Start = FIntPoint(BaseRoomStart.X - ProtrusionDepth, BaseRoomStart.Y + Position);
			Size = FIntPoint(ProtrusionDepth, ProtrusionWidth);
			break;
		}

		default:
			return;
	}

	// Clamp to grid bounds
	int32 ClampedStartX = FMath::Clamp(Start.X, 0, GridSize.X - 1);
	int32 ClampedStartY = FMath::Clamp(Start.Y, 0, GridSize.Y - 1);
	int32 ClampedWidth = FMath::Min(Size.X, GridSize.X - ClampedStartX);
	int32 ClampedHeight = FMath::Min(Size.Y, GridSize.Y - ClampedStartY);

	// Only add if there's valid space
	if (ClampedWidth >= MinProtrusionSize && ClampedHeight >= MinProtrusionSize)
	{
		MarkRectangle(ClampedStartX, ClampedStartY, ClampedWidth, ClampedHeight);
		
		UE_LOG(LogTemp, Verbose, TEXT("    Added protrusion on edge %d: Start(%d,%d), Size(%d,%d)"),
			EdgeIndex, ClampedStartX, ClampedStartY, ClampedWidth, ClampedHeight);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("    Protrusion too small after clamping, skipped"));
	}
}

bool UChunkyRoomGenerator::HasFloorNeighbor(FIntPoint Cell, FIntPoint Direction) const
{
	FIntPoint Neighbor = Cell + Direction;
	
	if (!IsValidGridCoordinate(Neighbor))
	{
		return false;
	}
	
	return GetCellState(Neighbor) == EGridCellType::ECT_FloorMesh;
}

TArray<FIntPoint> UChunkyRoomGenerator::GetPerimeterCells() const
{
	TArray<FIntPoint> PerimeterCells;

	// Check all floor cells to see if they're on the perimeter
	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint Cell(X, Y);
			
			if (GetCellState(Cell) != EGridCellType::ECT_FloorMesh)
			{
				continue; // Not a floor cell
			}

			// Check if any neighbor is empty or out of bounds
			TArray<FIntPoint> Directions = {
				FIntPoint(1, 0),   // East
				FIntPoint(-1, 0),  // West
				FIntPoint(0, 1),   // North
				FIntPoint(0, -1)   // South
			};

			bool IsPerimeter = false;
			for (const FIntPoint& Dir : Directions)
			{
				FIntPoint Neighbor = Cell + Dir;
				
				// Perimeter if neighbor is out of bounds or empty
				if (!IsValidGridCoordinate(Neighbor) || 
					GetCellState(Neighbor) == EGridCellType::ECT_Empty)
				{
					IsPerimeter = true;
					break;
				}
			}

			if (IsPerimeter)
			{
				PerimeterCells.Add(Cell);
			}
		}
	}

	return PerimeterCells;
}
#pragma endregion