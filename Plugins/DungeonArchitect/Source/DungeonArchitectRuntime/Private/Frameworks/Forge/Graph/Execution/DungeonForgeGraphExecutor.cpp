//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphExecutor.h"

#include "Core/Dungeon.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTaskInstance.h"
#include "Frameworks/Forge/Internal/Compiler/DungeonForgeGraphCompiler.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneCollector.h"

//////////////////////// FDungeonForgeOutputCollector ////////////////////////
void FDungeonForgeOutputCollector::RecordNodeOutput(const FGuid& InNodeGuid, const FDungeonForgeContext& InContext) {
	NodeOutputs.Add(InNodeGuid, InContext);
}

bool FDungeonForgeOutputCollector::HasOutputForNode(const FGuid& InNodeGuid) const {
	return NodeOutputs.Contains(InNodeGuid);
}

const FDungeonForgeContext* FDungeonForgeOutputCollector::GetNodeOutput(const FGuid& InNodeGuid) const {
	return NodeOutputs.Find(InNodeGuid);
}

FDungeonForgeContext* FDungeonForgeOutputCollector::GetNodeOutput(const FGuid& InNodeGuid) {
	return NodeOutputs.Find(InNodeGuid);
}

void FDungeonForgeOutputCollector::ClearAllOutputs() {
	NodeOutputs.Empty();
}


//////////////////////// FDungeonForgeGraphExecutionState ////////////////////////
FDungeonForgeContextPtr FDungeonForgeGraphExecutionState::GetTaskContext(FDungeonForgeTaskId InTaskId) const {
	if (const TSharedPtr<FDungeonForgeGraphTaskInstance>* SearchResult = TaskInstances.Find(InTaskId)) {
		const TSharedPtr<FDungeonForgeGraphTaskInstance>& TaskInstance = *SearchResult;
		if (TaskInstance.IsValid()) {
			return TaskInstance->GetContext();
		}
	}
	return nullptr;
}

FDungeonForgeContextPtr FDungeonForgeGraphExecutionState::GetTaskContext(const FGuid& InNodeGuid) const {
	for (const auto& Entry : TaskInstances) {
		TWeakPtr<FDungeonForgeGraphTaskInstance> TaskInstancePtr = Entry.Value;
		if (TaskInstancePtr.IsValid()) {
			TSharedPtr TaskInstance = TaskInstancePtr.Pin();
			if (TaskInstance.IsValid() && TaskInstance->Context->Node.IsValid()) {
				if (TaskInstance->Context->Node->NodeGuid == InNodeGuid) {
					return TaskInstance->Context;
				}
			}
		}
	}
	return nullptr;
}

//////////////////////// FDungeonForgeGraphExecutor ////////////////////////
void FDungeonForgeGraphExecutor::Execute(UDungeonForgeGraph* InForgeGraph, UDungeonForgeResourceManager* InResourceManager, ADungeon* InDungeon, FDungeonForgeOutputCollector* OutputCollector) {
	Dungeon = InDungeon;
	ForgeGraph = InForgeGraph;
	bRunning = true;

	ExecutionState = MakeShared<FDungeonForgeGraphExecutionState>();
	ExecutionState->Dungeon = InDungeon;
	ExecutionState->ChunkVisitedStates = MakeShared<FDungeonForgeGraphChunkState>();
	ExecutionState->ResourceManager = InResourceManager;
	ExecutionState->OutputCollector = OutputCollector;
	
	if (DependencyGraph.IsRunning()) {
		DependencyGraph.Abort();
	}
	
	if (InForgeGraph && InDungeon) {
		UWorld* World = InDungeon->GetWorld();
		TArray<FDungeonForgeGraphTask> CompiledTasks;
		FDungeonForgeGraphCompiler GraphCompiler(false);
		GraphCompiler.Compile(InForgeGraph, CompiledTasks);

		BuildDependencyGraph(CompiledTasks);
		DependencyGraph.Start(World);
	}

	if (!DependencyGraph.IsRunning()) {
		HandleGraphComplete();
	}
}

void FDungeonForgeGraphExecutor::Tick(float DeltaTime) {
	if (bRunning) {
		DependencyGraph.Tick(DeltaTime);
		if (!DependencyGraph.IsRunning()) {
			HandleGraphComplete();
		}
	}
}

void FDungeonForgeGraphExecutor::HandleGraphComplete() {
	// Destroy the left over chunks from the previous build
	if (Dungeon.IsValid() && ExecutionState.IsValid() && ExecutionState->ChunkVisitedStates.IsValid()) {
		FDungeonSceneCollector SceneCollector;
		SceneCollector.Collect(Dungeon.Get());
		for (const FGuid& VisitedChunk : ExecutionState->ChunkVisitedStates->GetVisitedChunks()) {
			SceneCollector.DeregisterChunk(VisitedChunk);
		}
		FDungeonSceneProviderCommandList DestroyCommands;
		SceneCollector.DestroyUnusedActors(DestroyCommands);
		UWorld* World = Dungeon->GetWorld();
		for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : DestroyCommands.GetCommands()) {
			if (Command.IsValid()) {
				Command->Execute(World);
			}
		}

		// Destroy the world, if this is a game world. We want the textures to linger around for the editor debug view to work
		bool bGameWorld = World->IsGameWorld();
		if (bGameWorld && ExecutionState->ResourceManager.IsValid()) {
			ExecutionState->ResourceManager->ReleaseResources();
		}
	}
	
	bRunning = false;
}

void FDungeonForgeGraphExecutor::Abort() {
	DependencyGraph.Abort();
}

bool FDungeonForgeGraphExecutor::IsRunning() const {
	return bRunning;
}

FDungeonForgeContextPtr FDungeonForgeGraphExecutor::GetTaskContext(const FGuid& InNodeGuid) const {
	return ExecutionState->GetTaskContext(InNodeGuid);
}

void FDungeonForgeGraphExecutor::BuildDependencyGraph(const TArray<FDungeonForgeGraphTask>& InTasks) {
	DependencyGraph.Clear();
	
	// Create the task instances
	TMap<FDungeonForgeTaskId, TSharedPtr<FDungeonForgeGraphTaskInstance>> TaskInstanceMap;
	for (const FDungeonForgeGraphTask& Task : InTasks) {
		check(Task.Node && Task.Node->GetSettings());
		TSharedPtr<FDungeonForgeGraphTaskInstance> TaskInstance = MakeShared<FDungeonForgeGraphTaskInstance>(Task, ExecutionState);
		DependencyGraph.RegisterTask(TaskInstance);
		TaskInstanceMap.Add(Task.NodeId, TaskInstance);
	}

	// Add the dependencies
	for (const FDungeonForgeGraphTask& Task : InTasks) {
		TSharedPtr<FDungeonForgeGraphTaskInstance> TaskInstance = TaskInstanceMap.FindChecked(Task.NodeId);
		check(TaskInstance.IsValid());

		for (const FDungeonForgeGraphTaskInput& Input : Task.Inputs) {
			if (TaskInstanceMap.Contains(Input.TaskId)) {
				TSharedPtr<FDungeonForgeGraphTaskInstance> InputTaskInstance = TaskInstanceMap.FindChecked(Input.TaskId);
				TaskInstance->AddDependency(InputTaskInstance);
			}
		}
	}

	for (const auto& Entry : TaskInstanceMap) {
		ExecutionState->TaskInstances.Add(Entry.Key, Entry.Value);
	}
}

