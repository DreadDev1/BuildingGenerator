//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph_GrammarExec.generated.h"

class UEdGraphNode_GrammarExecEntryNode;
class UEdGraphNode_GrammarExecRuleNode;
class UGraphGrammarProduction;
class UGrammarExecutionScript;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraph_GrammarExec : public UEdGraph {
    GENERATED_UCLASS_BODY()

public:
    UEdGraphNode_GrammarExecRuleNode* AddNewNode(TWeakObjectPtr<UGraphGrammarProduction> InRule);

    //// Begin UEdGraph Interface
    virtual void NotifyGraphChanged() override;
    //// End UEdGraph Interface

public:
    UPROPERTY()
    TObjectPtr<UEdGraphNode_GrammarExecEntryNode> EntryNode;
};

class UGrammarExecutionScriptRuleNode;
class UGrammarExecutionScriptEntryNode;

class DUNGEONARCHITECTEDITOR_API FGrammarExecutionScriptCompiler {
public:
    static void Compile(UGrammarExecutionScript* Script);

private:
    static void CopyNodeData(UGrammarExecutionScriptRuleNode* RuleScriptNode, UEdGraphNode_GrammarExecRuleNode* RuleEdNode);
    static void CopyNodeData(UGrammarExecutionScriptEntryNode* EntryScriptNode,
                             UEdGraphNode_GrammarExecEntryNode* EntryEdNode);
};

