//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTask.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/Lib/FlowAbstractGraphPathUtils.h"
#include "BaseFlowLayoutTaskPathBuilderBase.generated.h"

typedef TSharedPtr<class IFlowAGNodeGroupGenerator> IFlowAGNodeGroupGeneratorPtr;
typedef TSharedPtr<class FFlowAbstractGraphConstraints> FFlowAbstractGraphConstraintsPtr;

class UFlowLayoutNodeCreationConstraint;
struct FFlowAGStaticGrowthState;
struct FFlowAGGrowthState;
struct FFlowAGGrowthState_PathItem;

UENUM()
enum class EFlowPathTagConstraintsType : uint8 {
    AllNodesInPath,     // All nodes in the path should contain the tag
    ExcludeFromPath,    // Don't use nodes that contain this tag in the path
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FFlowPathTagConstraints {
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, Category = "Constraints")
    FName PathName;
    
    UPROPERTY(EditAnywhere, Category = "Constraints")
    EFlowPathTagConstraintsType ConstraintType = EFlowPathTagConstraintsType::AllNodesInPath;

};

UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UBaseFlowLayoutTaskPathBuilderBase : public UBaseFlowLayoutTask {
    GENERATED_BODY()

protected:
    virtual void FinalizePath(const FFlowAGStaticGrowthState& StaticState, FFlowAGGrowthState& GrowthState) const;
    virtual void FinalizePathNode(UFlowAbstractNode* InPathNode, const FFlowAGGrowthState_PathItem& PathItem) const {}
    virtual void ExtendPathNodes(FFlowExecNodeStatePtr InNodeState, const FFlowAGGrowthState& InPathState, const FRandomStream& InRandom, const FFlowAbstractGraphQuery& InGraphQuery) const {}
    virtual IFlowAGNodeGroupGeneratorPtr CreateNodeGroupGenerator(TWeakPtr<const IFlowDomain> InDomain) const;
    virtual FFlowAbstractGraphConstraintsPtr CreateGraphConstraints(TWeakPtr<const IFlowDomain> InDomain) const;
    virtual UFlowLayoutNodeCreationConstraint* GetNodeCreationConstraintLogic(ADungeon* InDungeon) const;
    
    struct FPathGrowthItem {
        FGuid NodeId;
        FGuid PreviousNodeId;
    };
};

