//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodeBase.h"
#include "FlowExecEdGraphNodes.generated.h"

class UFlowExecTask;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UFlowExecEdGraphNode_Task : public UFlowExecEdGraphNodeBase {
    GENERATED_BODY()

public:
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
    
public:
    UPROPERTY()
    TObjectPtr<UFlowExecTask> TaskTemplate;
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UFlowExecEdGraphNode_Result : public UFlowExecEdGraphNodeBase {
    GENERATED_BODY()

public:
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual bool CanDuplicateNode() const override { return false; }
};

