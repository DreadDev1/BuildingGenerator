//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTask.h"
#include "Frameworks/Lib/Async/DependencyGraph.h"

class ADungeon;
class UDungeonForgeNodeSettings;
struct FDungeonForgeGraphExecutionState;
struct FDungeonForgeContext;
typedef TSharedPtr<class IDungeonForgeElement, ESPMode::ThreadSafe> FDungeonForgeElementPtr;

class FDungeonForgeGraphTaskInstance : public FDADependencyGraphTask {
public:
	FDungeonForgeGraphTaskInstance(const FDungeonForgeGraphTask& InTask, const TWeakPtr<const FDungeonForgeGraphExecutionState>& InExecutionState);
	virtual void Run() override;
	virtual void Tick(float DeltaTime) override;

	FDungeonForgeContextPtr GetContext() const { return Context; }

private:
	void HandleComplete() const;
	
public:
	FDungeonForgeGraphTask Task;
	FDungeonForgeContextPtr Context;
	FDungeonForgeElementPtr Element;
	TWeakPtr<const FDungeonForgeGraphExecutionState> ExecutionState;
};

