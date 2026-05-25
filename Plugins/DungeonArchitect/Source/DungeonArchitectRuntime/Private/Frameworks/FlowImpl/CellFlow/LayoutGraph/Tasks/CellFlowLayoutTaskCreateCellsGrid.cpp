//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsGrid.h"

#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Utils/MathUtils.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStats.h"

void UCellFlowLayoutTaskCreateCellsGrid::GenerateCellsImpl(UDAFlowCellGraph* InGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const {
	const FIntPoint WorldLoc {0, 0};
	
	const FRandomStream& Random = *Input.Random;
	
	const int NumTiles = WorldSize.X * WorldSize.Y;
	TArray<int> CellTileMap;

	auto FnGetCellIdAtTile = [&](int x, int y) {
		if (x < 0 || x >= WorldSize.X || y < 0 || y >= WorldSize.Y) {
			return CellFlowConstants::CELL_INVALID;
		}
		const int Idx = y * WorldSize.X + x;
		return CellTileMap[Idx];
	};

	auto FnSetCellIdAtTile = [&](int x, int y, int Value) {
		if (x < 0 || x >= WorldSize.X || y < 0 || y >= WorldSize.Y) {
			return;
		}
		const int Idx = y * WorldSize.X + x;
		CellTileMap[Idx] = Value;
	};

	CellTileMap.SetNum(NumTiles);
	for (int i = 0; i < NumTiles; i++) {
		CellTileMap[i] = CellFlowConstants::CELL_EMPTY;
	}
	int CellIdCounter = 0;
	
	if (UCellFlowLayoutTaskCreateCellsGridFilter* GridFilter = Cast<UCellFlowLayoutTaskCreateCellsGridFilter>(Input.CustomParameters.GetObject(CellFlowConstants::CustomParameter_InitialGridFilterName))) {
		if (UCellFlowConfig* CellFlowConfig = Cast<UCellFlowConfig>(Input.CustomParameters.GetObject(TEXT("DungeonConfig")))) {
			FTransform DungeonTransform = Input.CustomParameters.GetTransform(TEXT("DungeonTransform"));
			DungeonTransform.SetScale3D(FVector::One());
			const FVector& GridSize = CellFlowConfig->GridSize;

			GridFilter->Filter(DungeonTransform, GridSize, WorldSize, CellTileMap);
		}
	}
	
	// Generate a shuffled tile coord list
	TArray<FIntPoint> TileCoords;
	{
		TileCoords.Reserve(NumTiles);

		for (int y = WorldLoc.Y; y < WorldLoc.Y + WorldSize.Y; y++) {
			for (int x = WorldLoc.X; x < WorldLoc.X + WorldSize.X; x++) {
				TileCoords.Add({x, y});
			}
		}
		FDAMathUtils::Shuffle(TileCoords, Random);
	}

	// Fit tiles in the tilemap
	{
		SCOPE_CYCLE_COUNTER(STAT_CFBuild_FitTiles);
		for (const FIntPoint& Coord : TileCoords) {
			for (int t = 0; t < FitIterations; t++) {
				int CellWidth = Random.RandRange(MinCellSize, MaxCellSize);
				int CellHeight;
				{
					int CellHeightMin = static_cast<int>(round(CellWidth * MinAspectRatio));
					int CellHeightMax = static_cast<int>(round(CellWidth * MaxAspectRatio));
					CellHeightMin = FMath::Clamp(CellHeightMin, MinCellSize, MaxCellSize);
					CellHeightMax = FMath::Clamp(CellHeightMax, MinCellSize, MaxCellSize);
					CellHeight = Random.RandRange(CellHeightMin, CellHeightMax);
				}

				bool bCanFit = true;
				// Check if we can fit a cell of this size
				for (int cy = 0; cy < CellHeight; cy++) {
					for (int cx = 0; cx < CellWidth; cx++) {
						const int CellIdAtTile = FnGetCellIdAtTile(Coord.X + cx, Coord.Y + cy);
						if (CellIdAtTile != CellFlowConstants::CELL_EMPTY || CellIdAtTile == CellFlowConstants::CELL_INVALID) {
							bCanFit = false;
							break;

						}
					}
					if (!bCanFit) break;
				}

				if (bCanFit) {
					UDAFlowCellLeafNodeGrid* Node = InGraph->CreateLeafNode<UDAFlowCellLeafNodeGrid>();
					Node->CellId = CellIdCounter++;
					for (int cy = 0; cy < CellHeight; cy++) {
						for (int cx = 0; cx < CellWidth; cx++) {
							FIntPoint LeafTileCoord(Coord.X + cx, Coord.Y + cy);
							Node->LeafTileCoords.Add(LeafTileCoord);
							FnSetCellIdAtTile(LeafTileCoord.X, LeafTileCoord.Y, Node->CellId);
						}
					}
					break;
				}
			}
		}
	}

	// Build the cell graph
	{
		SCOPE_CYCLE_COUNTER(STAT_CFBuild_BuildCellGraph);
		const int NumCells = InGraph->LeafNodes.Num();
		TArray<TSet<int>> Connections;
		Connections.SetNum(NumCells);

		static const TArray<FIntPoint> NCoords = {
			{-1, 0 },
			{ 1, 0 },
			{ 0, -1 },
			{ 0, 1 }
		};

		for (int y = WorldLoc.Y; y < WorldLoc.Y + WorldSize.Y; y++) {
			for (int x = WorldLoc.X; x < WorldLoc.X + WorldSize.X; x++) {
				const int GroupId = FnGetCellIdAtTile(x, y);
				if (GroupId == CellFlowConstants::CELL_EMPTY || GroupId == CellFlowConstants::CELL_INVALID) {
					continue;
				}
				for (const FIntPoint& NCoord : NCoords) {
					const int dx = NCoord.X;
					const int dy = NCoord.Y;
					const int NGroupID = FnGetCellIdAtTile(x + dx, y + dy);
					if (NGroupID != GroupId && NGroupID != CellFlowConstants::CELL_EMPTY && NGroupID != CellFlowConstants::CELL_INVALID) {
						Connections[GroupId].Add(NGroupID);
						Connections[NGroupID].Add(GroupId);
					}
				}
			}
		}

		InGraph->GroupNodes.Reset();
		InGraph->GroupNodes.SetNum(NumCells);
		for (int i = 0; i < NumCells; i++) {
			FDAFlowCellGroupNode& GroupNode = InGraph->GroupNodes[i];
			GroupNode.GroupId = i;
			GroupNode.LeafNodes.Add(i);    // each node belongs to its own group initially
			GroupNode.Connections = Connections[i];
			GroupNode.GroupColor = FColorUtils::GetRandomColor(Random, 0.3f);
		}
	}
}

