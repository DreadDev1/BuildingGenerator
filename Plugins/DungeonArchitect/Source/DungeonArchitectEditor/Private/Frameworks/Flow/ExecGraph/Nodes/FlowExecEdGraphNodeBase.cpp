//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"

#define LOCTEXT_NAMESPACE "UGridFlowExecEdGraphNodeBase"

UFlowExecEdGraphNodeBase::UFlowExecEdGraphNodeBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bCanRenameNode = false;
}

UEdGraphPin* UFlowExecEdGraphNodeBase::GetPin(const FGuid& PinId) const {
    for (UEdGraphPin* Pin : Pins) {
        if (Pin->PinId == PinId) {
            return Pin;
        }
    }
    return nullptr;
}

UEdGraphPin* UFlowExecEdGraphNodeBase::GetInputPin() const {
    return Pins.Num() > 0 ? Pins[0] : nullptr;
}

UEdGraphPin* UFlowExecEdGraphNodeBase::GetOutputPin() const {
    return Pins.Num() > 1 ? Pins[1] : nullptr;
}

void UFlowExecEdGraphNodeBase::InitializeNode() {
    CreateNewGuid();
    AllocateDefaultPins();
}

#if WITH_EDITOR
void UFlowExecEdGraphNodeBase::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    UEdGraphNode::PostEditChangeProperty(e);

    GetGraph()->NotifyGraphChanged();
}

void UFlowExecEdGraphNodeBase::NodeConnectionListChanged() {
    UEdGraphNode::NodeConnectionListChanged();
}

void UFlowExecEdGraphNodeBase::AllocateDefaultPins() {
    CreatePin(EGPD_Input, TEXT("Transition"), TEXT("In"));
    CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
}

FText UFlowExecEdGraphNodeBase::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    return LOCTEXT("DebugNodeCaption", "Exec Node");
}

void UFlowExecEdGraphNodeBase::AutowireNewNode(UEdGraphPin* FromPin) {
    if (!FromPin) {
        Super::AutowireNewNode(FromPin);
        return;
    }
    UFlowExecEdGraphNodeBase* OtherNode = Cast<UFlowExecEdGraphNodeBase>(FromPin->GetOwningNode());
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

UEdGraphPin* UFlowExecEdGraphNodeBase::CreatePin(EEdGraphPinDirection Dir, const FName& InPinCategory,
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

void UFlowExecEdGraphNodeBase::PinConnectionListChanged(UEdGraphPin* Pin) {
    Super::PinConnectionListChanged(Pin);
}

#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE

