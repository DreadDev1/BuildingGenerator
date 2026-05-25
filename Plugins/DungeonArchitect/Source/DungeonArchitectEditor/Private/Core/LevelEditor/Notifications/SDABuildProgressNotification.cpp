//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Notifications/SDABuildProgressNotification.h"

#include "Components/VerticalBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDABuildProgressNotification"


void SDABuildProgressNotification::Construct(const FArguments& InArgs)
{
	Dungeon = InArgs._Dungeon;
	StatusMessage = InArgs._StatusMessage;
	
	const TSharedPtr<SProgressBar> ProgressBar = SNew(SProgressBar)
		.Percent(this, &SDABuildProgressNotification::GetProgressBarPercent)
		.FillColorAndOpacity(FSlateColor(FLinearColor(0.0f, 1.0f, 1.0f)))
		.Visibility_Lambda([this]() {
			return StatusMessage.bBuildInProgress && StatusMessage.bShowProgressBar ? EVisibility::Visible : EVisibility::Hidden;
		});
	

	ChildSlot
	[
		SNew(SBox)
		.Padding(15.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(STextBlock)
				.Text(this, &SDABuildProgressNotification::GetTextContent)
				.Font(FAppStyle::Get().GetFontStyle(TEXT("NotificationList.FontBold")))
			]
			+ SVerticalBox::Slot()
			[
				SNew(STextBlock)
				.Text(this, &SDABuildProgressNotification::GetSubTextContent)
				.Font(FAppStyle::Get().GetFontStyle(TEXT("NotificationList.FontLight")))
			]
			+ SVerticalBox::Slot()
			.Padding(FMargin(0.0f, 10.0f, 0.0f, 2.0f))
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					ProgressBar.ToSharedRef()
				]
			]
		]
	];
}

void SDABuildProgressNotification::SetStatusMessage(const FDungeonBuildStatusMessage& InStatusMessage) {
	StatusMessage = InStatusMessage;
}

TOptional<float> SDABuildProgressNotification::GetProgressBarPercent() const {
	if (!StatusMessage.bShowProgressBar || StatusMessage.TotalWorkUnits <= 0) {
		return 0.0f;
	}
	return static_cast<float>(StatusMessage.CompletedWorkUnits) / static_cast<float>(StatusMessage.TotalWorkUnits);
}

FText SDABuildProgressNotification::GetTextContent() const {
	if (StatusMessage.bBuildInProgress) {
		FFormatNamedArguments Args;
		Args.Add(TEXT("Status"), StatusMessage.ProgressText);
		return FText::Format(LOCTEXT("NotificationTextTemplate", "Building Dungeon: {Status}"), Args);
	}
	else {
		static const FText BuildCompleteText = LOCTEXT("DungeonBuildCompleteText", "Dungeon Build Complete");
		return BuildCompleteText;
	}
}

FText SDABuildProgressNotification::GetSubTextContent() const {
	return StatusMessage.bBuildInProgress ? StatusMessage.ProgressSubText : FText::GetEmpty();
}


#undef LOCTEXT_NAMESPACE

