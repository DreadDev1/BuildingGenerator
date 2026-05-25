//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTask.h"

struct FDungeonForgeGraphTask;
class UDungeonForgeGraph;

class DUNGEONARCHITECTRUNTIME_API FDungeonForgeGraphCompiler {
public:
	FDungeonForgeGraphCompiler(bool bInIsCooking);
	void Compile(UDungeonForgeGraph* InGraph, TArray<FDungeonForgeGraphTask>& OutCompiledTasks);

private:
	TArray<FDungeonForgeGraphTask> CompileGraph(UDungeonForgeGraph* InGraph);
	
private:
	bool bIsCooking{}; 
};

