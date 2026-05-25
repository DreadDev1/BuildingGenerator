//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "Widgets/SCompoundWidget.h"

class SBorder;
class SDADraggableBoxOverlay;

class SDADraggableToolOverlayWidget : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SDADraggableToolOverlayWidget) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	virtual void ResetPositionInViewport();
	
protected:
	TSharedPtr<SDADraggableBoxOverlay> DraggableBoxOverlay;
	TSharedPtr<SBorder> WidgetContents;
	
	// The values to which the UI positioning is reset if asked
	float DefaultLeftPadding = 15.0f;
	float DefaultVerticalPadding = 75.0f;
	
	bool bIsEnabled = true;
	bool bPositionRelativeToBottom = true;
}; 

