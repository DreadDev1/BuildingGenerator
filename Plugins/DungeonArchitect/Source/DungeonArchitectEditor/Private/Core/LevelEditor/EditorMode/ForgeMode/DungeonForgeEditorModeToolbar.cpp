//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolbar.h"

#include "Components/HorizontalBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorModeToolbar"

void SDungeonForgeEditorModeToolbar::Construct(const FArguments& InArgs) {
	OnBuildButtonClicked = InArgs._OnBuildButtonClicked;
	
	ChildSlot
    [
        SNew(SBorder)
        .Padding(FMargin(8))
        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
        [
            SNew(SHorizontalBox)
        	+SHorizontalBox::Slot()
	        .Padding(2)
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("BuildButtonLabel", "Build"))
				.ContentPadding(FMargin(8, 4)) 
				.OnClicked_Raw(this, &SDungeonForgeEditorModeToolbar::HandleBuildClicked)
			]
        ]
    ];
}

FReply SDungeonForgeEditorModeToolbar::HandleBuildClicked() const {
	if (OnBuildButtonClicked.IsBound()) {
		OnBuildButtonClicked.Execute();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

