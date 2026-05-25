//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/GraphGrammar/ExecutionGraph/GraphPanelNodeFactory_GrammarExec.h"

#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecEntryNode.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecRuleNode.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Widgets/SGraphNode_GrammarExecEntryNode.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Widgets/SGraphNode_GrammarExecRuleNode.h"

TSharedPtr<class SGraphNode> FGraphPanelNodeFactory_GrammarExec::CreateNode(UEdGraphNode* Node) const {
    if (UEdGraphNode_GrammarExecRuleNode* RuleNode = Cast<UEdGraphNode_GrammarExecRuleNode>(Node)) {
        TSharedPtr<SGraphNode_GrammarExecRuleNode> SNode = SNew(SGraphNode_GrammarExecRuleNode, RuleNode);
        return SNode;
    }
    if (UEdGraphNode_GrammarExecEntryNode* EntryNode = Cast<UEdGraphNode_GrammarExecEntryNode>(Node)) {
        TSharedPtr<SGraphNode_GrammarExecEntryNode> SNode = SNew(SGraphNode_GrammarExecEntryNode, EntryNode);
        return SNode;
    }

    return nullptr;
}

