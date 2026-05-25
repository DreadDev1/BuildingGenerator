//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/Attributes.h"
#include "Core/Utils/DungeonUserParameters.h"
#include "Core/Volumes/DungeonNegationVolume.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTaskStructs.h"

#include "Misc/Variant.h"
#include "Templates/SubclassOf.h"

class IFlowDomain;
struct FRandomStream;
class UFlowExecTask;
class UFlowExecScript;
class UFlowExecScriptGraphNode;
struct FFlowProcessorSettings;

struct DUNGEONARCHITECTRUNTIME_API FFlowProcessorSettings {
    FDAAttributeList AttributeList;
    TMap<FString, FString> SerializedAttributeList;
    TWeakObjectPtr<ADungeon> Dungeon;
    FDungeonUserParameterCollection CustomParameters;
    TMap<FName, FVariant> BuilderAttributes;
    TArray<TWeakObjectPtr<AActor>> ContextActors;
};

struct DUNGEONARCHITECTRUNTIME_API FFlowProcessorResult {
    EFlowTaskExecutionResult ExecResult = EFlowTaskExecutionResult::FailHalt;
    EFlowTaskExecutionFailureReason FailReason = EFlowTaskExecutionFailureReason::Unknown;
};

class DUNGEONARCHITECTRUNTIME_API FFlowProcessor {
public:
    FFlowProcessor();
    FFlowProcessorResult Process(UFlowExecScript* ExecScript, const FRandomStream& InRandom, const FFlowProcessorSettings& InSettings);
    bool GetNodeState(const FGuid& NodeId, FFlowExecutionOutput& OutNodeState);
    FFlowExecutionOutput* GetNodeStatePtr(const FGuid& NodeId);
    EFlowTaskExecutionStage GetNodeExecStage(const FGuid& NodeId);
    void RegisterDomain(const TSharedPtr<IFlowDomain>& InDomain);
    
private:
    FFlowProcessorResult ExecuteNode(UFlowExecScriptGraphNode* Node, const FRandomStream& InRandom, const FFlowProcessorSettings& InSettings);
    
    static void SetTaskAttributes(UFlowExecTask* Task, const FFlowProcessorSettings& InSettings);
    TSharedPtr<IFlowDomain> FindDomain(const TSubclassOf<UFlowExecTask>& InTaskClass) const;

private:
    TSet<FGuid> VisitedNodes;
    TMap<FGuid, FFlowExecutionOutput> NodeStates;
    TMap<FGuid, EFlowTaskExecutionStage> NodeExecStages;
    TArray<TSharedPtr<IFlowDomain>> RegisteredDomains;
};

class DUNGEONARCHITECTRUNTIME_API IFlowProcessDomainExtender {
public:
    virtual ~IFlowProcessDomainExtender() {}
    virtual void ExtendDomains(FFlowProcessor& InProcessor) = 0;
};

