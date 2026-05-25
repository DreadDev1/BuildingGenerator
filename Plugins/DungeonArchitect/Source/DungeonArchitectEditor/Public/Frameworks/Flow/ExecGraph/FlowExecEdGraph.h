//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "FlowExecEdGraph.generated.h"

class UFlowExecScript;
class UFlowExecTask;
class UFlowExecEdGraphNode_Task;
class UFlowExecEdGraphNode_Result;
class UFlowExecScriptGraph;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UFlowExecEdGraph : public UEdGraph {
    GENERATED_UCLASS_BODY()
public:
    UFlowExecEdGraphNode_Task* AddNewNode(UFlowExecTask* InTask);

    //// Begin UEdGraph Interface
    virtual void NotifyGraphChanged() override;
    //// End UEdGraph Interface

public:
    UPROPERTY()
    TObjectPtr<UFlowExecEdGraphNode_Result> ResultNode;
};


class DUNGEONARCHITECTEDITOR_API FFlowExecScriptCompiler {
public:
    static void Compile(UEdGraph* EdGraph, UFlowExecScript* Script);
};

