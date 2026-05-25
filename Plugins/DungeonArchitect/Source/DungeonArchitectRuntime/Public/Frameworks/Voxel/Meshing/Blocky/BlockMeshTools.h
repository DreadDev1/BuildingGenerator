//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/VDB/VDBLib.h"

namespace UE {
	namespace Geometry {
		class FDynamicMesh3;
	}
}

struct FDAVoxelMeshGenSettings {
	double VoxelSize = {};
	FIntVector CoordMin = {};
	FIntVector CoordMax = {};
	bool bOptimizeMesh = {};
	int32 Seed = 0;
};


class FDAVoxelMeshGenerator {
public:
	static void GenerateBlockMesh(UE::Geometry::FDynamicMesh3& Mesh, DA::VDB::FVoxelGrid& VoxelGrid, const FDAVoxelMeshGenSettings& InSettings);
};

