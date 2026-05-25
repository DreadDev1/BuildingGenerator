//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Algorithms/Layout/LinearGraphLayout.h"

#include "Frameworks/Algorithms/Layout/GraphLayoutUtils.h"

#include "EdGraph/EdGraphNode.h"

namespace {
    void ArrangeHorizontal(FLinearLayoutNodePtr Node, int32 DistanceFromStart, TSet<FLinearLayoutNodePtr>& Visited) {
        Node->LocationX = FMath::Max(Node->LocationX, DistanceFromStart);

        // Go forward only if all the incoming nodes have been processed
        bool bAllIncomingNodesProcessed = true;
        for (FLinearLayoutNodePtr IncomingNode : Node->IncomingNodes) {
            if (!Visited.Contains(IncomingNode)) {
                bAllIncomingNodesProcessed = false;
                break;
            }
        }

        if (bAllIncomingNodesProcessed) {
            Visited.Add(Node);
            for (FLinearLayoutNodePtr OutgoingNode : Node->OutgoingNodes) {
                ArrangeHorizontal(OutgoingNode, DistanceFromStart + 1, Visited);
            }
        }
    }

    void ArrangeVertical(TArray<FLinearLayoutNodePtr> Nodes) {
        if (Nodes.Num() == 0) return;

        int32 MinX = MAX_int32;
        int32 MaxX = MIN_int32;
        TMap<int32, TArray<FLinearLayoutNodePtr>> DistanceFromStart;
        for (FLinearLayoutNodePtr Node : Nodes) {
            TArray<FLinearLayoutNodePtr>& List = DistanceFromStart.FindOrAdd(Node->LocationX);
            List.Add(Node);

            MinX = FMath::Min(MinX, Node->LocationX);
            MaxX = FMath::Max(MaxX, Node->LocationX);
        }

        for (int x = MinX; x <= MaxX; x++) {
            if (DistanceFromStart.Contains(x)) {
                int32 Y = 0;
                TArray<FLinearLayoutNodePtr>& NodesAtX = DistanceFromStart[x];
                for (FLinearLayoutNodePtr NodeAtX : NodesAtX) {
                    for (FLinearLayoutNodePtr OutgoingNode : NodeAtX->OutgoingNodes) {
                        if (OutgoingNode->LocationX == x + 1) {
                            OutgoingNode->LocationY = Y;
                            Y++;
                        }
                    }
                }
            }
        }
    }
}


void FLinearGraphLayout::PerformLayout(UEdGraph* Graph) {
    using namespace GraphLayoutUtils;
    TLayoutGraphGraph<FLinearLayoutNode> LinearGraph = GenerateLayoutGraph<FLinearLayoutNode>(Graph);
    TSharedPtr<FLinearLayoutNode> RootNode = FindRootNode(LinearGraph);

    // Arrange the nodes horizontally
    {
        TSet<FLinearLayoutNodePtr> Visited;
        ArrangeHorizontal(RootNode, 0, Visited);
    }

    // Arrange the nodes vertically
    ArrangeVertical(LinearGraph.Nodes);

    TMap<FGuid, UEdGraphNode*> NodeLookup;
    for (TObjectPtr<UEdGraphNode> Node : Graph->Nodes) {
        if (Node) {
            NodeLookup.FindOrAdd(Node->NodeGuid, Node);
        }
    }
    
    // Finalize the arrangement
    for (FLinearLayoutNodePtr Node : LinearGraph.Nodes) {
        const float X = Config.InterDistanceX * Node->LocationX;
        const float Y = Config.InterDistanceY * Node->LocationY;
        
        if (UEdGraphNode* EdNode = NodeLookup.FindRef(Node->NodeId)) {
            EdNode->NodePosX = Config.bHorizontalLayout ? X : Y;
            EdNode->NodePosY = Config.bHorizontalLayout ? Y : X;
        }
    }
}

