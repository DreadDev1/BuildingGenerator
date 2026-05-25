//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/Stair/GridDungeonStairGeneratorV2.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/Grid/GridDungeonBuilderLib.h"
#include "Builders/Grid/GridDungeonConfig.h"
#include "Builders/Grid/GridDungeonModel.h"

///////////////////////// Stair Generator V2 /////////////////////////
void UGridDungeonBuilderStairGeneratorV2::Generate(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder) const {
    UGridDungeonConfig* GridConfig = Cast<UGridDungeonConfig>(GetOuter());
    if (!GridModel || !GridBuilder || !GridConfig) {
        return;
    }
    
    GenerateDungeonHeights(GridModel, GridBuilder, false);
    
    ConnectStairs(GridModel, GridBuilder, GridConfig->GridCellSize);
}

namespace DA {
    class FAxisDomain {
    public:
        virtual ~FAxisDomain() = default;
        virtual int32 Get(const FIntPoint& V) const = 0;
        virtual int32 Get(const FIntVector& V) const = 0;
        virtual float Get(const FVector& V) const = 0;
        virtual int32 Get(const FRectangle& Bounds) const = 0;
        
        virtual void Set(FIntPoint& V, int32 Value) const = 0;
        virtual void Set(FIntVector& V, int32 Value) const = 0;
        virtual void Set(FVector& V, float Value) const = 0;
    };

    class FAxisDomainX : public FAxisDomain {
    public:
        virtual int32 Get(const FIntPoint& V) const override { return V.X; }
        virtual int32 Get(const FIntVector& V) const override { return V.X; }
        virtual float Get(const FVector& V) const override { return V.X; }
        virtual int32 Get(const FRectangle& Bounds) const override { return Bounds.X(); }
        virtual void Set(FIntPoint& V, int32 Value) const override { V.X = Value; }
        virtual void Set(FIntVector& V, int32 Value) const override { V.X = Value; }
        virtual void Set(FVector& V, float Value) const override { V.X = Value; }
    };
    
    class FAxisDomainY : public FAxisDomain {
    public:
        virtual int32 Get(const FIntPoint& V) const override { return V.Y; }
        virtual int32 Get(const FIntVector& V) const override { return V.Y; }
        virtual float Get(const FVector& V) const override { return V.Y; }
        virtual int32 Get(const FRectangle& Bounds) const override { return Bounds.Y(); }
        virtual void Set(FIntPoint& V, int32 Value) const override { V.Y = Value; }
        virtual void Set(FIntVector& V, int32 Value) const override { V.Y = Value; }
        virtual void Set(FVector& V, float Value) const override { V.Y = Value; }
    };

    struct FIslandBoundaryEdge {
        int32 OwningCellId = INDEX_NONE;
        int32 RemoteCellId = INDEX_NONE;
        
    };
    
    struct FIslandBoundary {
        TArray<FIslandBoundaryEdge> Edges;
    };

    struct FIsland {
        TArray<int32> CellIds;
        TMap<int32, FIslandBoundary> Boundaries; // Boundaries to other islands [Remote island id -> boundary info]
        TSet<FIntPoint> RasterizedTileCoords;
        int32 Z{};
        bool bHeightClamped{};
    };

    struct FIslandLookup {
        TMap<int32, int32> CellToIslandMap;
    };

    void VisitIslandCellRecursive(const FGridDungeonCell& InCell, FIsland& OutIsland, UGridDungeonModel* GridModel, TSet<int32>& VisitedCells) {
        if(VisitedCells.Contains(InCell.Id) || InCell.CellType == EGridDungeonCellType::Unknown) {
            return;
        }
        check(InCell.Bounds.Location.Z == OutIsland.Z);
                
        VisitedCells.Add(InCell.Id);
        OutIsland.CellIds.Add(InCell.Id);
        {
            FIntPoint BaseLocation(InCell.Bounds.Location.X, InCell.Bounds.Location.Y);
            for (int x = 0; x < InCell.Bounds.Size.X; x++) {
                for (int y = 0; y < InCell.Bounds.Size.Y; y++) {
                    OutIsland.RasterizedTileCoords.Add(BaseLocation + FIntPoint(x, y));
                }
            }
        }

        if (InCell.bHeightClamped) {
            // The cell's height is clamped, We don't want to add other cells here
            OutIsland.bHeightClamped = true;
            return;
        }
        
        if (InCell.CellType == EGridDungeonCellType::Room) {
            // Rooms are single islands
            return;
        }
                
        for (int32 AdjacentCellId : InCell.AdjacentCells) {
            if (!VisitedCells.Contains(AdjacentCellId)) {
                if (const FGridDungeonCell* AdjacentCell = GridModel->GetCell(AdjacentCellId)) {
                    // Cells on an island are on the same height.  Also, rooms are isolated islands
                    const bool bBelongsToIsland = (AdjacentCell->Bounds.Location.Z == OutIsland.Z)
                        && AdjacentCell->CellType != EGridDungeonCellType::Room;
                            
                    if (bBelongsToIsland) {
                        VisitIslandCellRecursive(*AdjacentCell, OutIsland, GridModel, VisitedCells);
                    }
                }
            }
        }
    }
    
