//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/Slate/SDAToolShutdownOverlay.h"

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorMode.h"

#include "InteractiveTool.h"
#include "SPrimaryButton.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDAToolShutdownOverlay"

void SDAToolShutdownOverlay::Construct(const FArguments& InArgs, const TWeakObjectPtr<UEdMode>& InEdMode) {
	EdModePtr = InEdMode;
	DisplayText = InArgs._DisplayText.Get();
	IconImage = InArgs._IconImage.Get();
	SetVisibility(EVisibility::SelfHitTestInvisible);
	
	const FSlateBrush* OverlayBrush = FAppStyle::Get().GetBrush("EditorViewport.OverlayBrush");
	ChildSlot
	[
		SNew(SHorizontalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.f, 15.f))
		[
			SNew(SBorder)
			.BorderImage(OverlayBrush)
			.Padding(8.f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
				[
					SNew(SImage)
					.Image(IconImage)
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
				[
					SNew(STextBlock)
					.Text(DisplayText)
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(0.0, 0.f, 2.f, 0.f))
				[
					SNew(SPrimaryButton)
					.Text(LOCTEXT("OverlayAccept", "Accept"))
					.ToolTipText(LOCTEXT("OverlayAcceptTooltip", "Accept/Commit the results of the active Tool [Enter]"))
					.OnClicked(this, &SDAToolShutdownOverlay::EndTool, EToolShutdownType::Accept)
					.IsEnabled(this, &SDAToolShutdownOverlay::CanAcceptActiveTool)
					.Visibility_Lambda([this]() { return ActiveToolHasAccept() ? EVisibility::Visible : EVisibility::Collapsed; })
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(2.0, 0.f, 0.f, 0.f))
				[
					SNew(SButton)
					.Text(LOCTEXT("OverlayCancel", "Cancel"))
					.ToolTipText(LOCTEXT("OverlayCancelTooltip", "Cancel the active Tool [Esc]"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SDAToolShutdownOverlay::EndTool, EToolShutdownType::Cancel)
					.IsEnabled(this, &SDAToolShutdownOverlay::CanCancelActiveTool)
					.Visibility_Lambda([this]() { return ActiveToolHasAccept() ? EVisibility::Visible : EVisibility::Collapsed; })
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(2.0, 0.f, 0.f, 0.f))
				[
					SNew(SPrimaryButton)
					.Text(LOCTEXT("OverlayComplete", "Complete"))
					.ToolTipText(LOCTEXT("OverlayCompleteTooltip", "Exit the active Tool [Enter]"))
					.OnClicked(this, &SDAToolShutdownOverlay::EndTool, EToolShutdownType::Completed)
					.IsEnabled(this, &SDAToolShutdownOverlay::CanCompleteActiveTool)
					.Visibility_Lambda([this]() { return CanCompleteActiveTool() ? EVisibility::Visible : EVisibility::Collapsed; })
				]
			]	
		]
	];
}

FReply SDAToolShutdownOverlay::EndTool(EToolShutdownType EndType) const {
	if (EdModePtr.IsValid()) {
		if (UEditorInteractiveToolsContext* ToolContext = EdModePtr->GetInteractiveToolsContext(EToolsContextScope::EdMode)) {
			ToolContext->EndTool(EndType);
		}
	}
	return FReply::Handled();
}

bool SDAToolShutdownOverlay::CanAcceptActiveTool() const {
	if (EdModePtr.IsValid()) {
		if (UEditorInteractiveToolsContext* ToolContext = EdModePtr->GetInteractiveToolsContext(EToolsContextScope::EdMode)) {
			return ToolContext->CanAcceptActiveTool();
		}
	}
	return false;
}

bool SDAToolShutdownOverlay::CanCancelActiveTool() const {
	if (EdModePtr.IsValid()) {
		if (UEditorInteractiveToolsContext* ToolContext = EdModePtr->GetInteractiveToolsContext(EToolsContextScope::EdMode)) {
			return ToolContext->CanCancelActiveTool();
		}
	}
	return false;
}

bool SDAToolShutdownOverlay::CanCompleteActiveTool() const {
	if (EdModePtr.IsValid()) {
		if (UEditorInteractiveToolsContext* ToolContext = EdModePtr->GetInteractiveToolsContext(EToolsContextScope::EdMode)) {
			return ToolContext->CanCompleteActiveTool();
		}
	}
	return true;
}

bool SDAToolShutdownOverlay::ActiveToolHasAccept() const {
	if (EdModePtr.IsValid()) {
		if (UEditorInteractiveToolsContext* ToolContext = EdModePtr->GetInteractiveToolsContext(EToolsContextScope::EdMode)) {
			return ToolContext->ActiveToolHasAccept();
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

