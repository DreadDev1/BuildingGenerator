//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/FlowExecEdGraph.h"

#include "Frameworks/Flow/ExecGraph/FlowExecEdGraphSchema.h"
#include "Frameworks/Flow/ExecGraph/FlowExecGraphScript.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodes.h"

#include "EdGraph/EdGraphPin.h"

////////////////////////// UEdGraph_FlowExec //////////////////////////

UFlowExecEdGraph::UFlowExecEdGraph(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    Schema = UFlowExecEdGraphSchema::StaticClass();

    ResultNode = ObjectInitializer.CreateDefaultSubobject<UFlowExecEdGraphNode_Result>(this, "ResultNode");
    ResultNode->InitializeNode();
    AddNode(ResultNode);
}

UFlowExecEdGraphNode_Task* UFlowExecEdGraph::AddNewNode(UFlowExecTask* InTask) {
    UFlowExecEdGraphNode_Task* Node = NewObject<UFlowExecEdGraphNode_Task>(this);
    Node->InitializeNode();
    Node->TaskTemplate = InTask;
    AddNode(Node);
    return Node;
}


void UFlowExecEdGraph::NotifyGraphChanged() {
    Super::NotifyGraphChanged();

    UFlowExecScript* Script = Cast<UFlowExecScript>(GetOuter());
    FFlowExecScriptCompiler::Compile(this, Script);

}

////////////////////////// FGridFlowExecScriptCompiler //////////////////////////

void FFlowExecScriptCompiler::Compile(UEdGraph* EdGraph, UFlowExecScript* Script) {
    if (!EdGraph || !Script) {
        return;
    }

    // Clear out the old state
    Script->ScriptGraph = nullptr;
    Script->ResultNode = nullptr;

    // Create a new script graph that mirrors the ed graph
    UFlowExecScriptGraph* ScriptGraph = NewObject<UFlowExecScriptGraph>(Script);

    // Create the nodes
    TMap<UFlowExecEdGraphNodeBase*, UFlowExecScriptGraphNode*> EdToScriptNodes;
    for (UEdGraphNode* EdNode : EdGraph->Nodes) {
        if (UFlowExecEdGraphNode_Result* ResultEdNode = Cast<UFlowExecEdGraphNode_Result>(EdNode)) {
            UFlowExecScriptResultNode* ResultScriptNode = NewObject<UFlowExecScriptResultNode>(ScriptGraph);
            ResultScriptNode->NodeId = ResultEdNode->NodeGuid;
            ScriptGraph->Nodes.Add(ResultScriptNode);
            EdToScriptNodes.Add(ResultEdNode, ResultScriptNode);
            Script->ResultNode = ResultScriptNode;
        }
        else if (UFlowExecEdGraphNode_Task* TaskEdNode = Cast<UFlowExecEdGraphNode_Task>(EdNode)) {
            UFlowExecScriptTaskNode* TaskScriptNode = NewObject<UFlowExecScriptTaskNode>(ScriptGraph);
            TaskScriptNode->NodeId = TaskEdNode->NodeGuid;
            UFlowExecTask* TaskTemplate = TaskEdNode->TaskTemplate;
            if (TaskTemplate) {
                TaskScriptNode->Task = NewObject<UFlowExecTask>(TaskScriptNode,
                                                                    TaskEdNode->TaskTemplate->GetClass(), NAME_None,
                                                                    RF_NoFlags, TaskEdNode->TaskTemplate);
            }
            ScriptGraph->Nodes.Add(TaskScriptNode);
            EdToScriptNodes.Add(TaskEdNode, TaskScriptNode);
        }
    }

    // Create the link mapping
    for (UEdGraphNode* EdNode : EdGraph->Nodes) {
        if (UFlowExecEdGraphNodeBase* RuleEdNode = Cast<UFlowExecEdGraphNodeBase>(EdNode)) {
            UFlowExecScriptGraphNode* ScriptNode = EdToScriptNodes[RuleEdNode];
            for (UEdGraphPin* OutgoingPin : RuleEdNode->GetOutputPin()->LinkedTo) {
                if (UFlowExecEdGraphNodeBase* OutgoingRuleEdNode = Cast<UFlowExecEdGraphNodeBase>(OutgoingPin->GetOwningNode())) {
                    // Make an outgoing / incoming link
                    UFlowExecScriptGraphNode* OutgoingScriptNode = EdToScriptNodes[OutgoingRuleEdNode];
                    ScriptNode->OutgoingNodes.Add(OutgoingScriptNode);
                    OutgoingScriptNode->IncomingNodes.Add(ScriptNode);
                }
            }
        }
    }
    Script->ScriptGraph = ScriptGraph;
}

