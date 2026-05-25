//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/DungeonThemeConnectionDrawingPolicy.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"

#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonMesh"

UEdGraphNode_DungeonThemeMarkerEmitter::UEdGraphNode_DungeonThemeMarkerEmitter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
}

void UEdGraphNode_DungeonThemeMarkerEmitter::AllocateDefaultPins() {
    // Create the input pin
    CreatePin(EGPD_Input, FDungeonThemePinDataTypes::PinType_Marker, TEXT("In"));
}

FLinearColor UEdGraphNode_DungeonThemeMarkerEmitter::GetNodeTitleColor() const {
    return FLinearColor(0.7f, 0.7f, 0.7f);
}

FText UEdGraphNode_DungeonThemeMarkerEmitter::GetTooltipText() const {
    return LOCTEXT("AnimSlotNode_Tooltip", "Plays animation from code using AnimMontage");
}

FText UEdGraphNode_DungeonThemeMarkerEmitter::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    FString MarkerName = ParentMarker ? ParentMarker->MarkerName : "[INVALID]";
    return FText::FromString(MarkerName);
}


void UEdGraphNode_DungeonThemeMarkerEmitter::AutowireNewNode(UEdGraphPin* FromPin) {
    if (!FromPin) {
        return;
    }

    UEdGraphPin* OutputPin = FromPin;
    UEdGraphPin* InputPin = GetInputPin();

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

