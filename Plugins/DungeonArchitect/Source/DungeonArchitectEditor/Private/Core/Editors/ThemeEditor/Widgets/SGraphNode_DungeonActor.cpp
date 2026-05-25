//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Widgets/SGraphNode_DungeonActor.h"

#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMeshList.h"
#include "Core/Editors/ThemeEditor/Widgets/SDungeonOutputPin.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "AssetThumbnail.h"
#include "EdGraph/EdGraph.h"
#include "SGraphPanel.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "SGraphNode_DungeonActorBase"


/** Widget for overlaying an execution-order index onto a node */
class SMeshGraphNodeIndex : public SCompoundWidget {
public:
    /** Delegate event fired when the hover state of this widget changes */
    DECLARE_DELEGATE_OneParam(FOnHoverStateChanged, bool /* bHovered */);

    /** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
    DECLARE_DELEGATE_RetVal_OneParam(FSlateColor, FOnGetIndexColor, bool /* bHovered */);

    SLATE_BEGIN_ARGS(SMeshGraphNodeIndex) {}
        SLATE_ATTRIBUTE(FText, Text)
        SLATE_EVENT(FOnHoverStateChanged, OnHoverStateChanged)
        SLATE_EVENT(FOnGetIndexColor, OnGetIndexColor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs) {
        OnHoverStateChangedEvent = InArgs._OnHoverStateChanged;
        OnGetIndexColorEvent = InArgs._OnGetIndexColor;

        const FSlateBrush* IndexBrush = FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Index"));

        ChildSlot
        [
            SNew(SOverlay)
            + SOverlay::Slot()
              .HAlign(HAlign_Fill)
              .VAlign(VAlign_Fill)
            [
                // Add a dummy box here to make sure the widget doesn't get smaller than the brush
                SNew(SBox)
						.WidthOverride(IndexBrush->ImageSize.X)
						.HeightOverride(IndexBrush->ImageSize.Y)
            ]
            + SOverlay::Slot()
              .HAlign(HAlign_Fill)
              .VAlign(VAlign_Fill)
            [
                SNew(SBorder)
				.BorderImage(IndexBrush)
				.BorderBackgroundColor(this, &SMeshGraphNodeIndex::GetColor)
				.Padding(FMargin(4.0f, 0.0f, 4.0f, 1.0f))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
					.Text(InArgs._Text)
					.Font(FDungeonArchitectStyle::Get().GetFontStyle("ThemeEd.Graph.ThemeNode.IndexText"))
                    
                ]
            ]
        ];
    }

    virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override {
        OnHoverStateChangedEvent.ExecuteIfBound(true);
        SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
    }

    virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override {
        OnHoverStateChangedEvent.ExecuteIfBound(false);
        SCompoundWidget::OnMouseLeave(MouseEvent);
    }

    /** Get the color we use to display the rounded border */
    FSlateColor GetColor() const {
        if (OnGetIndexColorEvent.IsBound()) {
            return OnGetIndexColorEvent.Execute(IsHovered());
        }

        return FSlateColor::UseForeground();
    }

