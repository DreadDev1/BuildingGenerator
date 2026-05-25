//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"


FVector2d UDAFlowCellLeafNodeGrid::GetCenter() const {
	// TODO: Optimize me.  this needs to be cached. Right now its called from O(N) code, so it's fine for now
	FVector2d Center = FVector2d::ZeroVector;
	if (LeafTileCoords.Num() > 0) {
		for (const FIntPoint& Coord : LeafTileCoords) {
			Center += FVector2d(Coord) + FVector2d(0.5f, 0.5f);
		}
		Center /= LeafTileCoords.Num();
	}
	return Center;
}

float UDAFlowCellLeafNodeGrid::GetArea() const {
	return LeafTileCoords.Num();
}



