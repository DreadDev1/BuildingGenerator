//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Tickable.h"
#include "DungeonArchitectNotificationSubsystem.generated.h"

class ADungeon;
class SDABuildProgressNotification;

/** Data payload for progress bars to handle an active progress notification */
struct FDungeonArchitectNotificationTask {
	TSharedPtr<SDABuildProgressNotification, ESPMode::ThreadSafe> Widget;
	TSharedPtr<SNotificationItem, ESPMode::ThreadSafe> Notification;
	TWeakObjectPtr<ADungeon> Dungeon;
	bool bComplete{};	
};

	
UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonArchitectNotificationSubsystem
	: public UEditorSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/** Static convenience method to return storm sync notification subsystem */
	static UDungeonArchitectNotificationSubsystem& Get();
	
	TSharedPtr<SNotificationItem> AddSimpleNotification(const FText& InNotificationText);
	TSharedPtr<SNotificationItem> AddSimpleNotification(const FNotificationInfo& InInfo);

	void OnDungeonBuildStarted(ADungeon* InDungeon);
	
	//~ Begin FTickableGameObject interface
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual bool IsTickableInEditor() const { return true; }
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsAllowedToTick() const override;
	virtual void Tick(float DeltaTime) override;
	TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDungeonArchitectNotificationSubsystem, STATGROUP_Tickables); }
	//~ End FTickableGameObject interface

private:
	static void DismissNotification(FDungeonArchitectNotificationTask& InTask);
	
protected:
	TArray<FDungeonArchitectNotificationTask> BuildStatusNotifications;
};

