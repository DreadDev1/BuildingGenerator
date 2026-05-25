//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/Stair/GridDungeonStairGeneratorLegacy.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/Grid/GridDungeonBuilderLib.h"
#include "Builders/Grid/GridDungeonConfig.h"
#include "Builders/Grid/GridDungeonModel.h"

/////////////////////// Legacy Stair Generator ///////////////////////

void UGridDungeonBuilderStairGeneratorLegacy::Generate(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder) const {
    UGridDungeonConfig* GridConfig = Cast<UGridDungeonConfig>(GetOuter());
    if (!GridModel || !GridBuilder || !GridConfig) {
        return;
    }
    
    GenerateDungeonHeights(GridModel, GridBuilder, true);
    
    int IterationWeights[] = {100, 50, 0, -50, -80, -130};
    for (int WeightIndex = 0; WeightIndex < 4; WeightIndex++) {
        ConnectStairs(GridModel, GridBuilder, GridConfig->GridCellSize, IterationWeights[WeightIndex]);
    }
}

void UGridDungeonBuilderStairGeneratorLegacy::ConnectStairs(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder, const FVector& GridToMeshScale, int32 WeightThreshold) const {
    using namespace GridDungeonBuilderImpl;
    if (!GridModel || !GridBuilder || GridModel->Cells.Num() == 0) return;
    
    
    TSet<uint32> visited;
    TSet<int32> islandVisited; // The node visited, to track multiple isolated islands
    // Loop through all the nodes, pick only one node from an island.  The entire island is then processed within the inner while loop with BFS
    for (int i = 0; i < GridModel->Cells.Num(); i++) {
        const FGridDungeonCell& start = GridModel->Cells[i];
        if (islandVisited.Contains(start.Id)) {
            // This island has already been processed
            continue;
        }
        TArray<StairEdgeInfo> Stack;
        Stack.Push(StairEdgeInfo(-1, start.Id));
        while (!Stack.IsEmpty()) {
            StairEdgeInfo top = Stack.Pop();
            if (top.CellIdA >= 0) {
                int32 hash1 = HASH(top.CellIdA, top.CellIdB);
                int32 hash2 = HASH(top.CellIdB, top.CellIdA);
                if (visited.Contains(hash1) || visited.Contains(hash2)) {
                    // Already processed
                    continue;
                }
                // Mark as processed
                visited.Add(hash1);
                visited.Add(hash2);

                islandVisited.Add(top.CellIdA);
                islandVisited.Add(top.CellIdB);

                // Check if it is really required to place a stair here.  There might be other paths nearby to this cell
                bool pathExists = GridBuilder->ContainsAdjacencyPath(top.CellIdA, top.CellIdB, StairConnectionTolerance,
                                                        true);
                if (!pathExists) {
                    // Process the edge
                    FGridDungeonCell* cellA = GridModel->GetCell(top.CellIdA);
                    FGridDungeonCell* cellB = GridModel->GetCell(top.CellIdB);
                    if (!cellA || !cellB) continue;
                    if (cellA->Bounds.Location.Z != cellB->Bounds.Location.Z) {
                        bool bothRooms = false;
                        if (cellA->CellType == EGridDungeonCellType::Room && cellB->CellType == EGridDungeonCellType::Room) {
                            bothRooms = true;

                        }
                        // Find the intersecting line
                        FRectangle intersection = FRectangle::Intersect(cellA->Bounds, cellB->Bounds);
                        if (intersection.Size.X > 0) {
                            bool cellAAbove = (cellA->Bounds.Location.Z > cellB->Bounds.Location.Z);
                            FGridDungeonCell* stairOwner = (cellAAbove ? cellB : cellA);
                            FGridDungeonCell* stairConnectedTo = (!cellAAbove ? cellB : cellA);

                            if (GridBuilder->ContainsStair(stairOwner->Id, stairConnectedTo->Id)) {
                                // Stair already exists here. Move to the next one
                                continue;
                            }

                            bool cellOwnerOnLeft = (stairOwner->Bounds.Center().Y < intersection.Location.Y);

                            bool foundValidStairLocation = false;
                            int32 validX = intersection.Location.X;

                            int32 validY = intersection.Location.Y;
                            if (cellOwnerOnLeft) validY--;

                            TArray<StairConnectionWeight> StairConnectionCandidates;
                            for (validX = intersection.Location.X; validX < intersection.Location.X + intersection.Size.X; validX++) {
                                auto currentPointInfo = GridModel->GetGridCellLookup(validX, validY);
                                if (stairOwner->CellType == EGridDungeonCellType::Room || stairConnectedTo->CellType ==
                                    EGridDungeonCellType::Room) {
                                    // Make sure the stair is on a door cell
                                    FGridDungeonCellInfo stairCellInfo = GridModel->GetGridCellLookup(validX, validY);
                                    if (!stairCellInfo.ContainsDoor) {
                                        // Stair not connected to a door. Probably trying to attach itself to a room wall. ignore
                                        continue;
                                    }

                                    // We have a door here.  A stair case is a must, but first make sure we have a door between these two cells 
                                    bool hasDoor = GridModel->DoorManager.ContainsDoorBetweenCells(
                                        stairOwner->Id, stairConnectedTo->Id);
                                    if (!hasDoor) continue;

                                    // Check again in more detail
                                    auto tz1 = validY;
                                    auto tz2 = validY - 1;
                                    if (cellOwnerOnLeft) {
                                        tz2 = validY + 1;
                                    }

                                    hasDoor = GridModel->DoorManager.ContainsDoor(validX, tz1, validX, tz2);
                                    if (hasDoor) {
                                        StairConnectionCandidates.Add(StairConnectionWeight(validX, 100));
                                        foundValidStairLocation = true;
                                        break;
                                    }
                                }
                                else {
                                    // Both the cells are non-rooms (corridors)
                                    int32 weight = 0;

                                    FGridDungeonCellInfo cellInfo0 = GridModel->GetGridCellLookup(validX, validY - 1);
                                    FGridDungeonCellInfo cellInfo1 = GridModel->GetGridCellLookup(validX, validY + 1);
                                    weight += (cellInfo0.CellType != EGridDungeonCellType::Unknown) ? 10 : 0;
                                    weight += (cellInfo1.CellType != EGridDungeonCellType::Unknown) ? 10 : 0;

                                    int adjacentY = cellOwnerOnLeft ? (validY - 1) : (validY + 1);
                                    if (currentPointInfo.ContainsDoor) {
                                        // Increase the weight if we connect into a door
                                        bool ownerOnDoor = GridModel->DoorManager.ContainsDoor(
                                            validX, validY, validX, adjacentY);
                                        if (ownerOnDoor) {
                                            // Connect to this
                                            weight += 100;
                                        }
                                        else {
                                            // Add a penalty if we are creating a stair blocking a door entry/exit
                                            weight -= 100;
                                        }
                                    }
                                    else {
                                        // Make sure we don't connect to a wall
                                        FGridDungeonCellInfo adjacentOwnerCellInfo = GridModel->GetGridCellLookup(
                                            validX, adjacentY);
                                        if (adjacentOwnerCellInfo.CellType == EGridDungeonCellType::Room) {
                                            // We are connecting to a wall. Add a penalty
                                            weight -= 100;
                                        }
                                    }

                                    if (GridModel->ContainsStairAtLocation(validX, validY)) {
                                        weight -= 90;
                                    }

                                    // Check the side of the stairs to see if we are not blocking a stair entry / exit
                                    if (GridModel->ContainsStairAtLocation(validX - 1, validY)) weight -= 60;
                                    if (GridModel->ContainsStairAtLocation(validX + 1, validY)) weight -= 60;
                                    if (GridModel->ContainsStairAtLocation(validX - 1, adjacentY)) {
                                        weight -= 10;
                                    }
                                    if (GridModel->ContainsStairAtLocation(validX + 1, adjacentY)) {
                                        weight -= 10;
                                    }
                                    if (GridModel->ContainsStairAtLocation(validX, adjacentY)) {
                                        weight -= 100;
                                    }

                                    StairConnectionCandidates.Add(StairConnectionWeight(validX, weight));
                                }
                            }

                            // Create a stair if necessary
                            if (StairConnectionCandidates.Num() > 0) {
                                StairConnectionCandidates.Sort();
                                StairConnectionWeight candidate = StairConnectionCandidates[0];
                                if (candidate.weight < WeightThreshold) {
                                    continue;
                                }

                                validX = candidate.position;
                                int stairZ = stairOwner->Bounds.Location.Z;
                                int paddingOffset = (stairOwner->Bounds.Y() > stairConnectedTo->Bounds.Y()) ? 1 : -1;
                                // Add a corridor padding here
                                for (int dx = -1; dx <= 1; dx++) {
                                    bool requiresPadding = false;
                                    if (dx == 0) {
                                        requiresPadding = true;
                                    }
                                    else {
                                        auto cellInfo = GridModel->GetGridCellLookup(validX + dx, validY);
                                        if (cellInfo.CellType != EGridDungeonCellType::Unknown) {
                                            requiresPadding = true;
                                        }
                                    }

                                    if (requiresPadding) {
                                        auto paddingInfo = GridModel->GetGridCellLookup(
                                            validX + dx, validY + paddingOffset);
                                        if (paddingInfo.CellType == EGridDungeonCellType::Unknown) {
                                            GridBuilder->AddCorridorPadding(validX + dx, validY + paddingOffset, stairZ);
                                        }
                                    }
                                }
                                GridModel->BuildCellLookup();
                                GridBuilder->GenerateAdjacencyLookup();
                            }
                            else {
                                continue;
                            }

                            float validZ = stairOwner->Bounds.Location.Z;
                            FVector StairLocation(validX, validY, validZ);
                            StairLocation += FVector(0.5f, 0.5f, 0);
                            StairLocation *= GridToMeshScale;

                            FQuat StairRotation = FQuat::Identity;
                            StairRotation = FQuat(FVector(0, 0, 1), cellOwnerOnLeft ? -PI / 2 : PI / 2);

                            if (!GridModel->CellStairsLookup.Contains(stairOwner->Id)) {
                                GridModel->CellStairsLookup.Add(stairOwner->Id, TArray<FGridDungeonStairInfo>());
                            }
                            FGridDungeonStairInfo Stair;
                            Stair.OwnerCell = stairOwner->Id;
                            Stair.ConnectedToCell = stairConnectedTo->Id;
                            Stair.Position = StairLocation;
                            Stair.Rotation = StairRotation;
                            Stair.IPosition = FIntVector(validX, validY, validZ);
                            GridModel->CellStairsLookup[stairOwner->Id].Add(Stair);
                        }
                        else if (intersection.Size.Y > 0) {
                            bool cellAAbove = (cellA->Bounds.Location.Z > cellB->Bounds.Location.Z);

                            FGridDungeonCell* stairOwner = (cellAAbove ? cellB : cellA);
                            FGridDungeonCell* stairConnectedTo = (!cellAAbove ? cellB : cellA);

                            if (GridBuilder->ContainsStair(stairOwner->Id, stairConnectedTo->Id)) {
                                // Stair already exists here. Move to the next one
                                continue;
                            }

                            bool cellOwnerOnLeft = (stairOwner->Bounds.Center().X < intersection.Location.X);

                            float validX = intersection.Location.X;
                            if (cellOwnerOnLeft) validX--;

                            float validY = intersection.Location.Y;
                            bool foundValidStairLocation = false;

                            TArray<StairConnectionWeight> StairConnectionCandidates;
                            for (validY = intersection.Location.Y; validY < intersection.Location.Y + intersection
                                                                                                      .Size.Y; validY++
                            ) {
                                auto currentPointInfo = GridModel->GetGridCellLookup(validX, validY);
                                if (stairOwner->CellType == EGridDungeonCellType::Room || stairConnectedTo->CellType ==
                                    EGridDungeonCellType::Room) {
                                    // Make sure the stair is on a door cell
                                    FGridDungeonCellInfo stairCellInfo = GridModel->GetGridCellLookup(validX, validY);
                                    if (!stairCellInfo.ContainsDoor) {
                                        // Stair not connected to a door. Probably trying to attach itself to a room wall. ignore
                                        continue;
                                    }

                                    // We have a door here.  A stair case is a must, but first make sure we have a door between these two cells 
                                    bool hasDoor = GridModel->DoorManager.ContainsDoorBetweenCells(
                                        stairOwner->Id, stairConnectedTo->Id);
                                    if (!hasDoor) continue;

                                    // Check again in more detail
                                    auto tx1 = validX;
                                    auto tx2 = validX - 1;
                                    if (cellOwnerOnLeft) {
                                        tx2 = validX + 1;
                                    }

                                    hasDoor = GridModel->DoorManager.ContainsDoor(tx1, validY, tx2, validY);
                                    if (hasDoor) {
                                        StairConnectionCandidates.Add(StairConnectionWeight(validY, 100));
                                        foundValidStairLocation = true;
                                        break;
                                    }
                                }
                                else {
                                    // Both the cells are non-rooms (corridors)
                                    int32 weight = 0;

                                    FGridDungeonCellInfo cellInfo0 = GridModel->GetGridCellLookup(validX - 1, validY);
                                    FGridDungeonCellInfo cellInfo1 = GridModel->GetGridCellLookup(validX + 1, validY);
                                    weight += (cellInfo0.CellType != EGridDungeonCellType::Unknown) ? 10 : 0;
                                    weight += (cellInfo1.CellType != EGridDungeonCellType::Unknown) ? 10 : 0;

                                    int adjacentX = cellOwnerOnLeft ? (validX - 1) : (validX + 1);

                                    if (currentPointInfo.ContainsDoor) {
                                        // Increase the weight if we connect into a door
                                        bool ownerOnDoor = GridModel->DoorManager.ContainsDoor(
                                            validX, validY, adjacentX, validY);
                                        if (ownerOnDoor) {
                                            // Connect to this
                                            weight += 100;
                                        }
                                        else {
                                            // Add a penalty if we are creating a stair blocking a door entry/exit
                                            weight -= 100;
                                        }
                                    }
                                    else {
                                        // Make sure we don't connect to a wall
                                        FGridDungeonCellInfo adjacentOwnerCellInfo = GridModel->GetGridCellLookup(
                                            adjacentX, validY);
                                        if (adjacentOwnerCellInfo.CellType == EGridDungeonCellType::Room) {
                                            // We are connecting to a wall. Add a penalty
                                            weight -= 100;
                                        }
                                    }

                                    if (GridModel->ContainsStairAtLocation(validX, validY)) weight -= 90;

                                    // Check the side of the stairs to see if we are not blocking a stair entry / exit
                                    if (GridModel->ContainsStairAtLocation(validX, validY - 1)) weight -= 60;
                                    if (GridModel->ContainsStairAtLocation(validX, validY + 1)) weight -= 60;
                                    if (GridModel->ContainsStairAtLocation(adjacentX, validY - 1)) {
                                        weight -= 10;
                                    }
                                    if (GridModel->ContainsStairAtLocation(adjacentX, validY + 1)) {
                                        weight -= 10;
                                    }
                                    if (GridModel->ContainsStairAtLocation(adjacentX, validY)) {
                                        weight -= 100;
                                    }

                                    StairConnectionCandidates.Add(StairConnectionWeight(validY, weight));
                                }
                            }

                            // Connect the stairs if necessary
                            if (StairConnectionCandidates.Num() > 0) {
                                StairConnectionCandidates.Sort();
                                StairConnectionWeight candidate = StairConnectionCandidates[0];
                                if (candidate.weight < WeightThreshold) {
                                    continue;
                                }
                                validY = candidate.position;
                                int stairZ = stairOwner->Bounds.Location.Z;
                                int paddingOffset = (stairOwner->Bounds.X() > stairConnectedTo->Bounds.X()) ? 1 : -1;
                                // Add a corridor padding here
                                for (int dy = -1; dy <= 1; dy++) {
                                    bool requiresPadding = false;
                                    if (dy == 0) {
                                        requiresPadding = true;
                                    }
                                    else {
                                        auto cellInfo = GridModel->GetGridCellLookup(validX, validY + dy);
                                        if (cellInfo.CellType != EGridDungeonCellType::Unknown) {
                                            requiresPadding = true;
                                        }
                                    }

                                    if (requiresPadding) {
                                        auto paddingInfo = GridModel->GetGridCellLookup(
                                            validX + paddingOffset, validY + dy);
                                        if (paddingInfo.CellType == EGridDungeonCellType::Unknown) {
                                            GridBuilder->AddCorridorPadding(validX + paddingOffset, validY + dy, stairZ);
                                        }
                                    }
                                }
                                GridModel->BuildCellLookup();
                                //gridModel.BuildSpatialCellLookup();
                                GridBuilder->GenerateAdjacencyLookup();
                            }
                            else {
                                continue;
                            }


                            float validZ = stairOwner->Bounds.Location.Z;
                            FVector StairLocation(validX, validY, validZ);
                            StairLocation += FVector(0.5f, 0.5f, 0);
                            StairLocation *= GridToMeshScale;

                            FQuat StairRotation = FQuat::Identity;
                            StairRotation = FQuat(FVector(0, 0, 1), cellOwnerOnLeft ? PI : 0);

                            if (!GridModel->CellStairsLookup.Contains(stairOwner->Id)) {
                                GridModel->CellStairsLookup.Add(stairOwner->Id, TArray<FGridDungeonStairInfo>());
                            }
                            FGridDungeonStairInfo Stair;
                            Stair.OwnerCell = stairOwner->Id;
                            Stair.ConnectedToCell = stairConnectedTo->Id;
                            Stair.Position = StairLocation;
                            Stair.Rotation = StairRotation;
                            Stair.IPosition = FIntVector(validX, validY, validZ);
                            GridModel->CellStairsLookup[stairOwner->Id].Add(Stair);
                            GridModel->Stairs.Add(Stair);
                        }
                    }
                }
            }

            // Move to the next adjacent nodes
            FGridDungeonCell* cellB = GridModel->GetCell(top.CellIdB);
            if (!cellB) continue;
            for (int32 adjacentCell : cellB->AdjacentCells) {
                int32 hash1 = HASH(cellB->Id, adjacentCell);
                int32 hash2 = HASH(adjacentCell, cellB->Id);
                if (visited.Contains(hash1) || visited.Contains(hash2)) continue;
                StairEdgeInfo Edge(top.CellIdB, adjacentCell);
                Stack.Push(Edge);
            }
        }
    }
}

