//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/Rectangle.h"

struct FGridDungeonCell;
class ADungeonNegationVolume;
enum class EGridDungeonCellType : uint8;

namespace GridDungeonBuilderImpl {
    struct FClusterVertex {
        FIntPoint Location;
        FClusterVertex(const FIntPoint& InLocation) : Location(InLocation) {}
        
        bool operator<(const FClusterVertex& Other) const {
            return Location.X < Other.Location.X || (Location.X == Other.Location.X && Location.Y < Other.Location.Y);
        }
        bool operator==(const FClusterVertex& Other) const {
            return Location.X == Other.Location.X && Location.Y == Other.Location.Y;
        }
    };
    struct FClusterEdge {
        FClusterVertex Start;
        FClusterVertex End;

        FClusterEdge(const FClusterVertex& InStart, const FClusterVertex& InEnd)
            : Start(InStart < InEnd ? InStart : InEnd)
            , End(InStart < InEnd ? InEnd : InStart)
        {
        }
        FClusterEdge(const FIntPoint& InStart, const FIntPoint& InEnd)
            : FClusterEdge(FClusterVertex(InStart), FClusterVertex(InEnd))
        {
            
        }

        bool operator<(const FClusterEdge& Other) const {
            return Start < Other.Start || (Start == Other.Start && End < Other.End);
        }
        bool operator==(const FClusterEdge& Other) const {
            return Start == Other.Start && End == Other.End;
        }
        
    };
    struct StairAdjacencyQueueNode {
        StairAdjacencyQueueNode() : cellId(INDEX_NONE), depth(0) {}
        StairAdjacencyQueueNode(int32 pCellId, int32 pDepth) : cellId(pCellId), depth(pDepth) {
        }

        int32 cellId;
        int32 depth;
    };
    
    FORCEINLINE uint32 GetTypeHash(const FClusterEdge& Edge) {
        return HashCombine(
            GetTypeHash(Edge.Start.Location),
            GetTypeHash(Edge.End.Location));
    }

    struct FClusterUnitRect {       // Rect from X,Y to (X+1, Y+1)
        FIntPoint BaseLocation;
        FClusterUnitRect(const FIntPoint& InLocation)
            : BaseLocation(InLocation)
        {
        }

        TArray<FClusterEdge> GetEdges() const;
    };

    struct StairConnectionWeight {
        StairConnectionWeight(int32 InPosition, int32 InWeight) : position(InPosition), weight(InWeight) {
        }

        int32 position;
        int32 weight;

        bool operator<(const StairConnectionWeight& other) const {
            return weight > other.weight;
        }
    };

    inline TMap<int32, FColor> g_DebugGroupColors;

    void OffsetTransformZ(const float Z, FTransform& OutTransform);

    FString GetStairMarkerName(int32 StairHeight);

    TArray<FIntPoint> GenerateBoundaryForConnectedRects(const TArray<FRectangle>& InRects);;
    
    struct NegationVolumeInfo {
        FRectangle Bounds;
        ADungeonNegationVolume* Volume;
    };

    struct Edge {
        Edge(int32 pCellA, int32 pCellB, float pWeight) : cellA(pCellA), cellB(pCellB), weight(pWeight) {
        }

        int32 cellA;
        int32 cellB;
        float weight;

        bool operator<(const Edge& other) const {
            return weight < other.weight;
        }
    };

    float GetDistance(const FGridDungeonCell& A, const FGridDungeonCell& B);

    struct FCellHeightNode {
        int32 CellId{};
        int32 Height{};
        bool MarkForIncrease{};
        bool MarkForDecrease{};
    };

    struct CellHeightFrameInfo {
        CellHeightFrameInfo(int32 pCellId, int32 pCurrentHeight) : CellId(pCellId), CurrentHeight(pCurrentHeight) {
        }

        int32 CellId;
        int32 CurrentHeight;
    };

    struct StairEdgeInfo {
        StairEdgeInfo(int32 pCellIdA, int32 pCellIdB) : CellIdA(pCellIdA), CellIdB(pCellIdB) {
        }

        int32 CellIdA;
        int32 CellIdB;
    };

    bool IsRoomCorridor(EGridDungeonCellType type0, EGridDungeonCellType type1);

    FORCEINLINE int ClampToInt(float value) {
        return FMath::FloorToInt(value);
    }

    void GetCellCorners(const FRectangle& InBounds, TSet<FIntVector>& OutCorners);

    FORCEINLINE int32 HASH(int32 a, int32 b) { return (a << 16) + b; };
}

