//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphExecutor.h"

#include "Tickable.h"
#include "DungeonForgeBuilder.generated.h"

struct FDungeonForgeContext;

UCLASS(EarlyAccessPreview, Meta=(DisplayName="Dungeon Forge", Description="A node-based system that unifies and merges multiple Dungeon Architect builders into a single workflow, allowing you to mix and match different generation approaches seamlessly"))
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeBuilder : public UDungeonBuilder, public FTickableGameObject {
	GENERATED_BODY()
public:
	virtual void BuildDungeonImpl(UWorld* World) override;
	virtual void DestroyDungeonImpl(UWorld* World) override;
	
	virtual bool CanBuildDungeon(FString& OutMessage) override;
    virtual FDungeonBuilderCapabilities GetCapabilities() override;
	
	virtual TSubclassOf<UDungeonModel> GetModelClass() override;
	virtual TSubclassOf<UDungeonConfig> GetConfigClass() override;
	virtual TSubclassOf<UDungeonQuery> GetQueryClass() override;
	virtual TSubclassOf<UDungeonToolData> GetToolDataClass() override;

	virtual bool IsGenerating() const override;
	virtual void AbortGeneration() override;
	
	//~ Begin FTickableGameObject
	virtual TStatId GetStatId() const override;
	virtual void Tick(float InDeltaSeconds) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	//~ End FTickableGameObject

	const FDungeonForgeContext* GetNodeInstanceContext(const FGuid& InNodeGuid) const;
	FDungeonForgeContext* GetNodeInstanceContext(const FGuid& InNodeGuid);
	
private:
	FDungeonForgeGraphExecutor GraphExecutor;

	UPROPERTY(Transient)
	FDungeonForgeOutputCollector GraphOutputCollector;

	UPROPERTY()
	TObjectPtr<UDungeonForgeResourceManager> ResourceManager;
};

