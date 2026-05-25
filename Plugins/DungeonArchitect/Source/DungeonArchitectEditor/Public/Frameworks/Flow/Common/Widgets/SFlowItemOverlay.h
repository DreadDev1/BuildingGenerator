//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Common/Widgets/FlowItemOverlayDelegates.h"

#include "Widgets/SCompoundWidget.h"

class UFlowGraphItem;

/** Widget for overlaying an execution-order index onto a node */
class SFlowItemOverlay : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SFlowItemOverlay)
            : _Selected(false) {
        }

        SLATE_ATTRIBUTE(bool, Selected)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const UFlowGraphItem* InItem);
    virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

    FSlateColor GetColor() const;
    FSlateColor GetTextColor() const;
    FText GetItemText() const;
    float GetWidgetRadius() const { return WidgetRadius; }
    const UFlowGraphItem* GetItem() const { return Item.Get(); }
    void SetBaseOffset(const FVector2D& InBaseOffset);
    FVector2D GetBaseOffset() const { return BaseOffset; }
    FFlowItemWidgetEvent& GetOnMousePressed() { return OnMousePressed; }
    EVisibility GetSelectionImageVisiblity() const;

private:
    FFlowItemWidgetEvent OnMousePressed;
    TWeakObjectPtr<const UFlowGraphItem> Item;
    float WidgetRadius = 0;
    bool bHovered = false;
    FVector2D BaseOffset;
    TAttribute<bool> Selected;
};

