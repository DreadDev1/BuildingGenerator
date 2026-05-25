//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "FlowExecGraphScript.generated.h"

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowExecScriptGraphNode : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid NodeId;

    UPROPERTY()
    TArray<TObjectPtr<UFlowExecScriptGraphNode>> OutgoingNodes;

    UPROPERTY()
    TArray<TObjectPtr<UFlowExecScriptGraphNode>> IncomingNodes;

public:
    virtual EFlowExecTaskInputConstraint GetInputConstraint() const;
    virtual TArray<int32> SelectIncomingNodeBranches(const FRandomStream& InRandom) const;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowExecScriptTaskNode : public UFlowExecScriptGraphNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UFlowExecTask> Task;

public:
    virtual EFlowExecTaskInputConstraint GetInputConstraint() const override;
    virtual TArray<int32> SelectIncomingNodeBranches(const FRandomStream& InRandom) const override;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowExecScriptResultNode : public UFlowExecScriptGraphNode {
    GENERATED_BODY()
public:

};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowExecScriptGraph : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UFlowExecScriptGraphNode>> Nodes;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowExecScript : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UFlowExecScriptGraph> ScriptGraph;

    UPROPERTY()
    TObjectPtr<UFlowExecScriptResultNode> ResultNode;
};

