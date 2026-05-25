//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTask.h"

#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#if WITH_EDITOR
bool FDungeonForgeGraphTaskInput::operator==(const FDungeonForgeGraphTaskInput& Other) const {
	return (TaskId == Other.TaskId)
		&& (UpstreamPin == Other.UpstreamPin)
		&& (DownstreamPin == Other.DownstreamPin)
		&& (bProvideData == Other.bProvideData);
}

void FDungeonForgeGraphTask::PrepareForCook() {
	check(Node);
	NodePtr = Node;
}

bool FDungeonForgeGraphTask::CanExecuteOnlyOnMainThread() const {
	// TODO: Handle me
	return true;
}

#else // WITH_EDITOR
void FDungeonForgeGraphTask::LoadCookedData() {
	check(NodePtr.Get());
	Node = NodePtr.Get();
}
#endif // WITH_EDITOR