    void GenerateIslands(UGridDungeonModel* GridModel, TArray<FIsland>& OutIslands, FIslandLookup& OutLookups) {
        {
            TSet<int32> VisitedCells;
            for (const FGridDungeonCell& Cell : GridModel->Cells) {
                if (VisitedCells.Contains(Cell.Id) || Cell.CellType == EGridDungeonCellType::Unknown) {
                    continue;
                }
                FIsland& Island = OutIslands.AddDefaulted_GetRef();
                Island.Z = Cell.Bounds.Location.Z;
                VisitIslandCellRecursive(Cell, Island, GridModel, VisitedCells);
            }
        }

        OutIslands.Sort([](const FIsland& A, const FIsland& B) {
            return A.Z > B.Z;
        });
        
        // Data structure for fast cell to island mapping
        OutLookups = {};
        TMap<int32, int32>& CellToIslandMap = OutLookups.CellToIslandMap;
        for (int IslandIdx = 0; IslandIdx < OutIslands.Num(); IslandIdx++) {
            const FIsland& Island = OutIslands[IslandIdx];
            for (int32 CellId : Island.CellIds) {
                int32& IslandIdxRef = CellToIslandMap.FindOrAdd(CellId);
                IslandIdxRef = IslandIdx;
            }
        }

        // Create the island connection list
        for (const FGridDungeonCell& Cell : GridModel->Cells) {
            if (int32 *IslandIdxPtr = CellToIslandMap.Find(Cell.Id)) {
                int32 IslandIdx = *IslandIdxPtr;

                for (int32 AdjacentCell : Cell.AdjacentCells) {
                    if (int32 *AdjacentIslandIdxPtr = CellToIslandMap.Find(AdjacentCell)) {
                        int32 AdjacentIslandIdx = *AdjacentIslandIdxPtr;
                        if (IslandIdx == AdjacentIslandIdx) {
                            // Belong to the same island
                            continue;
                        }
                        
                        FIslandBoundary& Boundary = OutIslands[IslandIdx].Boundaries.FindOrAdd(AdjacentIslandIdx);
                        FIslandBoundaryEdge& BoundaryEdge = Boundary.Edges.AddDefaulted_GetRef();
                        BoundaryEdge.OwningCellId = Cell.Id;
                        BoundaryEdge.RemoteCellId = AdjacentCell;
                    }
                }
            }
        }
    }
}

