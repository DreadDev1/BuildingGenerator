//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Algorithms/Layout/LayeredGraphLayout.h"

#include "Frameworks/Algorithms/Layout/GraphLayoutUtils.h"
#include "Frameworks/Algorithms/Layout/LayeredLayoutLib.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

void FLayeredGraphLayout::PerformLayout(UEdGraph* Graph) {
    using namespace GraphLayoutUtils;
    TLayoutGraphGraph<FLayeredLayoutNode> LayeredGraph = GenerateLayoutGraph<FLayeredLayoutNode>(Graph);
    TSharedPtr<FLayeredLayoutNode> RootNode = FindRootNode(LayeredGraph);
    
    if (!RootNode.IsValid()) {
        return;
    }

    FDALayeredLayoutLib::PerformLayout(RootNode);

    TMap<FGuid, UEdGraphNode*> NodeLookup;
    for (TObjectPtr<UEdGraphNode> Node : Graph->Nodes) {
        if (Node) {
            NodeLookup.FindOrAdd(Node->NodeGuid, Node);
        }
    }
    
    // Finalize the arrangement
    for (FLayeredLayoutNodePtr LayeredNode : LayeredGraph.Nodes) {
        const float X = Config.DepthDistance * LayeredNode->Depth;
        const float Y = Config.SiblingDistance * LayeredNode->X;

        if (UEdGraphNode* Node = NodeLookup.FindRef(LayeredNode->NodeId)) {
            Node->NodePosX = Config.bHorizontalLayout ? X : Y;
            Node->NodePosY = Config.bHorizontalLayout ? Y : X;
        }
    }
}

