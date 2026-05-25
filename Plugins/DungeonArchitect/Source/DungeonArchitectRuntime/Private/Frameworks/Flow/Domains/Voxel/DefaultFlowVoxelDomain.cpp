//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/Voxel/DefaultFlowVoxelDomain.h"

#include "Frameworks/FlowImpl/CellFlow/Voxel/Tasks/CellFlowVoxelTaskCreateDensity.h"

#define LOCTEXT_NAMESPACE "DefaultFlowVoxelDomain"

const FName FDefaultFlowVoxelDomain::DomainID = TEXT("DefaultFlowVoxel");

FName FDefaultFlowVoxelDomain::GetDomainID() const {
	return DomainID;
}

FText FDefaultFlowVoxelDomain::GetDomainDisplayName() const {
	return LOCTEXT("DomainDisplayNameDefaultVoxel", "Voxel");
}

void FDefaultFlowVoxelDomain::GetDomainTasks(TArray<UClass*>& OutTaskClasses) const {
	static const TArray<UClass*> DomainTasks = {
		UCellFlowVoxelTaskCreateDensity::StaticClass()
	};
	OutTaskClasses = DomainTasks;
}

#undef LOCTEXT_NAMESPACE


