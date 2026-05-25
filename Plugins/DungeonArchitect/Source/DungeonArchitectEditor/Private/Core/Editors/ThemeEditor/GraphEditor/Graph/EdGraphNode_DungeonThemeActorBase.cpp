//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "Core/Common/Utils/AssetThumbnailCache.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"

#include "EdGraph/EdGraphSchema.h"
#include "GraphEditAction.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonActorBase"
DEFINE_LOG_CATEGORY(DungeonNodeLog);

UEdGraphNode_DungeonThemeActorBase::UEdGraphNode_DungeonThemeActorBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    Probability = 1.0f;
    Affinity = 1.0f;
    ConsumeOnAttach = true;

    ThumbnailCacheManager = MakeShareable(new FDAAssetThumbnailCacheManager(FIntPoint(128, 128)));
}

void UEdGraphNode_DungeonThemeActorBase::AllocateDefaultPins() {
    UEdGraphPin* Inputs = CreatePin(EGPD_Input, FDungeonThemePinDataTypes::PinType_Mesh, TEXT("In"));
    UEdGraphPin* Outputs = CreatePin(EGPD_Output, FDungeonThemePinDataTypes::PinType_Marker, TEXT("Out"));
}

FLinearColor UEdGraphNode_DungeonThemeActorBase::GetNodeTitleColor() const {
    return FLinearColor(0.7f, 0.7f, 0.7f);
}

FText UEdGraphNode_DungeonThemeActorBase::GetTooltipText() const {
    return LOCTEXT("AnimSlotNode_Tooltip", "Plays animation from code using AnimMontage");
}

FText UEdGraphNode_DungeonThemeActorBase::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    return FText::FromString("Mesh");
}

void UEdGraphNode_DungeonThemeActorBase::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    Super::PostEditChangeProperty(e);
    if (!e.Property) return;

    FName PropertyName = e.Property->GetFName();

    UE_LOG(DungeonNodeLog, Log, TEXT("Property Changed: %s"), *PropertyName.ToString());

    if (UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(GetGraph())) {
        FEdGraphEditAction Action;
        Action.Action = GRAPHACTION_Default;
        Action.bUserInvoked = true;
        Action.Graph = ThemeGraph;
        Action.Nodes.Add(this);
        ThemeGraph->NotifyNodePropertyChanged(Action, e);
    }
}

void UEdGraphNode_DungeonThemeActorBase::AutowireNewNode(UEdGraphPin* FromPin) {
    if (!FromPin) {
        return;
    }

    UEdGraphPin* OutputPin = nullptr;
    UEdGraphPin* InputPin = nullptr;

    if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Mesh) {
        OutputPin = FromPin;
        InputPin = GetInputPin();
    }
    else if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Marker) {
        OutputPin = GetOutputPin();
        InputPin = FromPin;
    }

    const UEdGraphSchema* Schema = GetGraph()->GetSchema();
    const FPinConnectionResponse ConnectionValid = Schema->CanCreateConnection(OutputPin, InputPin);
    if (ConnectionValid.Response == CONNECT_RESPONSE_MAKE) {
        OutputPin->MakeLinkTo(InputPin);
    }
}

void UEdGraphNode_DungeonThemeActorBase::NodeConnectionListChanged() {
    UEdGraphNode_DungeonThemeBase::NodeConnectionListChanged();

    if (GetInputPin()->LinkedTo.Num() == 0) {
        ExecutionOrder = 1;
    }
}

TArray<UObject*> UEdGraphNode_DungeonThemeActorBase::GetThumbnailAssetObjects() const {
    return { GetNodeAssetObject(GetTransientPackage()) };
}

FLinearColor UEdGraphNode_DungeonThemeActorBase::GetBorderColor() {
    static constexpr FLinearColor BorderColor(0.08f, 0.08f, 0.08f);
    return BorderColor;
}

#undef LOCTEXT_NAMESPACE

