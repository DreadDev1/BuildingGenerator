//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Common/Widgets/SFlowItemOverlay.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SFlowItemOverlay"

void SFlowItemOverlay::Construct(const FArguments& InArgs, const UFlowGraphItem* InItem) {
    Item = InItem;
    Selected = InArgs._Selected;

    // TODO: Move this to DA style sheet
    const FSlateBrush* BodyBrush = FDungeonArchitectStyle::Get().GetBrush(TEXT("FlowEditor.ItemWidget.Body"));
    const FSlateBrush* SelectionBrush = FDungeonArchitectStyle::Get().GetBrush(
        TEXT("FlowEditor.ItemWidget.Body.Selected"));
    FSlateFontInfo Font = FDungeonArchitectStyle::Get().GetFontStyle("FlowEditor.ItemWidget.Text");
    if (Item.IsValid() && Item->CustomInfo.FontScale != 1.0f) {
        Font.Size = FMath::RoundToInt(Font.Size * Item->CustomInfo.FontScale);
    }

    WidgetRadius = BodyBrush->ImageSize.X * 0.5f;
    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
          .HAlign(HAlign_Fill)
          .VAlign(VAlign_Fill)
        [
            // Add a dummy box here to make sure the widget doesn't get smaller than the brush
            SNew(SBox)
			.WidthOverride(BodyBrush->ImageSize.X)
			.HeightOverride(BodyBrush->ImageSize.Y)
        ]
        + SOverlay::Slot()
          .HAlign(HAlign_Fill)
          .VAlign(VAlign_Fill)
        [
            SNew(SBorder)
			.BorderImage(BodyBrush)
			.BorderBackgroundColor(this, &SFlowItemOverlay::GetColor)
			.Padding(FMargin(4.0f, 0.0f, 4.0f, 1.0f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
            [
                SNew(STextBlock)
				.Text(this, &SFlowItemOverlay::GetItemText)
				.ColorAndOpacity(this, &SFlowItemOverlay::GetTextColor)
				.Font(Font)
            ]
        ]
        + SOverlay::Slot()
          .HAlign(HAlign_Fill)
          .VAlign(VAlign_Fill)
        [
            SNew(SBorder)
			.Visibility(this, &SFlowItemOverlay::GetSelectionImageVisiblity)
			.BorderImage(SelectionBrush)
        ]

    ];
}

void SFlowItemOverlay::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
    SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
    bHovered = true;
}

void SFlowItemOverlay::OnMouseLeave(const FPointerEvent& MouseEvent) {
    SCompoundWidget::OnMouseLeave(MouseEvent);
    bHovered = false;
}

FReply SFlowItemOverlay::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
    if (Item.IsValid()) {
        OnMousePressed.ExecuteIfBound(Item->ItemId, false);
    }
    return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SFlowItemOverlay::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) {
    if (Item.IsValid()) {
        OnMousePressed.ExecuteIfBound(Item->ItemId, true);
    }
    return SCompoundWidget::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

FSlateColor SFlowItemOverlay::GetColor() const {
    return FFlowItemVisuals::GetBackgroundColor(Item.Get(), bHovered);
}

FSlateColor SFlowItemOverlay::GetTextColor() const {
    return FFlowItemVisuals::GetTextColor(Item.Get(), bHovered);
}

FText SFlowItemOverlay::GetItemText() const {
    return FText::FromString(FFlowItemVisuals::GetText(Item.Get()));
}

void SFlowItemOverlay::SetBaseOffset(const FVector2D& InBaseOffset) {
    BaseOffset = InBaseOffset;
}

EVisibility SFlowItemOverlay::GetSelectionImageVisiblity() const {
    return Selected.Get() ? EVisibility::Visible : EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE

