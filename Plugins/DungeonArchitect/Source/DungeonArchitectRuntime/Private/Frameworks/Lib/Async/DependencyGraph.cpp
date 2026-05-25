//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Lib/Async/DependencyGraph.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogDADependencyGraph, Log, All);

void FDADependencyGraphTask::AddDependency(const TSharedPtr<FDADependencyGraphTask>& InDependency) {
	if (InDependency.IsValid()) {
		DependentTasks.Add(InDependency);
	}
}

bool FDADependencyGraphTask::CanStart() const {
	if (DependentTasks.IsEmpty()) {
		return true;
	}
	
	for (TWeakPtr<FDADependencyGraphTask> DependentTaskPtr : DependentTasks) {
		TSharedPtr<FDADependencyGraphTask> DependentTask = DependentTaskPtr.Pin();
		if (DependentTask.IsValid() && !DependentTask->IsComplete()) {
			return false;
		}
	}
	return true;
}

void FDADependencyGraphTask::RequestAbortGraph() const {
	if (Graph) {
		Graph->Abort();
	}
}

void FDADependencyGraph::Abort() {
	for (TSharedPtr<FDADependencyGraphTask> Task : Tasks) {
		if (Task) {
			Task->Abort();
			Task->State = EDADependencyGraphTaskState::Complete;
		}
	}
	Tasks.Reset();
}

void FDADependencyGraph::Clear() {
	check(!IsRunning());
	Tasks.Reset();
	WeakWorld = nullptr;
}

bool FDADependencyGraph::IsRunning() const {
	for (const TSharedPtr<FDADependencyGraphTask>& Task : Tasks) {
		if (Task.IsValid() && Task->State != EDADependencyGraphTaskState::Complete) {
			return true;
		}
	}
	return false;
}

TStatId FDADependencyGraph::GetStatId() const {
	return TStatId();
}

void FDADependencyGraph::GetTasks(EDADependencyGraphTaskState InState, TArray<TSharedPtr<FDADependencyGraphTask>>& OutTasks) const {
	for (TSharedPtr<FDADependencyGraphTask> Task : Tasks) {
		if (Task->State == InState) {
			OutTasks.Add(Task);
		}
	} 
}

void FDADependencyGraph::Tick(float DeltaTime) {
	// Check if we can start a pending task in this tick frame
	for (int TaskIndex = 0; TaskIndex < Tasks.Num(); TaskIndex++) {
		const TSharedPtr<FDADependencyGraphTask>& Task = Tasks[TaskIndex];
		if (Task.IsValid() && Task->State == EDADependencyGraphTaskState::Waiting && Task->CanStart()) {
			Task->State = EDADependencyGraphTaskState::Running;
			Task->Run();
		}
	}
	
	// Tick the running tasks
	for (int TaskIndex = 0; TaskIndex < Tasks.Num(); TaskIndex++) {
		const TSharedPtr<FDADependencyGraphTask>& Task = Tasks[TaskIndex];
		if (Task.IsValid()) {
			if (Task->GetState() == EDADependencyGraphTaskState::Running) {
				Task->Tick(DeltaTime);
			}
		}
	}

	// Filter out the completed tasks
	Tasks = Tasks.FilterByPredicate([](const TSharedPtr<FDADependencyGraphTask>& InTask) {
		return InTask.IsValid() && !InTask->IsComplete();
	});
}

UWorld* FDADependencyGraph::GetTickableGameObjectWorld() const {
	return WeakWorld.Get();
}

ETickableTickType FDADependencyGraph::GetTickableTickType() const {
	return ETickableTickType::Always;
}

void FDADependencyGraph::Start(UWorld* InWorld) {
	WeakWorld = InWorld;
	Tick(0);
}

void FDADependencyGraph::RegisterTask(const TSharedPtr<FDADependencyGraphTask>& InTask) {
	InTask->Graph = this;
	Tasks.Add(InTask);
}

