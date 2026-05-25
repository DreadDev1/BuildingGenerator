//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeCommon.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Lib/Async/DependencyGraph.h"
#include "DungeonForgeGraphExecutor.generated.h"

class UDungeonForgeResourceManager;
class ADungeon;
class UDungeonForgeGraph;
class FDungeonForgeGraphTaskInstance;
struct FDungeonForgeGraphTask;

USTRUCT()
struct FDungeonForgeOutputCollector {
	GENERATED_BODY()
public:
	void RecordNodeOutput(const FGuid& InNodeGuid, const FDungeonForgeContext& InContext);
	bool HasOutputForNode(const FGuid& InNodeGuid) const;
	const FDungeonForgeContext* GetNodeOutput(const FGuid& InNodeGuid) const;
	FDungeonForgeContext* GetNodeOutput(const FGuid& InNodeGuid);
	void ClearAllOutputs();

private:
	// Map of node GUIDs to their output contexts
	UPROPERTY()
	TMap<FGuid, FDungeonForgeContext> NodeOutputs;
};

class FDungeonForgeGraphChunkState {
public:
	void MarkAsVisited(const FGuid& InChunkId) {
		VisitedChunks.Add(InChunkId);
	}
	TSet<FGuid> GetVisitedChunks() const { return VisitedChunks; }
	
private:
	TSet<FGuid> VisitedChunks;
};

struct FDungeonForgeGraphExecutionState {
	TWeakObjectPtr<ADungeon> Dungeon;
	TWeakObjectPtr<UDungeonForgeResourceManager> ResourceManager;
	FDungeonForgeOutputCollector* OutputCollector{};
	
	TMap<FDungeonForgeTaskId, TSharedPtr<FDungeonForgeGraphTaskInstance>> TaskInstances;
	TSharedPtr<FDungeonForgeGraphChunkState> ChunkVisitedStates;
	
	FDungeonForgeContextPtr GetTaskContext(FDungeonForgeTaskId InTaskId) const;
	FDungeonForgeContextPtr GetTaskContext(const FGuid& InNodeGuid) const;
}; 


class FDungeonForgeGraphExecutor {
public:
	void Execute(UDungeonForgeGraph* InForgeGraph, UDungeonForgeResourceManager* InResourceManager, ADungeon* InDungeon, FDungeonForgeOutputCollector* OutputCollector = nullptr);
	void Tick(float DeltaTime);
	void Abort();
	bool IsRunning() const;
	
	FDungeonForgeContextPtr GetTaskContext(const FGuid& InNodeGuid) const;
	
private:
	void BuildDependencyGraph(const TArray<FDungeonForgeGraphTask>& InTasks);
	void HandleGraphComplete();

private:
	FDADependencyGraph DependencyGraph;
	TWeakObjectPtr<ADungeon> Dungeon;
	TObjectPtr<UDungeonForgeGraph> ForgeGraph;
	TSharedPtr<FDungeonForgeGraphExecutionState> ExecutionState;
	bool bRunning = false;
};

