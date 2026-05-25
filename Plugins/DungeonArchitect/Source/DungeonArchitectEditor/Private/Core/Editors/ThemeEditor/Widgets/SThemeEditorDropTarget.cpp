//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Widgets/SThemeEditorDropTarget.h"


void SThemeEditorDropTarget::Construct(const FArguments& InArgs) {
    Super::FArguments ParentArgs;
    ParentArgs._Content = InArgs._Content;
    ParentArgs._OnAssetsDropped = InArgs._OnAssetsDropped;
    ParentArgs._OnAreAssetsAcceptableForDrop = InArgs._OnAreAssetsAcceptableForDrop;
    ParentArgs._OnAreAssetsAcceptableForDropWithReason = InArgs._OnAreAssetsAcceptableForDropWithReason;
    ParentArgs._bSupportsMultiDrop = InArgs._bSupportsMultiDrop;

    Super::Construct(ParentArgs);

    SetCanTick(true);
    SetVisibility(EVisibility::HitTestInvisible);
}

FReply SThemeEditorDropTarget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) {
    PanelCoordDropPosition = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
    return SAssetDropTarget::OnDrop(MyGeometry, DragDropEvent);
}

void SThemeEditorDropTarget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
                                  const float InDeltaTime) {
    SAssetDropTarget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    // Enable mouse integration while dragging
    if (GetDragOverlayVisibility().IsVisible()) {
        SetVisibility(EVisibility::Visible);
    }
    else {
        SetVisibility(EVisibility::SelfHitTestInvisible);
    }
}

