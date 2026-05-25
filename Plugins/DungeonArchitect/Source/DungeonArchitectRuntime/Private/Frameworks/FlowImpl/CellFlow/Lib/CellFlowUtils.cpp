//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowUtils.h"

#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"

void FCellFlowUtils::GetEdgeEndPoints(const FCellFlowGridEdgeInfo& InEdge, const FCellFlowGridEdgeInfo& InEdgeTwin,
                                          FIntPoint& OutEdgeCoordSrc, FIntPoint& OutEdgeCoordDst) {
	const FIntPoint TileCoord{InEdge.Coord};
	const FIntPoint TileCoordTwin{InEdgeTwin.Coord};

	if (TileCoord.X == TileCoordTwin.X) {
		// Horizontal edge
		auto GenCoord = [TileCoord, TileCoordTwin](FIntPoint& Src, FIntPoint& Dst) {
			Src.Y = FMath::Max(TileCoord.Y, TileCoordTwin.Y);
			Src.X = TileCoord.X;
			Dst = Src + FIntPoint(1, 0);
		};
		
		if (TileCoord.Y < TileCoordTwin.Y) {
			GenCoord(OutEdgeCoordSrc, OutEdgeCoordDst);
		}
		else {
			GenCoord(OutEdgeCoordDst, OutEdgeCoordSrc);
		}
	}
	else {
		// Vertical edge
		auto GenCoord = [TileCoord, TileCoordTwin](FIntPoint& Src, FIntPoint& Dst) {
			Src.X = FMath::Max(TileCoord.X, TileCoordTwin.X);
			Src.Y = TileCoord.Y;
			Dst = Src + FIntPoint(0, 1);
		};
		
		if (TileCoord.X > TileCoordTwin.X) {
			GenCoord(OutEdgeCoordSrc, OutEdgeCoordDst);
		}
		else {
			GenCoord(OutEdgeCoordDst, OutEdgeCoordSrc);
		}
	}
}