void UGridDungeonBuilderStairGeneratorV2::ConnectStairs(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder, const FVector& GridToMeshScale) const {
    using namespace GridDungeonBuilderImpl;
    using namespace DA;
    if (!GridModel || !GridBuilder || GridModel->Cells.Num() == 0) return;

    constexpr int32 MAX_ITERATIONS = 100;
    int32 NumIterations = 0;
    bool bContinueIteration = true;
    while (bContinueIteration && NumIterations++ < MAX_ITERATIONS) {
        // Generate the islands
        TArray<FIsland> Islands;
        FIslandLookup IslandLookup;
        GenerateIslands(GridModel, Islands, IslandLookup);
        
        TMap<FIntPoint, int> HeightAtCoords;
        for (FIsland IslandEntry : Islands) {
            for (FIntPoint Coord : IslandEntry.RasterizedTileCoords) {
                int& HeightRef = HeightAtCoords.FindOrAdd(Coord);
                HeightRef = IslandEntry.Z;
            }
        }
        
        GridModel->Stairs.Reset();
        GridModel->CellStairsLookup.Reset();

        TArray<FIslandBoundaryEdge> EdgesToFixOnSameHeight;
    
        TSet<FIntPoint> ForceOccupiedCell;
        for (const FIsland& Island : Islands) {
            for (const auto& Entry : Island.Boundaries) {
                int32 AdjacentIslandIdx = Entry.Key;
                const FIsland& AdjacentIsland = Islands[AdjacentIslandIdx];
                if (Island.Z < AdjacentIsland.Z) {
                    const FIslandBoundary& Boundary = Entry.Value;
                    struct FGridDungeonStairCandidate {
                        FGridDungeonStairInfo StairInfo;
                        FIntVector TileCoord;
                        FIntVector RemoteTileCoord;
                        TArray<FIntPoint> CoordsToOccupy;
                    };
                
                    for (const FIslandBoundaryEdge& BoundaryEdge : Boundary.Edges) {
                        FGridDungeonCell* OwningCell = GridModel->GetCell(BoundaryEdge.OwningCellId);
                        FGridDungeonCell* RemoteCell = GridModel->GetCell(BoundaryEdge.RemoteCellId);
                        if (!OwningCell || !RemoteCell) {
                            continue;
                        }
                    
                        TArray<FGridDungeonStairCandidate> BoundaryStairCandidates;
                        TArray<FIslandBoundaryEdge> InvalidStairEdges;

                        auto ProcessIntersection = [&](const FRectangle& Intersection, float InRotationOffset, const FAxisDomain& PrimaryAxis, const FAxisDomain& SecondaryAxis) {
                            auto MakeCoord = [&PrimaryAxis, &SecondaryAxis](int32 PrimaryValue, int32 SecondaryValue) {
                                FIntPoint Coord;
                                PrimaryAxis.Set(Coord, PrimaryValue);
                                SecondaryAxis.Set(Coord, SecondaryValue);
                                return Coord;
                            };
                            auto MakeCoord3 = [&PrimaryAxis, &SecondaryAxis](int32 PrimaryValue, int32 SecondaryValue, int32 Z) {
                                FIntVector Coord;
                                PrimaryAxis.Set(Coord, PrimaryValue);
                                SecondaryAxis.Set(Coord, SecondaryValue);
                                Coord.Z = Z;
                                return Coord;
                            };

                            bool bConnectsToRoom = (OwningCell->CellType == EGridDungeonCellType::Room || RemoteCell->CellType == EGridDungeonCellType::Room);
                            for (int D = 0; D < PrimaryAxis.Get(Intersection.Size); D++) {
                                float RotationOffset = InRotationOffset;
                                const bool bRemoteCellAfter = SecondaryAxis.Get(RemoteCell->Bounds.Location) > SecondaryAxis.Get(OwningCell->Bounds.Location);
                            
                                FIntVector TileCoord = OwningCell->Bounds.Location;
                                PrimaryAxis.Set(TileCoord, PrimaryAxis.Get(Intersection.Location) + D);
                            
                                if (bRemoteCellAfter) {
                                    TileCoord += MakeCoord3(0, SecondaryAxis.Get(OwningCell->Bounds.Size) - 1, 0);
                                }

                                FIntVector RemoteTileCoord = TileCoord;
                                RemoteTileCoord += bRemoteCellAfter ? MakeCoord3(0, 1, 0) : MakeCoord3(0, -1, 0);
                            
                                if (bRemoteCellAfter) {
                                    RotationOffset += PI;
                                }

                                // We can't have a stair here. Check if we can have a path to the cell from another route
                                bool bStairValid = true;
                                TFunction<void(TArray<FIntPoint>&)> FuncGenOccupiedCoordList = [](TArray<FIntPoint>&) {};
                                {

                                    if (FMath::Abs(OwningCell->Bounds.Location.Z - RemoteCell->Bounds.Location.Z) > MaxAllowedStairHeight) {
                                        bStairValid = false;
                                    }
                                    else if (bConnectsToRoom) {
                                        if (bAvoidStairsInsideRooms) {
                                            if (OwningCell->CellType == EGridDungeonCellType::Room) {
                                                // The lower cell is a room, avoid generating a stair here
                                                bStairValid = false;
                                            }
                                        }

                                        if (bStairValid) {
                                            // It's valid only if we have a door between them
                                            const bool hasDoor = GridModel->DoorManager.ContainsDoor(TileCoord.X, TileCoord.Y, RemoteTileCoord.X, RemoteTileCoord.Y);
                                            bStairValid = hasDoor;
                                        }
                                    }

                                    // Discard single cell corridor islands
                                    if (bAvoidSingleCellStairDeadEnds && RemoteCell->CellType != EGridDungeonCellType::Room) {
                                        if (int32* IslandIdxPtr = IslandLookup.CellToIslandMap.Find(RemoteCell->Id)) {
                                            if (Islands.IsValidIndex(*IslandIdxPtr)) {
                                                const FIsland& RemoteIsland = Islands[*IslandIdxPtr];
                                                if (RemoteIsland.RasterizedTileCoords.Num() == 1) {
                                                    // We don't want to go up to a single cell dead end
                                                    bStairValid = false;
                                                }
                                            }
                                        }
                                    }
                                    
                                    
                                    if (bStairValid) {
                                        /*
                                         *  Top floor
                                         *  _ R _
                                         *  A S A
                                         *  B E B
                                         *  
                                         *  S = Staircase,
                                         *  A, B, E = tiles near the stair-case
                                         *  R = Top cell of the stair-case
                                         *  if A exists, B should exist
                                         *  E should always exist, either in this island, or on another island in the same height
                                        */
                                        const FIntPoint TileCoord2 = FIntPoint(TileCoord.X, TileCoord.Y);
                                        const FIntPoint RemoteTileCoord2 = FIntPoint(RemoteTileCoord.X, RemoteTileCoord.Y);
                                        const FIntPoint CoordA0 = TileCoord2 + MakeCoord(-1, 0);
                                        const FIntPoint CoordA1 = TileCoord2 + MakeCoord(1, 0);
                                        
                                        const int32 DY = bRemoteCellAfter ? -1 : 1;
                                        const FIntPoint CoordB0 = TileCoord2 + MakeCoord(-1, DY);
                                        const FIntPoint CoordB1 = TileCoord2 + MakeCoord(1, DY);
                                        const FIntPoint CoordE = TileCoord2 + MakeCoord(0, DY);

                                        const bool A0 = Island.RasterizedTileCoords.Contains(CoordA0);
                                        const bool A1 = Island.RasterizedTileCoords.Contains(CoordA1);
                                        const bool B0 = Island.RasterizedTileCoords.Contains(CoordB0);
                                        const bool B1 = Island.RasterizedTileCoords.Contains(CoordB1);
                                        bool E = Island.RasterizedTileCoords.Contains(CoordE);

                                        FuncGenOccupiedCoordList = [=](TArray<FIntPoint>& OutCoords) {
                                            if (A0) OutCoords.Add(CoordA0);
                                            if (A1) OutCoords.Add(CoordA1);
                                            if (B0) OutCoords.Add(CoordB0);
                                            if (B1) OutCoords.Add(CoordB1);
                                            if (E) OutCoords.Add(CoordE);
                                        };

                                        if (ForceOccupiedCell.Contains(TileCoord2)
                                            || ForceOccupiedCell.Contains(RemoteTileCoord2)
                                            || ForceOccupiedCell.Contains(CoordE))
                                        {
                                            bStairValid = false;
                                        }                                        

                                        if (!E) {
                                            bStairValid = false;
                                        }
                                        
                                        if (A0) {
                                            if (!B0) {
                                                bStairValid = false;
                                            }
                                        }
                                        if (A1) {
                                            if (!B1) {
                                                bStairValid = false;
                                            }
                                        }

                                        if (GridModel->DoorManager.ContainsDoor(CoordA0, TileCoord2)) {
                                            bStairValid = false;
                                        }
                                        if (GridModel->DoorManager.ContainsDoor(CoordA1, TileCoord2)) {
                                            bStairValid = false;
                                        }

                                        /*
                                        if (B0 || B1) {
                                            if (!E) {
                                                bStairValid = false;
                                            }
                                        }

                                        if (bStairValid && !A0 && !A1 && !B0 && !B1 && !E) {
                                            // This is a single tile island, make sure there's a cell at E at Z - 1 (chain of stairs)
                                            if (int* HeigthAtE = HeightAtCoords.Find(TileCoord2 + MakeCoord(0, DY))) {
                                                if (*HeigthAtE != Island.Z - 1) {
                                                    bStairValid = false;
                                                }
                                            }
                                            else {
                                                bStairValid = false;
                                            }
                                        }
                                        */
                                    }
                                }
                            
                                if (bStairValid) {
                                    FGridDungeonStairCandidate& Candidate = BoundaryStairCandidates.AddDefaulted_GetRef();
                                    FGridDungeonStairInfo& Stair = Candidate.StairInfo;
                                    Stair.OwnerCell = BoundaryEdge.OwningCellId;
                                    Stair.ConnectedToCell = BoundaryEdge.RemoteCellId;
                                    Stair.Position = (FVector(TileCoord) + FVector(0.5f, 0.5f, 0)) * GridToMeshScale;
                                    Stair.Rotation = FQuat(FVector::UpVector, RotationOffset);
                                    Stair.IPosition = TileCoord;

                                    Candidate.TileCoord = TileCoord;
                                    Candidate.RemoteTileCoord = RemoteTileCoord;
                                    FuncGenOccupiedCoordList(Candidate.CoordsToOccupy);
                                }
                                else {
                                    InvalidStairEdges.Add(BoundaryEdge);
                                }
                            }
                        };
                    
                        // Find the intersecting line
                        FRectangle Intersection = FRectangle::Intersect(OwningCell->Bounds, RemoteCell->Bounds);
                        if (Intersection.Size.X > 0) {
                            ProcessIntersection(Intersection, PI * 0.5f, FAxisDomainX(), FAxisDomainY());
                        }
                        else if (Intersection.Size.Y > 0) {
                            ProcessIntersection(Intersection, 0, FAxisDomainY(), FAxisDomainX());
                        }

                        if (BoundaryStairCandidates.Num() > 0) {
                            for (const FGridDungeonStairCandidate& Candidate : BoundaryStairCandidates) {
                                const bool bPathExists = GridBuilder->ContainsAdjacencyPath(Candidate.StairInfo.OwnerCell, Candidate.StairInfo.ConnectedToCell, StairConnectionTolerance, true);
                                if (!bPathExists) {
                                    const FGridDungeonStairInfo& BestStair = Candidate.StairInfo;
                                    GridModel->Stairs.Add(BestStair);
                            
                                    TArray<FGridDungeonStairInfo>& OwningStairLookup = GridModel->CellStairsLookup.FindOrAdd(BestStair.OwnerCell);
                                    OwningStairLookup.Add(BestStair);

                                    FIntPoint TileCoord = FIntPoint(Candidate.TileCoord.X, Candidate.TileCoord.Y);
                                    FIntPoint RemoteTileCoord = FIntPoint(Candidate.RemoteTileCoord.X, Candidate.RemoteTileCoord.Y);
                                    ForceOccupiedCell.Add(TileCoord);
                                    ForceOccupiedCell.Add(RemoteTileCoord);
                                    for (FIntPoint CoordToOccupy : Candidate.CoordsToOccupy) {
                                        ForceOccupiedCell.Add(CoordToOccupy);
                                    } 
                                }
                            }
                        }
                        else {
                            check(InvalidStairEdges.Num() > 0);
                            EdgesToFixOnSameHeight.Append(InvalidStairEdges);   // TODO: Adding just one will increase the iterations, try adding half of them randomly
                        }
                    }
                }
            }
        }

        bContinueIteration = false;
        // Check if we need to fix the heights of certain cells
        TSet<FGridDungeonCell*> ProcessedCoordCells;
        for (const FIslandBoundaryEdge& BoundaryEdge : EdgesToFixOnSameHeight) {
            const bool bPathExists = GridBuilder->ContainsAdjacencyPath(BoundaryEdge.OwningCellId, BoundaryEdge.RemoteCellId, StairConnectionTolerance, true);
            if (!bPathExists) {
                FGridDungeonCell* CellA = GridModel->GetCell(BoundaryEdge.OwningCellId);
                FGridDungeonCell* CellB = GridModel->GetCell(BoundaryEdge.RemoteCellId);
                check (CellA && CellB);
                {
                    FGridDungeonCell* CellHigher = CellA->Bounds.Location.Z > CellB->Bounds.Location.Z ? CellA : CellB;
                    FGridDungeonCell* CellLower = CellA->Bounds.Location.Z < CellB->Bounds.Location.Z ? CellA : CellB;
                    if (!CellHigher->bHeightClamped) {
                        if (!ProcessedCoordCells.Contains(CellHigher)) {
                            CellHigher->Bounds.Location.Z--;
                            ProcessedCoordCells.Add(CellHigher);
                            bContinueIteration = true;
                        }
                    }
                    else {
                        if (!CellLower->bHeightClamped) {
                            if (!ProcessedCoordCells.Contains(CellLower)) {
                                CellLower->Bounds.Location.Z = CellHigher->Bounds.Location.Z;
                                ProcessedCoordCells.Add(CellLower);
                                bContinueIteration = true;
                            }
                        }
                    }
                }
            }
        }
    }
}


