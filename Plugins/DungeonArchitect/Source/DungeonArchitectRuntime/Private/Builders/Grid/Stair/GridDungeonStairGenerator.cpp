//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/Stair/GridDungeonStairGenerator.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/Grid/GridDungeonBuilderLib.h"
#include "Builders/Grid/GridDungeonModel.h"

/////////////////////// Base Stair Generator ///////////////////////
void UGridDungeonBuilderStairGeneratorBase::GenerateDungeonHeights(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder, bool bFixHeights) const {
    using namespace GridDungeonBuilderImpl;
    // build the adjacency graph in memory
    if (GridModel->Cells.Num() == 0) return;
    TMap<int32, FCellHeightNode> CellHeightNodes;

    TSet<int32> Visited;
    for (const FGridDungeonCell& StartCell : GridModel->Cells) {
        if (Visited.Contains(StartCell.Id)) {
            continue;
        }
        
        TArray<CellHeightFrameInfo> Stack;
        Stack.Push(CellHeightFrameInfo(StartCell.Id, StartCell.Bounds.Location.Z));

        while (Stack.Num() > 0) {
            CellHeightFrameInfo Top = Stack.Pop();
            if (Visited.Contains(Top.CellId)) continue;
            Visited.Add(Top.CellId);

            FGridDungeonCell* Cell = GridModel->GetCell(Top.CellId);
            if (!Cell) continue;

            bool bApplyHeightVariation = (Cell->Bounds.Size.X > 1 && Cell->Bounds.Size.Y > 1);
            bApplyHeightVariation &= (Cell->CellType != EGridDungeonCellType::Room && Cell->CellType != EGridDungeonCellType::CorridorPadding);
            bApplyHeightVariation &= !Cell->UserDefined;
            bApplyHeightVariation &= !Cell->bHeightClamped;

            if (bApplyHeightVariation) {
                const float VariationProbability = FMath::Clamp(HeightVariationProbability, 0.0f, 1.0f);
                FRandomStream& Random = GridBuilder->GetRandomStreamRef();
                float RandomValue01 = Random.FRand();
                if (RandomValue01 < VariationProbability / 2.0f) {
                    Top.CurrentHeight--;
                }
                else if (RandomValue01 < VariationProbability) {
                    Top.CurrentHeight++;
                }
            }
            if (Cell->UserDefined || Cell->bHeightClamped) {
                Top.CurrentHeight = Cell->Bounds.Location.Z;
            }

            FCellHeightNode Node;
            Node.CellId = Cell->Id;
            Node.Height = Top.CurrentHeight;
            Node.MarkForIncrease = false;
            Node.MarkForDecrease = false;
            CellHeightNodes.Add(Node.CellId, Node);

            // Add the child nodes
            for (int32 ChildId : Cell->AdjacentCells) {
                if (Visited.Contains(ChildId)) continue;
                Stack.Push(CellHeightFrameInfo(ChildId, Top.CurrentHeight));
            }
        }
    }

    if (bFixHeights) {
        // Fix the dungeon heights
        const int32 FIX_MAX_TRIES = 50; // TODO: Move to gridConfig
        int32 FixIterations = 0;
        while (FixIterations < FIX_MAX_TRIES && FixDungeonCellHeights(GridModel, CellHeightNodes)) {
            FixIterations++;
        }
    }

    // Assign the calculated heights
    for (FGridDungeonCell& Cell : GridModel->Cells) {
        if (CellHeightNodes.Contains(Cell.Id)) {
            const FCellHeightNode& Node = CellHeightNodes[Cell.Id];
            Cell.Bounds.Location.Z = Node.Height;
        }
    }
}

bool UGridDungeonBuilderStairGeneratorBase::FixDungeonCellHeights(UGridDungeonModel* GridModel,
        TMap<int32, GridDungeonBuilderImpl::FCellHeightNode>& CellHeightNodes, const TSet<TPair<int32, int32>>& ClampedAdjacentNodes) const {
    using namespace GridDungeonBuilderImpl;
    bool bContinueIteration = false;
    if (GridModel->Cells.Num() == 0) return bContinueIteration;

    TSet<int32> Visited;
    TArray<int32> Stack;
    const FGridDungeonCell& RootCell = GridModel->Cells[0];
    Stack.Push(RootCell.Id);
    while (Stack.Num() > 0) {
        int32 CellId = Stack.Pop();
        if (Visited.Contains(CellId)) continue;
        Visited.Add(CellId);

        FGridDungeonCell* Cell = GridModel->GetCell(CellId);
        if (!Cell) continue;

        if (!CellHeightNodes.Contains(CellId)) continue;
        FCellHeightNode& HeightNode = CellHeightNodes[CellId];

        HeightNode.MarkForIncrease = false;
        HeightNode.MarkForDecrease = false;

        // Check if the adjacent cells have unreachable heights
        if (!Cell->bHeightClamped) {
            for (int32 ChildId : Cell->AdjacentCells) {
                FGridDungeonCell* ChildCell = GridModel->GetCell(ChildId);
                if (!ChildCell || !CellHeightNodes.Contains(ChildId)) continue;
                FCellHeightNode& childHeightNode = CellHeightNodes[ChildId];
                int32 HeightDifference = FMath::Abs(childHeightNode.Height - HeightNode.Height);
                int MaxAllowedHeight = MaxAllowedStairHeight;
                if (ClampedAdjacentNodes.Contains({ Cell->Id, ChildId })) {
                    MaxAllowedHeight = 0;
                }
            
                const bool bNotReachable = (HeightDifference > MaxAllowedHeight);
                if (bNotReachable) {
                    if (HeightNode.Height > childHeightNode.Height) {
                        HeightNode.MarkForDecrease = true;
                    }
                    else {
                        HeightNode.MarkForIncrease = true;
                    }
                    break;
                }
            }
        }

        // Add the child nodes
        for (int32 ChildId : Cell->AdjacentCells) {
            if (Visited.Contains(ChildId)) continue;
            Stack.Push(ChildId);
        }
    }


    TArray<int32> HeightCellIds;
    CellHeightNodes.GenerateKeyArray(HeightCellIds);
    bool bHeightChanged = false;
    for (int32 CellId : HeightCellIds) {
        FCellHeightNode& HeightNode = CellHeightNodes[CellId];
        if (HeightNode.MarkForDecrease) {
            HeightNode.Height--;
            bHeightChanged = true;
            HeightNode.MarkForDecrease = false;
        }
        else if (HeightNode.MarkForIncrease) {
            HeightNode.Height++;
            bHeightChanged = true;
            HeightNode.MarkForIncrease = false;
        }
    }

    // Iterate this function again if the height was changed in this step
    bContinueIteration = bHeightChanged;
    return bContinueIteration;
}

