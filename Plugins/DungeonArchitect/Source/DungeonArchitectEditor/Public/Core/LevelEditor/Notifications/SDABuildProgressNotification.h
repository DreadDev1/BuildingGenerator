//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Dungeon.h"

#include "Widgets/Notifications/INotificationWidget.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Simple widget to display progress bar when sending a pak buffer to a remote server.
 *
 * Designed to be used as the content of an SNotificationItem.
 */
class SDABuildProgressNotification : public SCompoundWidget, public INotificationWidget
{
public:
	SLATE_BEGIN_ARGS(SDABuildProgressNotification) {}

	SLATE_ARGUMENT(TWeakObjectPtr<ADungeon>, Dungeon)
	SLATE_ARGUMENT(FDungeonBuildStatusMessage, StatusMessage)
		
	SLATE_END_ARGS()

	/** Native construct */
	void Construct(const FArguments& InArgs);
	
	//~ Begin INotificationWidget interface
	virtual void OnSetCompletionState(SNotificationItem::ECompletionState State) override {}
	virtual TSharedRef<SWidget> AsWidget() override
	{
		return AsShared();
	}
	//~ End INotificationWidget interface
	
	void SetStatusMessage(const FDungeonBuildStatusMessage& InStatusMessage);
	
protected:
	/** Returns the actual progression based on TotalBytes and CurrentBytes, ranging from 0.f to 1.f */
	TOptional<float> GetProgressBarPercent() const;

	/**
	 * Returns text content for TextBlock heading of the widget
	 *
	 * If TotalBytes is set (not 0), will return CurrentBytes / TotalBytes in human readable form.
	 */
	FText GetTextContent() const;

	FText GetSubTextContent() const;
private:
	TWeakObjectPtr<ADungeon> Dungeon;
	FDungeonBuildStatusMessage StatusMessage;
};

