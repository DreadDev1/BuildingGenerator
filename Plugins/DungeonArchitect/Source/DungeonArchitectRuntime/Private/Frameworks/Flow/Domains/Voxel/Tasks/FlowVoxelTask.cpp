//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/Voxel/Tasks/FlowVoxelTask.h"


#if WITH_EDITOR
FLinearColor UFlowVoxelTask::GetNodeColor() const {
	return FLinearColor(1.0f, 0.25f, 0);
}
#endif // WITH_EDITOR

