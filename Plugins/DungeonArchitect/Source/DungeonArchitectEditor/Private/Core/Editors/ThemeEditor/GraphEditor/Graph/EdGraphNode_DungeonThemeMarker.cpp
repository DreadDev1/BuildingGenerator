//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/DungeonThemeConnectionDrawingPolicy.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"

#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonMesh"

UEdGraphNode_DungeonThemeMarker::UEdGraphNode_DungeonThemeMarker(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    MarkerName = FString("Marker");
    bUserDefined = true;
    bBuilderEmittedMarker = false;
}

void UEdGraphNode_DungeonThemeMarker::AllocateDefaultPins() {
    // Create the output pin
    CreatePin(EGPD_Output, FDungeonThemePinDataTypes::PinType_Mesh, TEXT("Out"));
}

FLinearColor UEdGraphNode_DungeonThemeMarker::GetNodeTitleColor() const {
    return FLinearColor(0.7f, 0.7f, 0.7f);
}

FText UEdGraphNode_DungeonThemeMarker::GetTooltipText() const {
    return LOCTEXT("AnimSlotNode_Tooltip", "Plays animation from code using AnimMontage");
}

FText UEdGraphNode_DungeonThemeMarker::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    return FText::FromString(MarkerName);
}


void UEdGraphNode_DungeonThemeMarker::DestroyNode() {
    // Remove this node's references from marker emitter nodes that point to this
    TArray<UEdGraphNode_DungeonThemeMarkerEmitter*> EmitterNodes;
    GetGraph()->GetNodesOfClass<UEdGraphNode_DungeonThemeMarkerEmitter>(EmitterNodes);
    for (UEdGraphNode_DungeonThemeMarkerEmitter* EmitterNode : EmitterNodes) {
        if (EmitterNode->ParentMarker == this) {
            EmitterNode->ParentMarker = nullptr;
        }
    }

    UEdGraphNode_DungeonThemeBase::DestroyNode();
}

void UEdGraphNode_DungeonThemeMarker::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    UEdGraphNode_DungeonThemeBase::PostEditChangeProperty(e);
    if (e.Property && e.Property->GetFName() == "MarkerName") {
        GetGraph()->NotifyGraphChanged();
    }
}

bool UEdGraphNode_DungeonThemeMarker::CanEditChange(const FProperty* InProperty) const {
    if (InProperty->GetFName() == "MarkerName" && !bUserDefined) {
        return false;
    }

    return UEdGraphNode::CanEditChange(InProperty);
}

void UEdGraphNode_DungeonThemeMarker::AutowireNewNode(UEdGraphPin* FromPin) {
    if (!FromPin) {
        return;
    }

    UEdGraphPin* OutputPin = GetOutputPin();
    UEdGraphPin* InputPin = FromPin;

    if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Marker) {
        // Make sure we have no loops with this connection
        const UEdGraphSchema* Schema = GetGraph()->GetSchema();
        const FPinConnectionResponse ConnectionValid = Schema->CanCreateConnection(OutputPin, InputPin);
        if (ConnectionValid.Response == CONNECT_RESPONSE_MAKE) {
            OutputPin->MakeLinkTo(InputPin);
        }
    }
}

#undef LOCTEXT_NAMESPACE

