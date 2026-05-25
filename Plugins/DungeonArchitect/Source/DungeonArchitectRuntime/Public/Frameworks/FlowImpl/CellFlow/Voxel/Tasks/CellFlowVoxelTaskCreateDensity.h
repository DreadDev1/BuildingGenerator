//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/Voxel/Tasks/FlowVoxelTaskCreateDensity.h"
#include "CellFlowVoxelTaskCreateDensity.generated.h"

class UCellFlowLayoutGraph;
class UDAFlowCellGraph;

UCLASS(Meta = (VoxelTask, Title = "Carve Voxel Space", Tooltip = "Carve out a voxel space around the dungeon layout", MenuPriority = 1000))
class DUNGEONARCHITECTRUNTIME_API UCellFlowVoxelTaskCreateDensity : public UFlowVoxelTaskCreateDensity {
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	float DistortionDistance = 300;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	float DistortionScale = 2000;
	
	UPROPERTY(EditAnywhere, Category = "Noise Settings", meta=(UIMin="1", UIMax="30"))
	uint32 NumOctaves = 12;
	
public:
	virtual bool CarveVoxels(DA::VDB::FVoxelGrid& VoxelGrid, const FFlowTaskExecutionSettings& InExecSettings, const FFlowExecNodeStatePtr& InState) const override;

private:
	void CarveGridLayout(DA::VDB::FVoxelGrid& VoxelGrid, UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph, const FVector& InGridSize) const;

	void DebugCarveSphere(DA::VDB::FVoxelGrid& VoxelGrid) const;
};


