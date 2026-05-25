//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/GridDungeonBuilderLib.h"

#include "Builders/Grid/GridDungeonModel.h"
#include "Core/Markers/DungeonMarkerNames.h"
#include "Core/Utils/DungeonModelHelper.h"

TArray<GridDungeonBuilderImpl::FClusterEdge> GridDungeonBuilderImpl::FClusterUnitRect::GetEdges() const {
	FIntPoint TopLeft = BaseLocation;
	FIntPoint BottomRight = BaseLocation + FIntPoint(1, 1);
            
	return {
		FClusterEdge(TopLeft, FIntPoint(BottomRight.X, TopLeft.Y)),
		FClusterEdge(FIntPoint(BottomRight.X, TopLeft.Y), BottomRight),
		FClusterEdge(BottomRight, FIntPoint(TopLeft.X, BottomRight.Y)),
		FClusterEdge(FIntPoint(TopLeft.X, BottomRight.Y), TopLeft)
	};
}

void GridDungeonBuilderImpl::OffsetTransformZ(const float Z, FTransform& OutTransform) {
	FVector Location = OutTransform.GetLocation();
	Location.Z += Z;
	OutTransform.SetLocation(Location);
}

FString GridDungeonBuilderImpl::GetStairMarkerName(int32 StairHeight) {
	if (StairHeight == 1) {
		return FGridBuilderMarkers::STAIR;
	}

	return FString::Printf(TEXT("%s%dX"), *FGridBuilderMarkers::STAIR, StairHeight);
}

TArray<FIntPoint> GridDungeonBuilderImpl::GenerateBoundaryForConnectedRects(const TArray<FRectangle>& InRects) {
	if (InRects.Num() == 0) {
		return {};
	}
        
	TArray<FClusterUnitRect> UnitRects;
	for (const FRectangle& Rect : InRects) {
		for (int dx = 0; dx < Rect.Size.X; dx++) {
			for (int dy = 0; dy < Rect.Size.Y; dy++) {
				UnitRects.Add({FIntPoint(Rect.Location.X + dx, Rect.Location.Y + dy) });
			}
		}
	} 

	TMap<FClusterEdge, int32> EdgeCount;
	for (const FClusterUnitRect& UnitRect : UnitRects) {
		for (const FClusterEdge& Edge : UnitRect.GetEdges()) {
			int32& Count = EdgeCount.FindOrAdd(Edge);
			Count++;
		}
	}
        
	TArray<FClusterEdge> BoundaryEdges;
	for (const auto& Pair : EdgeCount) {
		if (Pair.Value == 1) { // Keep only boundary edges
			BoundaryEdges.Add(Pair.Key);
		}
	}
        
	// construct the polyline.  This assuming the boundary forms a continuous path 
	TArray<FClusterVertex> BoundaryPolyline;
	if (BoundaryEdges.Num() > 0) {
		BoundaryPolyline.Add(BoundaryEdges[0].Start); // Add the start vertex of the first edge
		FClusterVertex CurrentVertex = BoundaryEdges[0].End;
		TSet<int32> Visited;
		Visited.Add(0);

		for (int32 i = 1; i < BoundaryEdges.Num(); ++i) {
			for (int32 j = 1; j < BoundaryEdges.Num(); ++j) {
				if (Visited.Contains(j)) continue;
                    
				if (BoundaryEdges[j].Start == CurrentVertex) {
					BoundaryPolyline.Add(CurrentVertex);
					Visited.Add(j);
					CurrentVertex = BoundaryEdges[j].End;
					break;
				}
				else if (BoundaryEdges[j].End == CurrentVertex) {
					BoundaryPolyline.Add(CurrentVertex);
					Visited.Add(j);
					CurrentVertex = BoundaryEdges[j].Start;
					break;
				}
			}
		}
	}

	TArray<FIntPoint> BoundaryPolyPoints;
	for (const FClusterVertex& BoundaryVertex : BoundaryPolyline) {
		BoundaryPolyPoints.Add(FIntPoint(BoundaryVertex.Location.X, BoundaryVertex.Location.Y));
	}
	return BoundaryPolyPoints;
}

float GridDungeonBuilderImpl::GetDistance(const FGridDungeonCell& A, const FGridDungeonCell& B) {
	FVector pointA = UDungeonModelHelper::MakeVector(A.Center());
	FVector pointB = UDungeonModelHelper::MakeVector(B.Center());
	return (pointA - pointB).Size();
}

bool GridDungeonBuilderImpl::IsRoomCorridor(EGridDungeonCellType type0, EGridDungeonCellType type1) {
	int32 rooms = 0, corridors = 0;
	rooms += (type0 == EGridDungeonCellType::Room) ? 1 : 0;
	rooms += (type1 == EGridDungeonCellType::Room) ? 1 : 0;

	corridors += (type0 == EGridDungeonCellType::Corridor || type0 == EGridDungeonCellType::CorridorPadding) ? 1 : 0;
	corridors += (type1 == EGridDungeonCellType::Corridor || type1 == EGridDungeonCellType::CorridorPadding) ? 1 : 0;
	return (rooms == 1 && corridors == 1);
}

void GridDungeonBuilderImpl::GetCellCorners(const FRectangle& InBounds, TSet<FIntVector>& OutCorners) {
	const int W = InBounds.Width() - 1;
	const int H = InBounds.Height() - 1;
	const int X = InBounds.X();
	const int Y = InBounds.Y();

	OutCorners.Add(FIntVector(X, Y, 0));
	OutCorners.Add(FIntVector(X + W, Y, 0));
	OutCorners.Add(FIntVector(X + W, Y + H, 0));
	OutCorners.Add(FIntVector(X, Y + H, 0));
}


