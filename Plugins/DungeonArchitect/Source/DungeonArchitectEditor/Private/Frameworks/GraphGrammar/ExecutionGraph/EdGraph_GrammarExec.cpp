//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/GraphGrammar/ExecutionGraph/EdGraph_GrammarExec.h"

#include "Frameworks/GraphGrammar/ExecutionGraph/EdGraphSchema_GrammarExec.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecEntryNode.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecRuleNode.h"
#include "Frameworks/GraphGrammar/GraphGrammar.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrammarExecGraphScript, Log, All);

////////////////////////// UEdGraph_GrammarExec //////////////////////////

UEdGraph_GrammarExec::UEdGraph_GrammarExec(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    Schema = UEdGraphSchema_GrammarExec::StaticClass();

    EntryNode = ObjectInitializer.CreateDefaultSubobject<UEdGraphNode_GrammarExecEntryNode>(this, "EntryNode");
    EntryNode->InitializeNode_Runtime();
    AddNode(EntryNode);
}

UEdGraphNode_GrammarExecRuleNode* UEdGraph_GrammarExec::AddNewNode(TWeakObjectPtr<UGraphGrammarProduction> InRule) {
    UEdGraphNode_GrammarExecRuleNode* Node = NewObject<UEdGraphNode_GrammarExecRuleNode>(this);
    Node->InitializeNode_Runtime();
    Node->Rule = InRule;
    AddNode(Node);
    return Node;
}


void UEdGraph_GrammarExec::NotifyGraphChanged() {
    Super::NotifyGraphChanged();

    UGrammarExecutionScript* Script = Cast<UGrammarExecutionScript>(GetOuter());
    FGrammarExecutionScriptCompiler::Compile(Script);
}


//////////////////////////// FGrammarExecutionScriptCompiler //////////////////////////// 

void FGrammarExecutionScriptCompiler::Compile(UGrammarExecutionScript* Script) {
    if (!Script) {
        return;
    }
    UGrammarScriptGraph* ScriptGraph = NewObject<UGrammarScriptGraph>(Script);

    // Create the nodes
    TMap<UEdGraphNode_GrammarExecNodeBase*, UGrammarScriptGraphNode*> EdToScriptNodes;
    for (UEdGraphNode* EdNode : Script->EdGraph->Nodes) {
        if (UEdGraphNode_GrammarExecEntryNode* EntryEdNode = Cast<UEdGraphNode_GrammarExecEntryNode>(EdNode)) {
            UGrammarExecutionScriptEntryNode* EntryScriptNode = NewObject<UGrammarExecutionScriptEntryNode
            >(ScriptGraph);
            CopyNodeData(EntryScriptNode, EntryEdNode);
            ScriptGraph->Nodes.Add(EntryScriptNode);
            EdToScriptNodes.Add(EntryEdNode, EntryScriptNode);
            Script->EntryNode = EntryScriptNode;
        }
        else if (UEdGraphNode_GrammarExecRuleNode* RuleEdNode = Cast<UEdGraphNode_GrammarExecRuleNode>(EdNode)) {
            UGrammarExecutionScriptRuleNode* RuleScriptNode = NewObject<UGrammarExecutionScriptRuleNode>(ScriptGraph);
            CopyNodeData(RuleScriptNode, RuleEdNode);
            ScriptGraph->Nodes.Add(RuleScriptNode);
            EdToScriptNodes.Add(RuleEdNode, RuleScriptNode);
        }
    }

    // Create the link mapping
    for (UEdGraphNode* EdNode : Script->EdGraph->Nodes) {
        if (UEdGraphNode_GrammarExecNodeBase* RuleEdNode = Cast<UEdGraphNode_GrammarExecNodeBase>(EdNode)) {
            UGrammarScriptGraphNode* ScriptNode = EdToScriptNodes[RuleEdNode];
            for (UEdGraphPin* OutgoingPin : RuleEdNode->GetOutputPin()->LinkedTo) {
                if (UEdGraphNode_GrammarExecNodeBase* OutgoingRuleEdNode = Cast<UEdGraphNode_GrammarExecNodeBase>(
                    OutgoingPin->GetOwningNode())) {
                    // Make an outgoing / incoming link
                    UGrammarScriptGraphNode* OutgoingScriptNode = EdToScriptNodes[OutgoingRuleEdNode];
                    ScriptNode->OutgoingNodes.Add(OutgoingScriptNode);
                    OutgoingScriptNode->IncomingNodes.Add(ScriptNode);
                }
            }
        }
    }

    Script->ScriptGraph = ScriptGraph;
}

void FGrammarExecutionScriptCompiler::CopyNodeData(UGrammarExecutionScriptRuleNode* RuleScriptNode,
                                                   UEdGraphNode_GrammarExecRuleNode* RuleEdNode) {
    RuleScriptNode->Rule = RuleEdNode->Rule;
    RuleScriptNode->ExecutionMode = RuleEdNode->ExecutionMode;
    RuleScriptNode->ExecutionConfig = RuleEdNode->ExecutionConfig;
}

void FGrammarExecutionScriptCompiler::CopyNodeData(UGrammarExecutionScriptEntryNode* EntryScriptNode,
                                                   UEdGraphNode_GrammarExecEntryNode* EntryEdNode) {

}

