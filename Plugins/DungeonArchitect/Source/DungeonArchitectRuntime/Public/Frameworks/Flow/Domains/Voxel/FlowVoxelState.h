//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "Frameworks/Voxel/Meshing/VoxelMeshGenerationSettings.h"
#include "Frameworks/Voxel/VDB/VDBLib.h"
#include "FlowVoxelState.generated.h"

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowVoxelState : public UObject, public IFlowExecCloneableState {
	GENERATED_BODY()
public:
	static const FName StateTypeID;
	
	UPROPERTY()
	FDAVoxelMeshGenerationSettings VoxelMeshSettings;
	
	UPROPERTY()
	bool bVoxelGenerationRequested = false;

public:
	virtual void CloneFromStateObject(const UObject* SourceObject) override;
};