private:
    /** Delegate event fired when the hover state of this widget changes */
    FOnHoverStateChanged OnHoverStateChangedEvent;

    /** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
    FOnGetIndexColor OnGetIndexColorEvent;
};


void SGraphNode_DungeonActorBase::Construct(const FArguments& InArgs, UEdGraphNode_DungeonThemeActorBase* InNode) {
    ThumbnailSize = FIntPoint(128, 128);
    GraphNode = InNode;
    EdActorNode = InNode;
    BuildThumbnailWidget();
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}


FSlateColor SGraphNode_DungeonActorBase::GetBorderBackgroundColor() const {
    static const FLinearColor BorderColor(0.08f, 0.08f, 0.08f);
    if (EdActorNode) {
        return EdActorNode->GetBorderColor();
    }
    
    return BorderColor;
}

const FSlateBrush* SGraphNode_DungeonActorBase::GetNameIcon() const {
    return FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}

TOptional<float> SGraphNode_DungeonActorBase::OnGetAffinityValue() const {
    if (UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(GraphNode)) {
        return ActorNode->Probability;
    }
    return TOptional<float>();
}

void SGraphNode_DungeonActorBase::OnAffinityValueChanged(float NewValue) {
    if (UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(GraphNode)) {
        ActorNode->Probability = NewValue;
    }
}

void SGraphNode_DungeonActorBase::OnAffinityValueCommitted(float NewValue, ETextCommit::Type CommitInfo) {
    OnAffinityValueChanged(NewValue);
}

void SGraphNode_DungeonActorBase::UpdateGraphNode() {
    InputPins.Empty();
    OutputPins.Empty();

    // Reset variables that are going to be exposed, in case we are refreshing an already setup node.
    RightNodeBox.Reset();
    LeftNodeBox.Reset();
    OutputPinBox.Reset();

    const FSlateBrush* NodeTypeIcon = GetNameIcon();

    FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
    TSharedPtr<SErrorText> ErrorText;
    TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

    IndexOverlay = SNew(SMeshGraphNodeIndex)
		.ToolTipText(this, &SGraphNode_DungeonActorBase::GetIndexTooltipText)
		.Visibility(this, &SGraphNode_DungeonActorBase::GetIndexVisibility)
		.Text(this, &SGraphNode_DungeonActorBase::GetIndexText)
		.OnHoverStateChanged(this, &SGraphNode_DungeonActorBase::OnIndexHoverStateChanged)
		.OnGetIndexColor(this, &SGraphNode_DungeonActorBase::GetIndexColor);

    this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
    this->GetOrAddSlot(ENodeZone::Center)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
    [
        SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0)
			.BorderBackgroundColor(this, &SGraphNode_DungeonActorBase::GetBorderBackgroundColor)
        [
            SNew(SOverlay)

            // INPUT PIN AREA
            + SOverlay::Slot()
              .HAlign(HAlign_Fill)
              .VAlign(VAlign_Top)
            [
                SAssignNew(LeftNodeBox, SVerticalBox)
            ]

            // OUTPUT PIN AREA
            + SOverlay::Slot()
              .HAlign(HAlign_Fill)
              .VAlign(VAlign_Bottom)
            [
                SAssignNew(RightNodeBox, SVerticalBox)
                + SVerticalBox::Slot()
                  .HAlign(HAlign_Fill)
                  .VAlign(VAlign_Fill)
                  .Padding(20.0f, 0.0f)
                  .FillHeight(1.0f)
                [
                    SAssignNew(OutputPinBox, SHorizontalBox)
                ]
            ]


            // THUMBNAIL AREA
            + SOverlay::Slot()
              .HAlign(HAlign_Center)
              .VAlign(VAlign_Center)
              .Padding(10.0f)
            [
                SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
						.BorderBackgroundColor(TitleShadowColor)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    // Thumbnail Slot
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    [
                        SNew(SBox)
								.WidthOverride(ThumbnailSize.X)
								.HeightOverride(ThumbnailSize.Y)
                        [
                            GetThumbnailWidget()
                        ]
                    ]
                ]
            ]
        ]
    ];

    CreatePinWidgets();
}

void SGraphNode_DungeonActorBase::OnIndexHoverStateChanged(bool bHovered) {
    UEdGraphNode_DungeonThemeBase* ParentNode = FDungeonArchitectThemeEditorUtils::GetParentNode(GraphNode);
    if (ParentNode != nullptr) {
        ParentNode->bHighlightChildNodeIndices = bHovered;
    }
}

FSlateColor SGraphNode_DungeonActorBase::GetIndexColor(bool bHovered) const {
    UEdGraphNode_DungeonThemeBase* ParentNode = FDungeonArchitectThemeEditorUtils::GetParentNode(GraphNode);
    const bool bHighlightHover = bHovered || (ParentNode && ParentNode->bHighlightChildNodeIndices);

    static const FName NodeHoveredColor("ThemeEd.Graph.ThemeNode.Index.BackgroundHoveredColor");
    static const FName NodeDefaultColor("ThemeEd.Graph.ThemeNode.Index.BackgroundColor");
    
    return bHovered
               ? FDungeonArchitectStyle::Get().GetSlateColor(NodeHoveredColor)
               : FDungeonArchitectStyle::Get().GetSlateColor(NodeDefaultColor);
}

EVisibility SGraphNode_DungeonActorBase::GetIndexVisibility() const {
    return EVisibility::Visible;
}

FText SGraphNode_DungeonActorBase::GetIndexText() const {
    UEdGraphNode_DungeonThemeActorBase* ActorNode = CastChecked<UEdGraphNode_DungeonThemeActorBase>(GraphNode);
    return FText::AsNumber(ActorNode->ExecutionOrder);
}

FText SGraphNode_DungeonActorBase::GetIndexTooltipText() const {
    if (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != nullptr) {
        return LOCTEXT("ExecutionIndexTooltip", "Execution index: this shows the order in which nodes are executed.");
    }
    return LOCTEXT("ChildIndexTooltip", "Child index: this shows the order in which child nodes are executed.");
}

void SGraphNode_DungeonActorBase::CreatePinWidgets() {
    UEdGraphNode_DungeonThemeActorBase* DungeonNode = CastChecked<UEdGraphNode_DungeonThemeActorBase>(GraphNode);

    {
        UEdGraphPin* CurPin = DungeonNode->GetOutputPin();
        TSharedPtr<SGraphPin> NewPin = SNew(SDungeonOutputPin, CurPin);
        NewPin->SetIsEditable(IsEditable);
        this->AddPin(NewPin.ToSharedRef());
        //OutputPins.Add(NewPin.ToSharedRef());
    }
    {
        UEdGraphPin* CurPin = DungeonNode->GetInputPin();
        TSharedPtr<SGraphPin> NewPin = SNew(SDungeonOutputPin, CurPin);
        NewPin->SetIsEditable(IsEditable);
        this->AddPin(NewPin.ToSharedRef());
        //InputPins.Add(NewPin.ToSharedRef());
    }
}

void SGraphNode_DungeonActorBase::AddPin(const TSharedRef<SGraphPin>& PinToAdd) {
    /*
    PinToAdd->SetOwner(SharedThis(this));
    RightNodeBox->AddSlot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .FillHeight(1.0f)
        [
            PinToAdd
        ];
        */
    PinToAdd->SetOwner(SharedThis(this));

    const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
    const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
    if (bAdvancedParameter) {
        PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
    }

    if (PinToAdd->GetDirection() == EGPD_Input) {
        LeftNodeBox->AddSlot()
                   .HAlign(HAlign_Fill)
                   .VAlign(VAlign_Fill)
                   .FillHeight(1.0f)
                   .Padding(20.0f, 0.0f)
        [
            PinToAdd
        ];
        InputPins.Add(PinToAdd);
    }
    else // Direction == EEdGraphPinDirection::EGPD_Output
    {
        OutputPinBox->AddSlot()
                    .HAlign(HAlign_Fill)
                    .VAlign(VAlign_Fill)
                    .FillWidth(1.0f)
        [
            PinToAdd
        ];
        OutputPins.Add(PinToAdd);
    }
}


