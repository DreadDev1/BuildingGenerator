//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/Tasks/SelectFlowExecTask.h"


USelectFlowExecTask::USelectFlowExecTask() {
	InputConstraint = EFlowExecTaskInputConstraint::MultiInput;
}

void USelectFlowExecTask::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
	check(Input.IncomingNodeOutputs.Num() == 1);
	Output.State = Input.IncomingNodeOutputs[0].State->Clone();
	Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

TArray<int32> USelectFlowExecTask::SelectIncomingNodeBranches(const FRandomStream& InRandom, const TArray<UFlowExecScriptGraphNode*> IncomingNodes) {
	return {InRandom.RandRange(0, IncomingNodes.Num() - 1)};
}

