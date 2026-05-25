//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SDungeonForgeEditorModeToolbar : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SDungeonForgeEditorModeToolbar) {}
	SLATE_EVENT(FSimpleDelegate, OnBuildButtonClicked)
SLATE_END_ARGS()
	
void Construct(const FArguments& InArgs);

private:
	FReply HandleBuildClicked() const;

private:
	FSimpleDelegate OnBuildButtonClicked;
};

