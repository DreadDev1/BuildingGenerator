//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/FlowExecGraphPanelNodeFactory.h"

#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodeBase.h"
#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodes.h"
#include "Frameworks/Flow/ExecGraph/Widgets/SGraphNode_FlowExecNode.h"

TSharedPtr<class SGraphNode> FFlowExecGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const {
    if (UFlowExecEdGraphNodeBase* ExecNode = Cast<UFlowExecEdGraphNodeBase>(Node)) {
        TSharedPtr<SGraphNode_FlowExecNode> SNode = SNew(SGraphNode_FlowExecNode, ExecNode);

        if (ExecNode->IsA<UFlowExecEdGraphNode_Result>()) {
            SNode->SetBorderColor(FLinearColor(0.08f, 0.16f, 0.08f));
        }
        return SNode;
    }
    return nullptr;
}

