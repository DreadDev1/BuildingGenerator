//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Notifications/DungeonArchitectNotificationSubsystem.h"

#include "Core/Dungeon.h"
#include "Core/LevelEditor/Notifications/SDABuildProgressNotification.h"

#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DANotificationSubsystem"

UDungeonArchitectNotificationSubsystem& UDungeonArchitectNotificationSubsystem::Get() {
	check(GEditor);
	return *GEditor->GetEditorSubsystem<UDungeonArchitectNotificationSubsystem>();
}

TSharedPtr<SNotificationItem> UDungeonArchitectNotificationSubsystem::AddSimpleNotification(const FText& InNotificationText)
{
	const FNotificationInfo NotifyInfo(InNotificationText);
	return FSlateNotificationManager::Get().AddNotification(NotifyInfo);
}

TSharedPtr<SNotificationItem> UDungeonArchitectNotificationSubsystem::AddSimpleNotification(const FNotificationInfo& InInfo)
{
	return FSlateNotificationManager::Get().AddNotification(InInfo);
}

void UDungeonArchitectNotificationSubsystem::OnDungeonBuildStarted(ADungeon* InDungeon) {
	if (!InDungeon) {
		return;
	}
	BuildStatusNotifications.RemoveAllSwap([this, InDungeon](FDungeonArchitectNotificationTask& NotificationInfo) {
			if (NotificationInfo.Dungeon == InDungeon) {
				DismissNotification(NotificationInfo);
				return true;  // Remove this item
			}
			if (!NotificationInfo.Dungeon.IsValid()) {
				return true;  // Remove this item
			}

			// Keep this item
			return false;
		}, EAllowShrinking::No);
	
	// Create a notification task
	FDungeonBuildStatusMessage StatusMessage = InDungeon->GetBuildStatusMessage();
	FDungeonArchitectNotificationTask& Task = BuildStatusNotifications.AddDefaulted_GetRef();
	Task.Dungeon = InDungeon;
	Task.Widget = SNew(SDABuildProgressNotification)
		.Dungeon(Task.Dungeon)
		.StatusMessage(StatusMessage);
			
	FNotificationInfo Info(Task.Widget);
	Info.WidthOverride = 380.f;
	Info.FadeInDuration = 0.2f;
	Info.ExpireDuration = 2.f;
	Info.FadeOutDuration = 1.f;
	Info.bFireAndForget = false;
	Info.bAllowThrottleWhenFrameRateIsLow = false;

	Task.Notification = FSlateNotificationManager::Get().AddNotification(Info);
	Task.Notification->SetCompletionState(SNotificationItem::CS_Pending);
}


UWorld* UDungeonArchitectNotificationSubsystem::GetTickableGameObjectWorld() const {
	return GetWorld();
}

ETickableTickType UDungeonArchitectNotificationSubsystem::GetTickableTickType() const {
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool UDungeonArchitectNotificationSubsystem::IsAllowedToTick() const {
	return BuildStatusNotifications.Num() > 0;
}

void UDungeonArchitectNotificationSubsystem::Tick(float DeltaTime) {
	// Cleanup the notification list
	BuildStatusNotifications.RemoveAllSwap([this](const FDungeonArchitectNotificationTask& NotificationInfo) {
		if (!NotificationInfo.Dungeon.IsValid()) {
			// Remove this item
			return true;
		}
		if (NotificationInfo.bComplete) {
			// Remove this item
			return true;
		}
		
		// Keep this item
		return false;
	}, EAllowShrinking::No);
	
	for (FDungeonArchitectNotificationTask& TaskInfo : BuildStatusNotifications) {
		if (!TaskInfo.Dungeon.IsValid()) {
			continue;
		}

		if (!TaskInfo.bComplete) {
			FDungeonBuildStatusMessage StatusMessage = TaskInfo.Dungeon->GetBuildStatusMessage();
			TaskInfo.Widget->SetStatusMessage(StatusMessage);
			
			// Check if we have finished building
			if (!StatusMessage.bBuildInProgress) {
				DismissNotification(TaskInfo);
			}
		}
	}
}

void UDungeonArchitectNotificationSubsystem::DismissNotification(FDungeonArchitectNotificationTask& InTask) {
	InTask.Notification->SetCompletionState(SNotificationItem::CS_Success);
	InTask.Notification->Fadeout();
	InTask.bComplete = true;
}

#undef LOCTEXT_NAMESPACE

