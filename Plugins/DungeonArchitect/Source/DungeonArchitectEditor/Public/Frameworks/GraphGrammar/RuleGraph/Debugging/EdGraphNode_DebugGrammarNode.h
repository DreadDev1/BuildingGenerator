//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/GraphGrammar/RuleGraph/Nodes/EdGraphNode_GrammarNode.h"
#include "EdGraphNode_DebugGrammarNode.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DebugGrammarNode : public UEdGraphNode_GrammarNode {
    GENERATED_UCLASS_BODY()
public:
    void ResetState();
    void ResetModuleState();
public:
    bool bProcessed = false;
    bool bModuleAssigned = false;
    TSoftObjectPtr<UWorld> ModuleLevel;
    FBox ModuleBounds;
    FTransform WorldTransform;
    FString StatusMessage;
    int32 IncomingDoorIndex;
    int32 RemoteDoorIndex;
    FGuid IncomingDoorId;
    FGuid RemoteDoorId;
    FGuid RemoteNodeId;
    FBox IncomingDoorBounds;
};


UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DebugGrammarDoorNode : public UEdGraphNode {
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UEdGraphNode_DebugGrammarNode> Incoming;

    UPROPERTY()
    TObjectPtr<UEdGraphNode_DebugGrammarNode> Outgoing;
};

