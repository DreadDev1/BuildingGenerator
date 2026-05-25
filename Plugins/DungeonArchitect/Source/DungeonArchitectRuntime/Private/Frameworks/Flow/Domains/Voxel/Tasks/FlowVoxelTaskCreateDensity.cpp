//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/Voxel/Tasks/FlowVoxelTaskCreateDensity.h"

#include "Frameworks/Flow/Domains/Voxel/FlowVoxelState.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"

void UFlowVoxelTaskCreateDensity::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
	check(Input.IncomingNodeOutputs.Num() == 1);
	Output.State = Input.IncomingNodeOutputs[0].State->Clone();

	UFlowVoxelState* VoxelState = NewObject<UFlowVoxelState>();
	VoxelState->VoxelMeshSettings = VoxelMeshSettings;
	VoxelState->bVoxelGenerationRequested = true;
	Output.State->SetStateObject(UFlowVoxelState::StateTypeID, VoxelState);

	/*
	DA::VDB::FVoxelGrid& VoxelGrid = *VoxelState->VoxelGrid;
	if (!CarveVoxels(VoxelGrid, InExecSettings, Output.State)) {
		Output.ErrorMessage = "Invalid Input";
		Output.ExecutionResult = EFlowTaskExecutionResult::FailHalt;
		return;
	}
	*/
	
	Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

