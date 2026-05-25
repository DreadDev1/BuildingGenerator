//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTaskInstance.h"

#include "Core/Dungeon.h"
#include "Core/Utils/Debug/DungeonDebugVisualizer.h"
#include "Frameworks/Forge/Common/Utils/DungeonForgeDebugUtils.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphExecutor.h"

FDungeonForgeGraphTaskInstance::FDungeonForgeGraphTaskInstance(const FDungeonForgeGraphTask& InTask, const TWeakPtr<const FDungeonForgeGraphExecutionState>& InExecutionState)
	: Task(InTask)
	, ExecutionState(InExecutionState)
{
}

void FDungeonForgeGraphTaskInstance::Run() {
	check(State == EDADependencyGraphTaskState::Running);

	bool bRunning{};
	if (!Element.IsValid()) {
		if (UDungeonForgeNodeSettings* NodeSettings = Task.Node ? Task.Node->GetSettings() : nullptr) {
			Element = NodeSettings->CreateElement();
			FDungeonForgeDataCollection TaskInputData;
			for (const FDungeonForgeGraphTaskInput& Input : Task.Inputs) {
				if (ExecutionState.IsValid()) {
					TSharedPtr<const FDungeonForgeGraphExecutionState> ExecutionStatePtr = ExecutionState.Pin();
					if (FDungeonForgeContextPtr InputTaskContext = ExecutionStatePtr->GetTaskContext(Input.TaskId)) {
						TArray<FDungeonForgeTaggedData> UpstreamPinDataList = InputTaskContext->OutputData.GetInputsByPin(Input.UpstreamPin->Label);
						for (const FDungeonForgeTaggedData& UpstreamPinData : UpstreamPinDataList) {
							FDungeonForgeTaggedData DownstreamPinData = UpstreamPinData;
							DownstreamPinData.Pin = Input.DownstreamPin->Label;
							TaskInputData.AddData(DownstreamPinData);
						}
					}
				}
			}
			Context = Element->Initialize(TaskInputData, Task.Node, ExecutionState);
		}
	}
	if (Element.IsValid()) {
		bool bElementComplete = Element->Execute(Context);
		bRunning = !bElementComplete;
	}
	else {
		bRunning = false;
	}

	State = bRunning
		? EDADependencyGraphTaskState::Running
		: EDADependencyGraphTaskState::Complete;

	if (State == EDADependencyGraphTaskState::Complete) {
		HandleComplete();
	}
}

void FDungeonForgeGraphTaskInstance::Tick(float DeltaTime) {
	FDADependencyGraphTask::Tick(DeltaTime);

	if (State == EDADependencyGraphTaskState::Running) {
		bool bRunning{};
		if (Element.IsValid()) {
			const bool bComplete = Element->Tick(Context, DeltaTime);
			bRunning = !bComplete;
		}
		if (!bRunning) {
			State = EDADependencyGraphTaskState::Complete;
		}
		
		if (State == EDADependencyGraphTaskState::Complete) {
			HandleComplete();
		}
	}
}

void FDungeonForgeGraphTaskInstance::HandleComplete() const {
	if (Context) {
		TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
		if (ExecState.IsValid() && ExecState->OutputCollector && IsValid(Task.Node) && Context.IsValid()) {
			ExecState->OutputCollector->RecordNodeOutput(Task.Node->NodeGuid, *Context);

			// Build the debug visualization
#if WITH_EDITOR
			FDungeonForgeDebugUtils::UpdateNodeDebugVisualization(Context.Get());
#endif // WITH_EDITOR
		}
	}
}

