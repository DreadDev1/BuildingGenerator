//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/Voxel/Tasks/FlowVoxelTask.h"
#include "Frameworks/Voxel/Meshing/VoxelMeshGenerationSettings.h"
#include "FlowVoxelTaskCreateDensity.generated.h"

UCLASS(Meta = (VoxelTask, Title = "Carve Voxel Space", Tooltip = "Carve out a voxel space around the dungeon layout", MenuPriority = 1000))
class DUNGEONARCHITECTRUNTIME_API UFlowVoxelTaskCreateDensity : public UFlowVoxelTask {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Voxel")
	FDAVoxelMeshGenerationSettings VoxelMeshSettings;

public:
	virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const override;

protected:
	virtual bool CarveVoxels(DA::VDB::FVoxelGrid& VoxelGrid, const FFlowTaskExecutionSettings& InExecSettings, const FFlowExecNodeStatePtr& InState) const { return true; }
};


