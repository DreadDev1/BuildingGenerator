//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraphNode.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"

#define LOCTEXT_NAMESPACE "UGridFlowAbstractEdGraphNodeBase"

UFlowLayoutEdGraphNode::UFlowLayoutEdGraphNode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bCanRenameNode = false;
}

UEdGraphPin* UFlowLayoutEdGraphNode::GetPin(const FGuid& PinId) const {
    for (UEdGraphPin* Pin : Pins) {
        if (Pin->PinId == PinId) {
            return Pin;
        }
    }
    return nullptr;
}

UEdGraphPin* UFlowLayoutEdGraphNode::GetInputPin() const {
    return Pins[0];
}

UEdGraphPin* UFlowLayoutEdGraphNode::GetOutputPin() const {
    return Pins[1];
}

void UFlowLayoutEdGraphNode::InitializeNode() {
    CreateNewGuid();
    AllocateDefaultPins();
}

#if WITH_EDITOR
void UFlowLayoutEdGraphNode::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    UEdGraphNode::PostEditChangeProperty(e);

    GetGraph()->NotifyGraphChanged();
}

void UFlowLayoutEdGraphNode::NodeConnectionListChanged() {
    UEdGraphNode::NodeConnectionListChanged();
}

void UFlowLayoutEdGraphNode::AllocateDefaultPins() {
    CreatePin(EGPD_Input, TEXT("Transition"), TEXT("In"));
    CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
}

void UFlowLayoutEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin) {
    if (!FromPin) {
        Super::AutowireNewNode(FromPin);
        return;
    }
    UFlowLayoutEdGraphNode* OtherNode = Cast<UFlowLayoutEdGraphNode>(FromPin->GetOwningNode());
    if (OtherNode) {
        if (OtherNode->GetOutputPin() == FromPin) {
            UEdGraphPin* OutputPin = FromPin;
            UEdGraphPin* InputPin = GetInputPin();
            const UEdGraphSchema* Schema = GetGraph()->GetSchema();
            const FPinConnectionResponse ConnectionValid = Schema->CanCreateConnection(OutputPin, InputPin);
            if (ConnectionValid.Response == CONNECT_RESPONSE_MAKE) {
                OutputPin->MakeLinkTo(InputPin);
            }
        }
    }
}

UEdGraphPin* UFlowLayoutEdGraphNode::CreatePin(EEdGraphPinDirection Dir, const FName& InPinCategory,
                                                         const FName& PinName, int32 Index /*= INDEX_NONE*/) {
    UEdGraphPin* NewPin = UEdGraphPin::CreatePin(this);
    NewPin->PinName = PinName;
    NewPin->Direction = Dir;

    FEdGraphPinType PinType;
    PinType.PinCategory = InPinCategory;

    NewPin->PinType = PinType;

    Modify(false);
    if (Pins.IsValidIndex(Index)) {
        Pins.Insert(NewPin, Index);
    }
    else {
        Pins.Add(NewPin);
    }
    return NewPin;
}

void UFlowLayoutEdGraphNode::PinConnectionListChanged(UEdGraphPin* Pin) {
    Super::PinConnectionListChanged(Pin);
}

#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE

