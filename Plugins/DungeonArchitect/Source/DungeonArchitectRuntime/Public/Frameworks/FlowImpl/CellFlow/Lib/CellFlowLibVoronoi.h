//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonShapes.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "Frameworks/Lib/Geometry/DCEL.h"

class UCellFlowLayoutGraph;

class FCellFlowLibVoronoi {
public:
	struct FStairGenInfo {
		FDAFlowCellGraphDCELEdgeInfo StairInfo;
		FTransform MarkerTransform;
		FTransform OcclusionTransform;
		FVector OcclusionBoxExtents;
		FDAShapePolygon OcclusionShape;
	};
	
	struct FEmitMarkersContext {
		const UDAFlowCellGraph* CellGraph;
		FVector GridSize;
		FRandomStream* Random;
		TMap<FGuid, const UFlowAbstractNode*> LayoutNodes;
		TMap<int, int> LeafToGroupMap;
		TMap<int32, TSet<const DA::DCEL::FEdge*>> GroupBoundaryEdges;
		TMap<int32, TArray<FDAShapePolygon>> GroupFaceShapes;
		TMap<int32, FCellFlowVoronoiMarkerSetup> GroupNodeChunkMarkers;
		TMap<int32, FStairGenInfo> StairGenInfoByEdge;
	};

	enum class ECellFlowEdgeType : uint8 {
		Unknown,
		BoundaryEdge,		// This is on the border of the dungeon. The other side of this edge (twin) is empty
		SharedEdgeSameHeight, // Both sides have ground.   This edge is shared between ground tiles of same height
		SharedEdgeHigh,		// Both sides have ground.   This edge is on the higher ground
		SharedEdgeLow		// Both sides have ground.   This edge is on the lower ground
	};

	static bool IsGroupBorderEdge(const FDAFlowCellGroupNode& GroupNode, const DA::DCEL::FEdge* InEdge);
	static bool IsDungeonBorderEdge(const FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* InEdge);
	static ECellFlowEdgeType GetEdgeType(const FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* InEdge);
	static const UFlowAbstractNode* GetLayoutNodeFromFaceId(const FEmitMarkersContext& EmitMarkerContext, int FaceId);
	static FTransform GetEdgeMarkerTransform(const DA::DCEL::FEdge* InEdge, const FVector& GridSize, const UDAFlowCellLeafNode* LeafNode, float StartOffset, float MarkerScale, const FCellFlowSizedMarkerDef& MarkerDef, bool bCenterOnOcclusionBounds);

	typedef TFunction<void(const FCellFlowSizedMarkerDef& /*MarkerDef*/, const FTransform& /*MarkerTransform*/, const FTransform& /*OclusionTransform*/)> TFuncEmitMarker;
	typedef TFunction<void(const FString&, const FTransform&)> TFuncEmitVoxelMarker;
	static void EmitEdgeMarkers(FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* Edge, const TFuncEmitMarker& EmitMarker, const TFuncEmitVoxelMarker& EmitVoxelMarker, const FString& VoxelMarkerName, float StartT = 0, float EndT = 1);
	static void EmitDoorEdgeMarkers(FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* Edge, const TFuncEmitMarker& EmitMarker, const TFuncEmitVoxelMarker& EmitVoxelMarker, const FString& VoxelMarkerName);

	static void InitMarkerContext(const UDAFlowCellGraph* InCellGraph, const DA::DCELGraph& InDCELGraph, UCellFlowLayoutGraph* InLayoutGraph,
			const UCellFlowConfigMarkerSettings* InMarkerSettings, const FVector& InGridSize, FRandomStream& InRandom,
			FEmitMarkersContext& OutEmitMarkerContext);
};

