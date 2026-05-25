//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Utils/DungeonThemeGraphLayout.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Frameworks/Algorithms/Layout/GraphLayoutUtils.h"
#include "Frameworks/Algorithms/Layout/LayeredLayoutLib.h"

#include "EdGraph/EdGraph.h"
#include "GraphEditor.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "DungeonThemeGraphLayout"

const FDungeonThemeGraphLayout::FSettings FDungeonThemeGraphLayout::DefaultSettings = {};

void FDungeonThemeGraphLayout::PerformLayout(UEdGraph* InThemeGraph, TSharedPtr<SGraphEditor> InGraphEditor, const FSettings& InSettings) {
	if (!InGraphEditor.IsValid() || !InThemeGraph) {
		return;
	}

	using namespace GraphLayoutUtils;
	const FScopedTransaction Transaction(LOCTEXT("GraphLayout", "Layout Graph"));
		
	TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
	InThemeGraph->GetNodesOfClass<UEdGraphNode_DungeonThemeMarker>(MarkerNodes);

	struct FNodeInfo {
		TObjectPtr<UEdGraphNode> Node{};
		FSlateRect Bounds;
	};

	
	TMap<FGuid, FNodeInfo> NodeLookup;
	for (TObjectPtr<UEdGraphNode> EdNode : InThemeGraph->Nodes) {
		if (EdNode) {
			FNodeInfo NodeInfo;
			NodeInfo.Node = EdNode;
			if (!InGraphEditor->GetBoundsForNode(EdNode, NodeInfo.Bounds, 0)) {
				FVector2D Location(EdNode->NodePosX, EdNode->NodePosY);
				FVector2D Size = FVector2D::Zero();
				if (EdNode->IsA<UEdGraphNode_DungeonThemeMarker>() || EdNode->IsA<UEdGraphNode_DungeonThemeMarkerEmitter>()) {
					Size = FVector2D(120, 75);
				}
				else {
					Size = FVector2D(152, 152);
				}
				NodeInfo.Bounds = FSlateRect(Location, Location + Size);
			}
			
			NodeInfo.Bounds = NodeInfo.Bounds.ExtendBy(FMargin(InSettings.NodePadding.X, InSettings.NodePadding.Y));
			NodeLookup.FindOrAdd(EdNode->NodeGuid, NodeInfo);
		}
	}

	struct FMarkerCluster {
		TLayoutGraphGraph<FLayeredLayoutNode> LayeredGraph;
		TObjectPtr<UEdGraphNode_DungeonThemeMarker> MarkerNode;
		FVector2D BaseLocation = FVector2D::Zero();
		TMap<FGuid, FVector2D> DesiredLocalNodePosition;
		FSlateRect OriginalBounds;
		FSlateRect Bounds;
		void CalculateBounds(const TMap<FGuid, FNodeInfo>& InNodeLookup, int32 InClusterPadding) {
			if (LayeredGraph.Nodes.Num() == 0) {
				return;
			}

			Bounds = InNodeLookup.FindRef(LayeredGraph.Nodes[0]->NodeId).Bounds;
			for (int i = 0; i < LayeredGraph.Nodes.Num(); i++) {
				TSharedPtr<FLayeredLayoutNode> LayoutNode = LayeredGraph.Nodes[i];
				FVector2D Location = DesiredLocalNodePosition.FindRef(LayoutNode->NodeId);
				FVector2D NodeSize = InNodeLookup.FindRef(LayoutNode->NodeId).Bounds.GetSize();
				FSlateRect DesiredBounds(Location, Location + NodeSize);
				if (i == 0) {
					Bounds = DesiredBounds;
				}
				else {
					Bounds = Bounds.Expand(DesiredBounds);
				}
			}
				
			Bounds = Bounds.ExtendBy(FMargin(InClusterPadding));
			OriginalBounds = Bounds;
		}
	};
	TMap<FGuid, FMarkerCluster> MarkerClusters;
	for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerNodes) {
		if (!MarkerNode) {
			continue;
		}

		FMarkerCluster& MarkerCluster = MarkerClusters.FindOrAdd(MarkerNode->NodeGuid);
		MarkerCluster.MarkerNode = MarkerNode;
		MarkerCluster.LayeredGraph = GenerateLayoutGraph<FLayeredLayoutNode>(InThemeGraph, MarkerNode);

		// Sort the child nodes based on the ed graph node positions
		for (const TSharedPtr<FLayeredLayoutNode>& LayoutNode : MarkerCluster.LayeredGraph.Nodes) {
			if (LayoutNode.IsValid()) {
				LayoutNode->OutgoingNodes.Sort([&NodeLookup](const FLayeredLayoutNodePtr& A, const FLayeredLayoutNodePtr& B) {
					FNodeInfo NodeInfoA = NodeLookup.FindRef(A->NodeId);
					FNodeInfo NodeInfoB = NodeLookup.FindRef(B->NodeId);
					return NodeInfoA.Bounds.Left > NodeInfoB.Bounds.Left;
				});
			}
		}

		TLayoutGraphGraphQuery<FLayeredLayoutNode> LayeredGraphQuery(MarkerCluster.LayeredGraph);
		TSharedPtr<FLayeredLayoutNode> RootNode = LayeredGraphQuery.GetNode(MarkerNode->NodeGuid);
			
		FDALayeredLayoutLib::PerformLayout(RootNode);

		float NodeWidth = 0;
		for (const TSharedPtr<FLayeredLayoutNode>& LayeredNode : MarkerCluster.LayeredGraph.Nodes) {
			if (LayeredNode.IsValid()) {
				FNodeInfo NodeInfo = NodeLookup.FindRef(LayeredNode->NodeId);
				if (NodeInfo.Node && NodeInfo.Node->IsA<UEdGraphNode_DungeonThemeActorBase>()) {
					NodeWidth = FMath::Max(NodeWidth, NodeInfo.Bounds.GetSize().X);
				}
			}
		}
		if (FMath::IsNearlyZero(NodeWidth)) {
			NodeWidth = 152;
		}

		// Find the desired size of the nodes on each depth level
		TMap<int32, FVector2D> DepthNodeSizes;
		for (const TSharedPtr<FLayeredLayoutNode>& LayeredNode : MarkerCluster.LayeredGraph.Nodes) {
			FVector2D& DepthNodeSize = DepthNodeSizes.FindOrAdd(LayeredNode->Depth);
			FNodeInfo& NodeInfo = NodeLookup.FindOrAdd(LayeredNode->NodeId);
				
			DepthNodeSize.X = FMath::Max(DepthNodeSize.X, NodeInfo.Bounds.GetSize().X);
			DepthNodeSize.Y = FMath::Max(DepthNodeSize.Y, NodeInfo.Bounds.GetSize().Y);   
		}
			
		TMap<int32, float> AccumulatedDepthHeights;
		{
			int32 MaxDepth = 0;
			for (auto& Entry : DepthNodeSizes) {
				MaxDepth = FMath::Max(MaxDepth, Entry.Key);
			}
			MaxDepth = FMath::Min(MaxDepth, 100);

			int32 CurrentDepth = 0;
			for (int i = 0; i <= MaxDepth; i++) {
				AccumulatedDepthHeights.Add(i, CurrentDepth);
				CurrentDepth += DepthNodeSizes.FindOrAdd(i).Y;
			}
			AccumulatedDepthHeights.Add(MaxDepth + 1, CurrentDepth);
		}
			
		auto GetLocalPointLocation = [&](const FLayeredLayoutNodePtr& InNode) {
			float DepthNodeHeightTop = AccumulatedDepthHeights.FindRef(InNode->Depth);
			float DepthNodeHeightBottom = AccumulatedDepthHeights.FindRef(InNode->Depth + 1);
			float DepthNodeHeight = (DepthNodeHeightTop + DepthNodeHeightBottom) * 0.5f;
				
			return FVector2D(NodeWidth * InNode->X, -DepthNodeHeight);
		};
			
		const FSlateRect MarkerBounds = NodeLookup.FindRef(MarkerNode->NodeGuid).Bounds;
		const FVector2D MarkerCenter = MarkerBounds.GetCenter();
		const FVector2D WorldOffset = GetLocalPointLocation(RootNode) + MarkerCenter + InSettings.NodePadding;

		for (FLayeredLayoutNodePtr LayeredNode : MarkerCluster.LayeredGraph.Nodes) {
			FNodeInfo& NodeInfo = NodeLookup.FindOrAdd(LayeredNode->NodeId);
			FVector2D NodeSize = NodeInfo.Bounds.GetSize();
			FVector2D DesiredCenterPoint = WorldOffset - GetLocalPointLocation(LayeredNode);
			FVector2D NodeLocation = DesiredCenterPoint - NodeSize * 0.5f;

			FVector2D& DesiredLocRef = MarkerCluster.DesiredLocalNodePosition.FindOrAdd(LayeredNode->NodeId);
			DesiredLocRef = NodeLocation;
		}
	}

	// Calculate the cluster bounds
	for (auto& Entry : MarkerClusters) {
		FMarkerCluster& MarkerCluster = Entry.Value;
		MarkerCluster.CalculateBounds(NodeLookup, InSettings.ClusterPadding);
	}

	auto ArrangeMarkerClusters = [](TMap<FGuid, FMarkerCluster>& MarkerClusters, float DesiredCanvasWidth) {
		TArray<TPair<FGuid, FMarkerCluster>> SortedClusters;
		for (const auto& Pair : MarkerClusters) {
			SortedClusters.Add(TPair<FGuid, FMarkerCluster>(Pair.Key, Pair.Value));
		}

		SortedClusters.Sort([](const TPair<FGuid, FMarkerCluster>& A, const TPair<FGuid, FMarkerCluster>& B) {
			return A.Value.OriginalBounds.GetArea() > B.Value.OriginalBounds.GetArea();
		});

		struct FLevel {
			FVector2D::FReal Y;
			FVector2D::FReal Height;
			FVector2D::FReal CurrentX;
		};
		TArray<FLevel> Levels;

		for (auto& Pair : SortedClusters) {
			FMarkerCluster& Cluster = MarkerClusters[Pair.Key];
			FVector2D Size = Cluster.Bounds.GetSize();
		        
			bool Placed = false;
		        
			for (FLevel& Level : Levels) {
				if (Level.CurrentX + Size.X <= DesiredCanvasWidth) {
					Cluster.Bounds = FSlateRect(
						Level.CurrentX,
						-Level.Y,
						Level.CurrentX + Size.X,
						-(Level.Y + Size.Y)
					);
		                
					Level.CurrentX += Size.X;
					Placed = true;
					break;
				}
			}
		        
			if (!Placed) {
				float NewY = Levels.Num() > 0 ? Levels.Last().Y - Levels.Last().Height : 0;
		            
				Levels.Add(FLevel{
					NewY,
					Size.Y,
					Size.X
				});
		            
				Cluster.Bounds = FSlateRect(
					0,
					-NewY,
					Size.X,
					-(NewY + Size.Y)
				);
			}
		}
	};
	
	ArrangeMarkerClusters(MarkerClusters, 2000);
	for (auto& Entry : MarkerClusters) {
		FMarkerCluster& Cluster = Entry.Value;
		for (TSharedPtr<FLayeredLayoutNode> LayeredNode : Cluster.LayeredGraph.Nodes) {
			FNodeInfo NodeInfo = NodeLookup.FindRef(LayeredNode->NodeId);
			FVector2D DesiredLocalPos = Cluster.DesiredLocalNodePosition.FindRef(LayeredNode->NodeId);
			if (NodeInfo.Node) {
				FVector2D NodeLoc = Cluster.Bounds.GetTopLeft() - Cluster.OriginalBounds.GetTopLeft() + DesiredLocalPos;
				NodeInfo.Node->NodePosX = FMath::RoundToInt(NodeLoc.X);
				NodeInfo.Node->NodePosY = FMath::RoundToInt(NodeLoc.Y);
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE

