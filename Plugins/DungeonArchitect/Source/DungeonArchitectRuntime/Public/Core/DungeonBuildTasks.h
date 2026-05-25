//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Dungeon.h"
#include "Core/DungeonModel.h"
#include "Frameworks/Lib/Async/DependencyGraph.h"

#include "Async/AsyncWork.h"

struct FDungeonBuildSettings;
class UDungeonCanvasComponent;
class UPCGComponent;
class UDungeonModel;
class ADungeon;
class UPCGGraph;
class FDungeonLayoutBuilderAsyncTask;
class FDungeonSceneProvider;
enum class EDungeonBuildGraphPhase : uint8;

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTaskBase : public FDADependencyGraphTask {
public:
	FDungeonBuildTaskBase(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings);
	virtual FText GetNotificationStatusText() const { return FText::GetEmpty(); }
	virtual FText GetNotificationStatusSubText() const { return FText::GetEmpty(); }
	virtual bool SupportsProgressBar() const { return false; }
	virtual int32 GetProgressTotalWorkUnits() const { return 0; }
	virtual int32 GetProgressCompletedWorkUnits() const { return 0; }
	
protected:
	TWeakObjectPtr<ADungeon> Dungeon;
	FDungeonBuildSettings BuildSettings;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTask_GenerateLayout : public FDungeonBuildTaskBase {
public:
	explicit FDungeonBuildTask_GenerateLayout(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings);

	virtual void Run() override;
	virtual void Abort() override;
	virtual void Tick(float DeltaTime) override;
	virtual FText GetNotificationStatusText() const override;

private:
	bool IsGenerationDone() const;
	void HandleCompleteTask() const;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTask_SetupPCG : public FDungeonBuildTaskBase {
public:
	FDungeonBuildTask_SetupPCG(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings);
	virtual void Run() override;
	virtual FText GetNotificationStatusText() const override;
	
private:
	TWeakObjectPtr<ADungeon> Dungeon;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTask_GeneratePCGGraph : public FDungeonBuildTaskBase {
public:
	explicit FDungeonBuildTask_GeneratePCGGraph(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings, UPCGGraph* InPCGGraph);
	virtual void Run() override;
	virtual void Tick(float DeltaTime) override;
	virtual FText GetNotificationStatusText() const override;
	virtual FText GetNotificationStatusSubText() const override;
	
private:
	void HandleGenerationComplete();
	
private:
	TWeakObjectPtr<UPCGGraph> PCGGraph;
	TWeakObjectPtr<UPCGComponent> PCGComponentPtr;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTask_SpawnSceneItems : public FDungeonBuildTaskBase {
public:
	explicit FDungeonBuildTask_SpawnSceneItems(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings);

	virtual void Run() override;
	virtual void Tick(float DeltaTime) override;
	virtual FText GetNotificationStatusText() const override;
	virtual FText GetNotificationStatusSubText() const override;
	
	virtual bool SupportsProgressBar() const override { return true; }
	virtual int32 GetProgressTotalWorkUnits() const override;
	virtual int32 GetProgressCompletedWorkUnits() const override;
	
	struct FDungeonState {
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<ADungeon> Dungeon;
		TWeakObjectPtr<UDungeonBuilder> Builder;
		TWeakObjectPtr<UDungeonConfig> Config;
		TWeakObjectPtr<UDungeonModel> Model;
		TArray<TObjectPtr<UDungeonThemeAsset>> Themes;
		FGuid ChunkTag;
		
		bool IsValid() const {
			return Builder.IsValid() && Config.IsValid() && Model.IsValid() && World.IsValid();
		}
		
	};
	void SetDungeonStateOverride(const FDungeonState& InStateOverride);

private:
	FDungeonState GetDungeonState();
	
private:
	TSharedPtr<FDungeonSceneProvider> SceneProvider;
	FDungeonState DungeonStateOverride;
	bool bUsesDungeonStateOverride{};
};

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTask_FinalizeBuild : public FDungeonBuildTaskBase {
public:
	explicit FDungeonBuildTask_FinalizeBuild(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings);

	virtual void Run() override;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonBuildTaskUtils {
public:
	static TSharedPtr<FDADependencyGraph> CreateDungeonActorBuildGraph(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings);
}; 

