//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "SNodePanel.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UFlowLayoutEdGraphNode;
class SGraphPin;
class SFlowItemOverlay;

class SGraphNode_FlowLayoutNode : public SGraphNode {
public:
    SLATE_BEGIN_ARGS(SGraphNode_FlowLayoutNode) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UFlowLayoutEdGraphNode* InNode);

    struct FLinkItemWidgetInfo {
        TSharedPtr<SFlowItemOverlay> ItemWidget;
        FGuid DestinationNodeId;
    };

    // SGraphNode interface
    virtual void UpdateGraphNode() override;
    virtual void CreatePinWidgets() override;
    virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
    virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const override;
    virtual FVector2D ComputeDesiredSize(float) const override;
    // End of SGraphNode interface

    TArray<TSharedPtr<SFlowItemOverlay>> GetNodeItemWidgets() const { return NodeItemWidgets; }
    TArray<FLinkItemWidgetInfo> GetLinkItemWidgets() const { return LinkItemWidgets; }

protected:
    static FText GetPreviewCornerText();
    FSlateColor GetNodeColor() const;
    FSlateColor GetTextColor() const;
    FLinearColor GetTextShadowColor() const;
    void CreateNodeItemWidgets();
    void CreateLinkItemWidgets();
    void OnItemClicked(const FGuid& InItemId, bool bDoubleClicked) const;
    bool IsItemSelected(FGuid InItemId) const;

protected:
    TArray<TSharedPtr<SFlowItemOverlay>> NodeItemWidgets;
    TArray<FLinkItemWidgetInfo> LinkItemWidgets;
};

