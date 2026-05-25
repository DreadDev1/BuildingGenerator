//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecRuleNode.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/DungeonThemeConnectionDrawingPolicy.h"
#include "Frameworks/GraphGrammar/GraphGrammar.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_GrammarNode"


UEdGraphNode_GrammarExecRuleNode::UEdGraphNode_GrammarExecRuleNode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
#if WITH_EDITOR
    bCanRenameNode = false;
#endif // WITH_EDITOR

}

#if WITH_EDITOR
void UEdGraphNode_GrammarExecRuleNode::AllocateDefaultPins() {
    AllocateDefaultPins_Runtime();
}

FText UEdGraphNode_GrammarExecRuleNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    FText Caption = Rule.IsValid() ? Rule->RuleName : LOCTEXT("DeletedCaption", "[DELETED]");
    return Caption;
}

void UEdGraphNode_GrammarExecRuleNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
    UEdGraphNode_GrammarExecNodeBase::PostEditChangeProperty(PropertyChangedEvent);
}

void UEdGraphNode_GrammarExecRuleNode::AutowireNewNode(UEdGraphPin* FromPin) {
    if (!FromPin) {
        Super::AutowireNewNode(FromPin);
        return;
    }
    UEdGraphNode_GrammarExecRuleNode* OtherNode = Cast<UEdGraphNode_GrammarExecRuleNode>(FromPin->GetOwningNode());
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

void UEdGraphNode_GrammarExecRuleNode::PinConnectionListChanged(UEdGraphPin* Pin) {
    Super::PinConnectionListChanged(Pin);
}

#endif // WITH_EDITOR

void UEdGraphNode_GrammarExecRuleNode::InitializeNode_Runtime() {
    CreateNewGuid_Runtime();
    AllocateDefaultPins_Runtime();
}

void UEdGraphNode_GrammarExecRuleNode::CreateNewGuid_Runtime() {
    NodeGuid = FGuid::NewGuid();
}

void UEdGraphNode_GrammarExecRuleNode::AllocateDefaultPins_Runtime() {
    CreatePin_Runtime(EGPD_Input, TEXT("Transition"), TEXT("In"));
    CreatePin_Runtime(EGPD_Output, TEXT("Transition"), TEXT("Out"));
}

UEdGraphPin* UEdGraphNode_GrammarExecRuleNode::CreatePin_Runtime(EEdGraphPinDirection Dir, const FName& InPinCategory,
                                                          const FName& PinName, int32 InIndex /*= INDEX_NONE*/) {
    UEdGraphPin* NewPin = UEdGraphPin::CreatePin(this);
    NewPin->PinName = PinName;
    NewPin->Direction = Dir;

    FEdGraphPinType PinType;
    PinType.PinCategory = InPinCategory;

    NewPin->PinType = PinType;

    Modify(false);
    if (Pins.IsValidIndex(InIndex)) {
        Pins.Insert(NewPin, InIndex);
    }
    else {
        Pins.Add(NewPin);
    }
    return NewPin;
}

UEdGraphPin* UEdGraphNode_GrammarExecRuleNode::GetInputPin() const {
    return Pins[0];
}

UEdGraphPin* UEdGraphNode_GrammarExecRuleNode::GetOutputPin() const {
    return Pins[1];
}


#undef LOCTEXT_NAMESPACE

