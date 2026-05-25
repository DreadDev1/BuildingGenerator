//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshGeneratorGrid.h"

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowUtils.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshLib.h"

void FCellFlowMeshGenGrid::Generate(UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph,
                                    const FCellFlowLevelMeshGenSettings& InSettings,
                                    FFlowVisLib::FGeometry& OutSurfaceGeometry,
                                    FFlowVisLib::FGeometry& OutLineGeometry) {
	using namespace UE::Geometry;

	const FVector3f GridSize = FVector3f(InSettings.VisualizationScale);

	bool bRenderInactiveGroups = false;
#if WITH_EDITORONLY_DATA
	bRenderInactiveGroups = InCellGraph->bRenderInactiveGroups;
#endif // WITH_EDITORONLY_DATA

	TMap<FGuid, const UFlowAbstractNode*> LayoutNodes;
	for (const UFlowAbstractNode* GraphNode : InLayoutGraph->GraphNodes) {
		const UFlowAbstractNode*& NodeRef = LayoutNodes.FindOrAdd(GraphNode->NodeId);
		NodeRef = GraphNode;
	}

	int GlobalCliffDepth = FCellFlowLevelMeshLib::GetGlobalCliffDepth(InLayoutGraph, InCellGraph, LayoutNodes);

	// Build the tile quads
	{
		const FVector4f InActiveColor = FVector4f{0.25f, 0.25f, 0.25f, 1.0f};
		TArray<FFlowVisLib::FQuadInfo> RenderQuads;
		for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
			if (!GroupNode.IsActive()) continue;
			if (GroupNode.LeafNodes.Num() == 0) continue;

			FVector4f BlockedTileColor = FVector4f(0.1f);
			FVector4f Color = GroupNode.GroupColor;
			const UFlowAbstractNode** LayoutNodePtr = LayoutNodes.Find(GroupNode.LayoutNodeID);
			const UFlowAbstractNode* LayoutNode = LayoutNodePtr ? *LayoutNodePtr : nullptr;
			if (!LayoutNode || !LayoutNode->bActive) {
				Color = InActiveColor;
			}

			bool bGroupActive = LayoutNode && LayoutNode->bActive;
			if (!bRenderInactiveGroups && !bGroupActive) {
				continue;
			}

			for (const int LeafNodeId : GroupNode.LeafNodes) {
				if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(InCellGraph->LeafNodes[LeafNodeId])) {
					for (const FIntPoint& LeafTileCoord : GridLeafNode->LeafTileCoords) {
						const FVector2f NodeSize(1, 1);
						FFlowVisLib::FQuadInfo& Quad = RenderQuads.AddDefaulted_GetRef();
						Quad.Location = FVector3f(LeafTileCoord.X, LeafTileCoord.Y, GridLeafNode->LogicalZ) * GridSize;
						Quad.Size = NodeSize * FVector2f(GridSize.X, GridSize.Y);
						Quad.UVScale = NodeSize;
						Quad.Color = Color;
					}
				}
			}
		}
		FFlowVisLib::EmitQuad(RenderQuads, OutSurfaceGeometry);
	}

	TArray<FCellFlowGridEdgeInfo> PreviewHalfEdges;
	if (InCellGraph->GridInfo.HalfEdges.Num() == 0) {
		FFlowAbstractGraphQuery LayoutGraphQuery(InLayoutGraph);
		DA::FCellGraphBuilder::GenerateEdgeList(InCellGraph, PreviewHalfEdges, LayoutGraphQuery, true);
	}
	const TArray<FCellFlowGridEdgeInfo>& HalfEdges = (InCellGraph->GridInfo.HalfEdges.Num() == 0)
														 ? PreviewHalfEdges
														 : InCellGraph->GridInfo.HalfEdges;

	TArray<FFlowVisLib::FLineInfo> Lines;
	auto DrawEdge = [&](const FCellFlowGridEdgeInfo& Edge, const FVector4f& InColor) {
		const FCellFlowGridEdgeInfo& EdgeTwin = HalfEdges[Edge.TwinIndex];
		const FIntPoint TileCoord{Edge.Coord};
		const FIntPoint TileCoordTwin{EdgeTwin.Coord};

		FIntPoint EdgeCoordSrc{};
		FIntPoint EdgeCoordDst{};
		FCellFlowUtils::GetEdgeEndPoints(Edge, EdgeTwin, EdgeCoordSrc, EdgeCoordDst);

		if (TileCoord.X == TileCoordTwin.X) {
			// Horizontal edge
			EdgeCoordSrc.Y = FMath::Max(TileCoord.Y, TileCoordTwin.Y);
			EdgeCoordSrc.X = TileCoord.X;
			EdgeCoordDst = EdgeCoordSrc + FIntPoint(1, 0);
		}
		else {
			// Vertical edge
			EdgeCoordSrc.X = FMath::Max(TileCoord.X, TileCoordTwin.X);
			EdgeCoordSrc.Y = TileCoord.Y;
			EdgeCoordDst = EdgeCoordSrc + FIntPoint(0, 1);
		}

		constexpr float LineZOffset = 5;
		auto RegisterLine = [&Lines, &GridSize, &InColor](const FIntPoint& EdgeCoordSrc, int32 SrcZ, const FIntPoint& EdgeCoordDst, int32 DstZ) {
			FFlowVisLib::FLineInfo& Line = Lines.AddDefaulted_GetRef();
			Line.Start = FVector3f(EdgeCoordSrc.X, EdgeCoordSrc.Y, SrcZ) * GridSize + FVector3f(0, 0, LineZOffset);
			Line.End = FVector3f(EdgeCoordDst.X, EdgeCoordDst.Y, DstZ) * GridSize + FVector3f(0, 0, LineZOffset);
			Line.Color = InColor;
		};

		int32 HeightLo = Edge.HeightZ;
		RegisterLine(EdgeCoordSrc, HeightLo, EdgeCoordDst, HeightLo);

		/*
		if (InCellGraph->GroupNodes.IsValidIndex(Edge.TileGroup)) {
			int32 GroupHeight = InCellGraph->GroupNodes[Edge.TileGroup].GroupHeight;
			int32 HeightHi = Edge.HeightZ + GroupHeight;
			
			RegisterLine(EdgeCoordSrc, HeightLo, EdgeCoordSrc, HeightHi);
			RegisterLine(EdgeCoordDst, HeightLo, EdgeCoordDst, HeightHi);
			RegisterLine(EdgeCoordSrc, HeightHi, EdgeCoordDst, HeightHi);
		}
		*/

		auto GetEdgeHeight = [GlobalCliffDepth](const FCellFlowGridEdgeInfo& InEdge) {
			return (InEdge.TileGroup != INDEX_NONE) ? InEdge.HeightZ : GlobalCliffDepth;
		};
		const int Height0 = GetEdgeHeight(Edge);
		const int Height1 = GetEdgeHeight(EdgeTwin);

		if (InCellGraph->GroupNodes.IsValidIndex(Edge.TileGroup)) {
			if (Height0 > Height1) {
				const int HU = Height0;
				const int HL = Height1;
				const FVector3f U0 = FVector3f(EdgeCoordSrc.X, EdgeCoordSrc.Y, HU) * GridSize;
				const FVector3f U1 = FVector3f(EdgeCoordDst.X, EdgeCoordDst.Y, HU) * GridSize;
				const FVector3f L0 = FVector3f(EdgeCoordSrc.X, EdgeCoordSrc.Y, HL) * GridSize;
				const FVector3f L1 = FVector3f(EdgeCoordDst.X, EdgeCoordDst.Y, HL) * GridSize;

			
				const FVector4f Color = InCellGraph->GroupNodes[Edge.TileGroup].GroupColor;


				const FIntPoint Normal2D = TileCoordTwin - TileCoord;
				const FVector3f Normal(Normal2D.X, Normal2D.Y, 0);
				bool bFlipTriangle;
				if (Normal2D.X == 0) {
					bFlipTriangle = Normal2D.Y > 0;
				}
				else {
					bFlipTriangle = Normal2D.X < 0;
				}

				const int32 I00 = OutSurfaceGeometry.Vertices.Add(
					FFlowVisLib::CreateVertex(L0, FVector2f(0.5f, 0.5f), {0, 0}, Color, Normal));
				const int32 I10 = OutSurfaceGeometry.Vertices.Add(
					FFlowVisLib::CreateVertex(L1, FVector2f(0.5f, 0.5f), {0, 0}, Color, Normal));
				const int32 I11 = OutSurfaceGeometry.Vertices.Add(
					FFlowVisLib::CreateVertex(U1, FVector2f(0.5f, 0.5f), {0, 0}, Color, Normal));
				const int32 I01 = OutSurfaceGeometry.Vertices.Add(
					FFlowVisLib::CreateVertex(U0, FVector2f(0.5f, 0.5f), {0, 0}, Color, Normal));


				if (!bFlipTriangle) {
					OutSurfaceGeometry.Triangles.Add(FIndex3i(I00, I11, I10));
					OutSurfaceGeometry.Triangles.Add(FIndex3i(I00, I01, I11));
				}
				else {
					OutSurfaceGeometry.Triangles.Add(FIndex3i(I00, I10, I11));
					OutSurfaceGeometry.Triangles.Add(FIndex3i(I00, I11, I01));
				}
			}
		}

		if (Edge.bContainsStair) {
			if (const FDAFlowCellGraphGridStairInfo* StairInfo = InCellGraph->GridInfo.Stairs.Find(Edge.EdgeIndex)) {
				const FVector4f StairColor{1, 0.25f, 0.25f, 1};
				TQuat<float> StairRotation({0, 0, 1}, StairInfo->AngleRadians);
				FVector3f BaseLoc = FVector3f(StairInfo->LocalLocation) * GridSize;
				FCellFlowLevelMeshLib::RenderStairs(BaseLoc, StairRotation, GridSize, StairColor, OutSurfaceGeometry);
			}
		}
	};

	auto GetLineColor = [](const FCellFlowGridEdgeInfo& Edge) {
		return Edge.bConnection
				   ? FVector4f(1, 0, 0, 1)
				   : FVector4f(0, 0, 0, 1);
	};

	TSet<int32> VisitedEdgeIndices;
	for (int i = 0; i < HalfEdges.Num(); i++) {
		if (VisitedEdgeIndices.Contains(i)) {
			continue;
		}

		const FCellFlowGridEdgeInfo& Edge = HalfEdges[i];
		if (!Edge.bEnabled) {
			continue;
		}
		
		DrawEdge(Edge, GetLineColor(Edge));

		const FCellFlowGridEdgeInfo& EdgeTwin = HalfEdges[Edge.TwinIndex];
		if (EdgeTwin.TileGroup != INDEX_NONE && Edge.HeightZ == EdgeTwin.HeightZ) {
			// No need to draw a duplicate twin edge at the same location
			VisitedEdgeIndices.Add(Edge.TwinIndex);
		}
	}

	constexpr float LineWidth = 10;
	FFlowVisLib::EmitLine(Lines, LineWidth, OutLineGeometry);
}

