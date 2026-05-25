//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/Voxel/FlowVoxelState.h"


const FName UFlowVoxelState::StateTypeID = TEXT("FlowVoxelState");

void UFlowVoxelState::CloneFromStateObject(const UObject* SourceObject) {
	if (const UFlowVoxelState* OtherVoxelState = Cast<UFlowVoxelState>(SourceObject)) {
		VoxelMeshSettings = OtherVoxelState->VoxelMeshSettings;
	}
}
