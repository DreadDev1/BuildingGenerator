//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"

UEdGraphNode_GrammarExecNodeBase::UEdGraphNode_GrammarExecNodeBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {

}

#if WITH_EDITOR
void UEdGraphNode_GrammarExecNodeBase::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    UEdGraphNode::PostEditChangeProperty(e);

    GetGraph()->NotifyGraphChanged();
}

void UEdGraphNode_GrammarExecNodeBase::NodeConnectionListChanged() {
    UEdGraphNode::NodeConnectionListChanged();
}
#endif //WITH_EDITOR

UEdGraphPin* UEdGraphNode_GrammarExecNodeBase::GetPin(const FGuid& PinId) const {
    for (UEdGraphPin* Pin : Pins) {
        if (Pin->PinId == PinId) {
            return Pin;
        }
    }
    return nullptr;
}

