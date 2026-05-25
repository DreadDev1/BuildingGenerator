//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraphSchema.h"

#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutConnectionDrawingPolicy.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraphNode.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "FlowAbstractEdGraphSchema"

UFlowLayoutEdGraphSchema::UFlowLayoutEdGraphSchema(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
}

void UFlowLayoutEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const {
    const UFlowLayoutEdGraph* Graph = Cast<UFlowLayoutEdGraph>(ContextMenuBuilder.CurrentGraph);

    // TODO: Add the node actions
}

void UFlowLayoutEdGraphSchema::GetContextMenuActions(class UToolMenu* Menu,
                                                           class UGraphNodeContextMenuContext* Context) const {
}

const FPinConnectionResponse UFlowLayoutEdGraphSchema::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const {
    // Make sure the data types match
    if (A->PinType.PinCategory != B->PinType.PinCategory) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }
    // Make sure they are not the same pins
    if (A->GetOwningNode() == B->GetOwningNode()) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }

    // Disallow connection on same direction
    if (A->Direction == EGPD_Output && B->GetOwningNode()->IsA<UFlowLayoutEdGraphNode>()) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }

    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

FLinearColor UFlowLayoutEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const {
    return FColor::Yellow;
}

bool UFlowLayoutEdGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const {
    return false;
}

FConnectionDrawingPolicy* UFlowLayoutEdGraphSchema::CreateConnectionDrawingPolicy(
    int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
    class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const {
    return new FFlowLayoutConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect,
                                                        InDrawElements, InGraphObj);
}

#if WITH_EDITOR
bool UFlowLayoutEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const {
    UFlowLayoutEdGraphNode* NodeA = Cast<UFlowLayoutEdGraphNode>(A->GetOwningNode());
    UFlowLayoutEdGraphNode* NodeB = Cast<UFlowLayoutEdGraphNode>(B->GetOwningNode());
    UEdGraphPin* OutputA = NodeA->GetOutputPin();
    UEdGraphPin* InputB = NodeB->GetInputPin();
    if (!OutputA || !InputB) {
        return false;
    }

    bool bConnectionMade = UEdGraphSchema::TryCreateConnection(OutputA, InputB);
    if (bConnectionMade && OutputA && InputB) {
        // Allow only one outgoing link
        TArray<UEdGraphPin*> LinkedPins = A->LinkedTo;
        for (UEdGraphPin* LinkedPin : LinkedPins) {
            if (LinkedPin != InputB) {
                // Break this pin
                OutputA->BreakLinkTo(LinkedPin);
            }
        }
        // Break a reverse link, if it exists
        {
            UEdGraphPin* InputA = NodeA->GetInputPin();
            UEdGraphPin* OutputB = NodeB->GetOutputPin();
            if (InputA && OutputB) {
                OutputB->BreakLinkTo(InputA);
            }
        }

        UEdGraph* Graph = A->GetOwningNode()->GetGraph();
        Graph->NotifyGraphChanged();
    }
    return bConnectionMade;
}

void UFlowLayoutEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const {
    UEdGraphSchema::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
    TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
}

void UFlowLayoutEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const {
    UEdGraphSchema::BreakSinglePinLink(SourcePin, TargetPin);
    SourcePin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
}

void UFlowLayoutEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const {
    UEdGraphSchema::BreakNodeLinks(TargetNode);
    TargetNode.GetGraph()->NotifyGraphChanged();
}
#endif // WITH_EDITOR

//////////////////////////////////////// FFlowExecSchemaAction_NewNode ////////////////////////////////////////
UEdGraphNode* FFlowAbstractSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin,
                                                                   const FVector2D Location,
                                                                   bool bSelectNewNode /*= true*/) {
    UEdGraphNode* NewNode = FDungeonSchemaAction_NewNode::PerformAction(ParentGraph, FromPin, Location, bSelectNewNode);

    if (UFlowLayoutEdGraphNode* TaskNode = Cast<UFlowLayoutEdGraphNode>(NewNode)) {
        // TODO: Initialize me
    }

    return NewNode;
}

#undef LOCTEXT_NAMESPACE

