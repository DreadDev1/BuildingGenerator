//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutGraphPanelNodeFactory.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Impl/GridFlow/GridFlowLayoutEdGraphNode.h"
#include "Frameworks/Flow/Domains/LayoutGraph/SGraphNode_FlowLayoutNode.h"

TSharedPtr<class SGraphNode> FFlowLayoutGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const {
    if (UGridFlowLayoutEdGraphNode* AbstractNode = Cast<UGridFlowLayoutEdGraphNode>(Node)) {
        TSharedPtr<SGraphNode_FlowLayoutNode> SNode = SNew(SGraphNode_FlowLayoutNode, AbstractNode);
        return SNode;
    }

    return nullptr;
}

