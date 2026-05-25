//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/FlowExecEdGraphSchema.h"

#include "Core/Common/Utils/DungeonGraphUtils.h"
#include "Frameworks/Flow/Domains/FlowDomain.h"
#include "Frameworks/Flow/Domains/Voxel/DefaultFlowVoxelDomain.h"
#include "Frameworks/Flow/ExecGraph/FlowExecConnectionDrawingPolicy.h"
#include "Frameworks/Flow/ExecGraph/FlowExecEdGraph.h"
#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodes.h"
#include "Frameworks/Flow/ExecGraph/Tasks/CommonFlowGraphDomain.h"
#include "Frameworks/FlowImpl/GridFlow/Tilemap/Tasks/GridFlowTilemapTaskInitialize.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "GridFlowExecEdGraphSchema"

UFlowExecEdGraphSchema::UFlowExecEdGraphSchema(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    DomainFilter = MakeShareable(new FFlowExecSchemaDomainFilter);
}

void UFlowExecEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const {
    const UFlowExecEdGraph* Graph = Cast<UFlowExecEdGraph>(ContextMenuBuilder.CurrentGraph);

    TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
    GetActionList(Actions, Graph, ContextMenuBuilder.OwnerOfTemporaries);

    for (TSharedPtr<FEdGraphSchemaAction> Action : Actions) {
        ContextMenuBuilder.AddAction(Action);
    }
}

namespace {
    template <typename TTask>
    void AddTaskContextAction(const FText& Category, const FText& Title, const FText& Tooltip, int32 Priority,
                              TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, UEdGraph* OwnerOfTemporaries) {
        AddTaskContextAction(TTask::StaticClass(), Category, Title, Tooltip, Priority, OutActions, OwnerOfTemporaries);
    }

    void AddTaskContextAction(UClass* Class, const FText& Category, const FText& Title, const FText& Tooltip, int32 Priority, TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, UEdGraph* OwnerOfTemporaries) {
        const TSharedPtr<FFlowExecSchemaAction_NewNode> NewActorNodeAction = MakeShared<FFlowExecSchemaAction_NewNode>(Category, Title, Tooltip, 0);
        OutActions.Add(NewActorNodeAction);
        
        UFlowExecEdGraphNode_Task* TaskNodeTemplate = NewObject<UFlowExecEdGraphNode_Task>(OwnerOfTemporaries);
        TaskNodeTemplate->TaskTemplate = NewObject<UFlowExecTask>(TaskNodeTemplate, Class);
        NewActorNodeAction->NodeTemplate = TaskNodeTemplate;
    }
}

void UFlowExecEdGraphSchema::GetActionList(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions,
                                               const UEdGraph* Graph, UEdGraph* OwnerOfTemporaries) const {
    struct FTaskClassInfo {
        UClass* Class;
        FText Title;
        FText Tooltip;
        FText Category;
        int32 Priority;
    };
    TArray<FTaskClassInfo> TaskClasses;
    if (DomainFilter.IsValid()) {
        TArray<IFlowDomainPtr> ActiveDomains;
        for (IFlowDomainWeakPtr DomainPtr : DomainFilter->GetAllowedDomains()) {
            IFlowDomainPtr Domain = DomainPtr.Pin();
            if (Domain.IsValid()) {
                ActiveDomains.Add(Domain);
            }
        }
        
        //ActiveDomains.Add(MakeShared<FDefaultFlowVoxelDomain>());
        ActiveDomains.Add(MakeShared<FCommonFlowGraphDomain>());
        
        for (const IFlowDomainPtr& Domain : ActiveDomains) {
            int32 TaskCounter = 1;
            TArray<UClass*> DomainClasses;
            Domain->GetDomainTasks(DomainClasses);
            const FText DomainCategory = Domain->GetDomainDisplayName();
            for (UClass* Class : DomainClasses) {
                FTaskClassInfo ClassInfo;
                ClassInfo.Class = Class;
                ClassInfo.Category = DomainCategory;
                FString TitleText = FString::Printf(TEXT("%d.  %s"), TaskCounter++, *Class->GetMetaData("Title"));
                ClassInfo.Title = FText::FromString(TitleText);
                ClassInfo.Tooltip = FText::FromString(Class->GetMetaData("Tooltip"));
                ClassInfo.Priority = Class->GetIntMetaData("MenuPriority");
                TaskClasses.Add(ClassInfo);
            }
        }
    }
    
    
    for (const FTaskClassInfo& ClassInfo : TaskClasses) {
        AddTaskContextAction(ClassInfo.Class, ClassInfo.Category, ClassInfo.Title, ClassInfo.Tooltip, ClassInfo.Priority, OutActions, OwnerOfTemporaries);
    }
}

void UFlowExecEdGraphSchema::SetAllowedDomains(const TArray<IFlowDomainWeakPtr>& InAllowedDomains) const {
    if (DomainFilter.IsValid()) {
        DomainFilter->SetAllowedDomains(InAllowedDomains);
    }
}

TArray<IFlowDomainWeakPtr> UFlowExecEdGraphSchema::GetAllowedDomains() const { return DomainFilter->GetAllowedDomains(); }

