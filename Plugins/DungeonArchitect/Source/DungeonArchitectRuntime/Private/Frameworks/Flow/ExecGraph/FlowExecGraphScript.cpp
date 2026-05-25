//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/FlowExecGraphScript.h"


EFlowExecTaskInputConstraint UFlowExecScriptGraphNode::GetInputConstraint() const {
	return EFlowExecTaskInputConstraint::SingleInput;
}

TArray<int32> UFlowExecScriptGraphNode::SelectIncomingNodeBranches(const FRandomStream& InRandom) const {
	TArray<int32> Result;
	for (int i = 0; i < IncomingNodes.Num(); i++) {
		Result.Add(i);
	}
	return Result;
}

EFlowExecTaskInputConstraint UFlowExecScriptTaskNode::GetInputConstraint() const {
	return Task ? Task->GetInputConstraint() : Super::GetInputConstraint();
}

TArray<int32> UFlowExecScriptTaskNode::SelectIncomingNodeBranches(const FRandomStream& InRandom) const {
	return Task ? Task->SelectIncomingNodeBranches(InRandom, IncomingNodes) : Super::SelectIncomingNodeBranches(InRandom);
}