TArray<FOverlayWidgetInfo> SGraphNode_DungeonActorBase::GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const {
    TArray<FOverlayWidgetInfo> Widgets;
    check(IndexOverlay.IsValid());

    FVector2D Origin(0.0f, 0.0f);

    FOverlayWidgetInfo Overlay(IndexOverlay);
    //Overlay.OverlayOffset = FVector2D(WidgetSize.X - (IndexOverlay->GetDesiredSize().X * 0.5f), Origin.Y);
    Overlay.OverlayOffset = FVector2D(WidgetSize.X - 10, Origin.Y);
    Widgets.Add(Overlay);


    return Widgets;
}


void SGraphNode_DungeonActorBase::RebuildExecutionOrder() const {
    UEdGraphNode_DungeonThemeBase* Parent = FDungeonArchitectThemeEditorUtils::GetParentNode(GraphNode);
    if (Parent) {
        Parent->UpdateChildExecutionOrder();
        
        if (TSharedPtr<SGraphPanel> GraphPanel = GetOwnerPanel()) {
            if (UEdGraphPin* OutputPin = Parent->GetOutputPin()) {
                for (UEdGraphPin* ChildNodePin : OutputPin->LinkedTo) {
                    if (ChildNodePin) {
                        if (UEdGraphNode_DungeonThemeBase* ChildThemeNode = Cast<UEdGraphNode_DungeonThemeBase>(ChildNodePin->GetOwningNode())) {
                            GraphPanel->RefreshNode(*ChildThemeNode);
                        }
                    }
                }
            }
        }
    }
}


void SGraphNode_DungeonActorBase::MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty) {
    RebuildExecutionOrder();
    return SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
}

bool SGraphNode_DungeonActorBase::GetObjectOrAssetData(const UObject* InObject, FObjectOrAssetData& OutValue) const {
    // Potentially accessing the value while garbage collecting or saving the package could trigger a crash.
    // so we fail to get the value when that is occurring.
    if (GIsSavingPackage || IsGarbageCollecting()) {
        return false;
    }

    OutValue = FObjectOrAssetData(InObject);

    const bool bFoundObject = (InObject == nullptr);
    return bFoundObject;
}

///////////////////////////////////////////////////////////////////

void SGraphNode_DungeonActor::Construct(const FArguments& InArgs, UEdGraphNode_DungeonThemeActorBase* InNode) {
    SGraphNode_DungeonActorBase::Construct({}, InNode);

}

void SGraphNode_DungeonActor::BuildThumbnailWidget() {
    const TArray<UObject*> ThumbnailObjects = EdActorNode ? EdActorNode->GetThumbnailAssetObjects() : TArray<UObject*>();
    if (UEdGraphNode_DungeonThemeActorBase* ActorEdNode = Cast<UEdGraphNode_DungeonThemeActorBase>(GetNodeObj())) {
        ThumbnailWidget = FDungeonArchitectThemeEditorUtils::CreateAssetThumbWidget(ThumbnailObjects, ActorEdNode->GetThumbnailCacheManager());
    }
}

TSharedRef<SWidget> SGraphNode_DungeonActor::GetThumbnailWidget() const {
    return ThumbnailWidget.ToSharedRef();
}

void SGraphNode_DungeonActor::OnPropertyChanged(UEdGraphNode_DungeonThemeBase* Sender, const FName& PropertyName) {
    if (UEdGraphNode_DungeonThemeMeshList* MeshListNode = Cast<UEdGraphNode_DungeonThemeMeshList>(Sender)) {
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UEdGraphNode_DungeonThemeMeshList, Meshes)) {
            BuildThumbnailWidget();
        }
    }
    GraphNode->GetGraph()->NotifyGraphChanged();
}

#undef LOCTEXT_NAMESPACE

