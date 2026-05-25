//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"

#include "Core/Utils/MathUtils.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowVoronoi.h"
#include "Frameworks/Lib/Voronoi/Voronoi.h"

const FName UCellFlowVoronoiGraph::StateTypeID = TEXT("CellFlowVoronoiGraphData");

void UCellFlowLayoutTaskCreateCellsVoronoi::GenerateCellsImpl(UDAFlowCellGraph* InCellGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const {
	using namespace DA;
	const FRandomStream& Random = *Input.Random;
	UCellFlowVoronoiGraph* Data = NewObject<UCellFlowVoronoiGraph>();
	OutputState->SetStateObject(UCellFlowVoronoiGraph::StateTypeID, Data);

	for (int SiteIdx = 0; SiteIdx < NumPoints; SiteIdx++) {
		const FVector2d SiteLocation = { Random.FRand() * WorldSize.X, Random.FRand() * WorldSize.Y };
		Data->Sites.Add(SiteLocation);
	}

	auto GenerateVoronoiGraph = [this, Data]() {
		FVoronoiGenerator::FSettings Settings;
		Settings.UnboundedEdgeExtension = FMath::Max(WorldSize.X, WorldSize.Y) * 2;
		FVoronoiGenerator::Generate(Data->VoronoiGraph, Data->Sites, Settings);
    
		Data->DGraph.Generate(Data->VoronoiGraph);
    
		DCELGraphClipper Clipper;
		Clipper.ClipBoundary(Data->DGraph,
			{0, 0},
			FVector2d{
				static_cast<double>(WorldSize.X),
				static_cast<double>(WorldSize.Y)
			});
	};

	GenerateVoronoiGraph();
	for (int i = 0; i < NumRelaxIterations; i++) {
		Data->DGraph.RelaxPoints(Data->Sites);
		GenerateVoronoiGraph();
	}

	int CellIdCounter{-1};
	for (const UDAFlowCellLeafNode* LeafNode : InCellGraph->LeafNodes) {
		if (LeafNode) {
			CellIdCounter = FMath::Max(CellIdCounter, LeafNode->CellId);
		}
	}
	
	TMap<int, int> SiteToCellId;
	for (DCEL::FFace* Face : Data->DGraph.GetFaces()) {
		if (!Face || !Face->bValid || !Face->Outer) continue;

		// Check if the face belongs to an unbounded edge
		if (bDisableBoundaryCells) {
			DCEL::TraverseFaceEdges(Face->Outer, [Face](const DCEL::FEdge* InEdge) {
				if (InEdge && InEdge->bValid && !InEdge->Twin->LeftFace) {
					Face->bValid = false;
				}
			});
		}

		UDAFlowCellLeafNodeVoronoi* Node = InCellGraph->CreateLeafNode<UDAFlowCellLeafNodeVoronoi>();
		Node->CellId = ++CellIdCounter;
		Node->SiteIndex = Face->FaceId;

		const FVector2d SiteLocation = Data->Sites[Face->FaceId];
		Node->CenterX = SiteLocation.X;
		Node->CenterY = SiteLocation.Y;

		auto TriangleArea = [](const FVector2d& A, const FVector2d& B, const FVector2d& C) {
			return 0.5 * (A.X * (B.Y - C.Y) + B.X * (C.Y - A.Y) + C.X * (A.Y - B.Y));
		};
		
		// Calculate the area
		float Area{};
		DCEL::TraverseFaceEdges(Face->Outer, [&](const DCEL::FEdge* InEdge) {
			if (InEdge && InEdge->Twin && InEdge->Origin && InEdge->Twin->Origin) {
				Area += TriangleArea(SiteLocation, InEdge->Origin->Location, InEdge->Twin->Origin->Location);
			}
		});

		Node->Area = Area;

		SiteToCellId.Add(Node->SiteIndex, Node->CellId);
	}

	const float EdgeConnectionThresholdSq = FMath::Square(EdgeConnectionThreshold);
	
	InCellGraph->GroupNodes.Reset();
	for (const DCEL::FFace* Face : Data->DGraph.GetFaces()) {
		const int SiteId = Face->FaceId;
		const int CellId = SiteToCellId[SiteId];
		
		// Create a group node that contains this single leaf node
		FDAFlowCellGroupNode& GroupNode = InCellGraph->GroupNodes.AddDefaulted_GetRef();
		GroupNode.GroupId = CellId;
		GroupNode.GroupColor = FColorUtils::GetRandomColor(Random, 0.3f);
		if (Face->bValid) {
			GroupNode.LeafNodes.Add(CellId);    // each node belongs to its own group initially
			
			// Build the connection list
			DCEL::TraverseFaceEdges(Face->Outer, [&](const DCEL::FEdge* InEdge) {
				if (InEdge && InEdge->Twin && InEdge->Twin->LeftFace && InEdge->Twin->LeftFace->bValid) {
					const int ConnectedSiteIndex = InEdge->Twin->LeftFace->FaceId;
					if (const int* ConnectedCellIdPtr = SiteToCellId.Find(ConnectedSiteIndex)) {
						// Check if the edge is large enough
						const double EdgeLengthSq = DA::DCEL::GetEdgeLengthSq(InEdge);
						if (EdgeLengthSq > EdgeConnectionThresholdSq) {
							const int ConnectedCellId = *ConnectedCellIdPtr;
							GroupNode.Connections.Add(ConnectedCellId);
						}
					}
				}
			});
		}
	}
}

void UCellFlowVoronoiGraph::CloneFromStateObject(const UObject* SourceObject) {
	if (const UCellFlowVoronoiGraph* Other = Cast<UCellFlowVoronoiGraph>(SourceObject)) {
		Sites = Other->Sites;
		VoronoiGraph.CloneFrom(Other->VoronoiGraph);
		DGraph.CloneFrom(Other->DGraph);
	}
}