const FPinConnectionResponse UFlowExecEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const {
    // Make sure the data types match
    if (A->PinType.PinCategory != B->PinType.PinCategory) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }
    // Make sure they are not the same pins
    if (A->GetOwningNode() == B->GetOwningNode()) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }

    // Constrain result node direction
    if (A->GetOwningNode()->IsA<UFlowExecEdGraphNode_Result>()) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }

    // Check the input constraint
    {
        if (const UFlowExecEdGraphNode_Task* TaskNodeB = Cast<UFlowExecEdGraphNode_Task>(B->GetOwningNode())) {
            if (TaskNodeB->TaskTemplate) {
                const EFlowExecTaskInputConstraint InputConstraint = TaskNodeB->TaskTemplate->GetInputConstraint();
                if (InputConstraint == EFlowExecTaskInputConstraint::NoInput) {
                    return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
                }
            }
        }
    }
    
    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

FLinearColor UFlowExecEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const {
    return FColor::Yellow;
}

bool UFlowExecEdGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const {
    return false;
}

UEdGraphNode* UFlowExecEdGraphSchema::CreateSubstituteNode(UEdGraphNode* Node, const UEdGraph* Graph,
            FObjectInstancingGraph* InstanceGraph, TSet<FName>& InOutExtraNames) const {
    if (UFlowExecEdGraphNode_Task* TaskNode = Cast<UFlowExecEdGraphNode_Task>(Node)) {
        if (DomainFilter.IsValid()) {
            for (const IFlowDomainWeakPtr& DomainPtr : DomainFilter->GetAllowedDomains()) {
                IFlowDomainPtr Domain = DomainPtr.Pin();
                if (Domain.IsValid()) {
                    UFlowExecTask* CompatibleTemplate =  Domain->TryCreateCompatibleTask(TaskNode->TaskTemplate);
                    if (CompatibleTemplate) {
                        TaskNode->TaskTemplate = CompatibleTemplate;
                        return TaskNode;
                    }
                }
            } 
        }
    }
    
    return nullptr;
}

FConnectionDrawingPolicy* UFlowExecEdGraphSchema::CreateConnectionDrawingPolicy(
    int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
    class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const {
    return new FFlowExecConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect,
                                                    InDrawElements, InGraphObj);
}

#if WITH_EDITOR
bool UFlowExecEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const {
    const UFlowExecEdGraphNodeBase* NodeA = Cast<UFlowExecEdGraphNodeBase>(A->GetOwningNode());
    const UFlowExecEdGraphNodeBase* NodeB = Cast<UFlowExecEdGraphNodeBase>(B->GetOwningNode());
    UEdGraphPin* OutputA = NodeA->GetOutputPin();
    UEdGraphPin* InputB = NodeB->GetInputPin();
    if (!OutputA || !InputB) {
        return false;
    }

    /*
    bool bSourceIsTilemapCreateNode = false;
    {
        if (const UFlowExecEdGraphNode_Task* TaskNodeA = Cast<UFlowExecEdGraphNode_Task>(NodeA)) {
            if (TaskNodeA->TaskTemplate) {
                bSourceIsTilemapCreateNode = TaskNodeA->TaskTemplate && TaskNodeA->TaskTemplate->IsA<UGridFlowTilemapTaskInitialize>();
            }
        }
    }
    */
    
    const bool bConnectionMade = UEdGraphSchema::TryCreateConnection(OutputA, InputB);
    if (bConnectionMade) {
        EFlowExecTaskInputConstraint InputConstraint = EFlowExecTaskInputConstraint::MultiInput;
        if (const UFlowExecEdGraphNode_Task* TaskNodeB = Cast<UFlowExecEdGraphNode_Task>(NodeB)) {
            if (TaskNodeB->TaskTemplate) {
                InputConstraint = TaskNodeB->TaskTemplate->GetInputConstraint();
            }
        }

        bool bGraphChanged{};
        if (InputConstraint == EFlowExecTaskInputConstraint::SingleInput) {
            // Allow only one incoming link
            TArray<UEdGraphPin*> LinkedPins = InputB->LinkedTo;
            for (UEdGraphPin* LinkedPin : LinkedPins) {
                if (LinkedPin != OutputA) {
                    // Break this pin
                    InputB->BreakLinkTo(LinkedPin);
                    bGraphChanged = true;
                }
            }
        }

        // Break a reverse link, if it exists
        {
            UEdGraphPin* InputA = NodeA->GetInputPin();
            UEdGraphPin* OutputB = NodeB->GetOutputPin();
            if (InputA && OutputB) {
                OutputB->BreakLinkTo(InputA);
                bGraphChanged = true;
            }
        }

        if (bGraphChanged) {
            UEdGraph* Graph = A->GetOwningNode()->GetGraph();
            Graph->NotifyGraphChanged();
        }
    }
    return bConnectionMade;
}

void UFlowExecEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const {
    UEdGraphSchema::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
    TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
}

void UFlowExecEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const {
    UEdGraphSchema::BreakSinglePinLink(SourcePin, TargetPin);
    SourcePin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
}

void UFlowExecEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const {
    UEdGraphSchema::BreakNodeLinks(TargetNode);
    TargetNode.GetGraph()->NotifyGraphChanged();
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE

