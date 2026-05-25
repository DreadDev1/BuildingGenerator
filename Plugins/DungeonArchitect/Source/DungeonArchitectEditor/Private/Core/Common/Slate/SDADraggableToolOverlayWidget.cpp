//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/Slate/SDADraggableToolOverlayWidget.h"

#include "Core/Common/Slate/SDADraggableBox.h"

#include "Widgets/Layout/SBorder.h"

///////////////////////////// SDungeonThemeEdModeSelectToolWidget /////////////////////////////
void SDADraggableToolOverlayWidget::Construct(const FArguments& InArgs) {
	ChildSlot
	[
		SAssignNew(DraggableBoxOverlay, SDADraggableBoxOverlay)
		.bPositionRelativeToBottom(bPositionRelativeToBottom)
		.Visibility_Lambda([this]()
		{
			return bIsEnabled ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		[
			SAssignNew(WidgetContents, SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("EditorViewport.OverlayBrush"))
			.Padding(8.f)
			[
				InArgs._Content.Widget
			]
		]
	];

	// Don't block things under the overlay.
	SetVisibility(EVisibility::SelfHitTestInvisible);

	ResetPositionInViewport();
}

void SDADraggableToolOverlayWidget::ResetPositionInViewport() {
	if (DraggableBoxOverlay.IsValid()) {
		DraggableBoxOverlay->SetBoxPosition(DefaultLeftPadding, DefaultVerticalPadding);
	}
}


