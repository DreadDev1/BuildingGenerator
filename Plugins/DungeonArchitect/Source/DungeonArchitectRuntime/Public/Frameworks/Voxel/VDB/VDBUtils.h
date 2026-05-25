//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonMeshGeometry.h"
#include "Frameworks/Voxel/Meshing/MarchingCube/VoxelMeshBuilder.h"
#include "Frameworks/Voxel/VDB/VDBLib.h"

namespace DA::VDB {
	struct FVoxelChunkDescriptor {
		FIntVector CoordMin;
		FIntVector CoordMax;
		TArray<FIntVector> Voxels;
	};
	
	class DUNGEONARCHITECTRUNTIME_API FVoxelUtils {
	public:
		static void GatherChunks(const FVoxelGrid& InGrid, int32 InChunkVoxelSize, TArray<FVoxelChunkDescriptor>& OutChunks);
		static void MeshChunk(const FVoxelChunkDescriptor& InChunk, FVoxelGrid& InVoxelGrid, const DA::MarchingCubes::FSettings& InSettings, FDungeonMeshGeometry& OutMeshData);
		static void MeshChunks(const TArray<FVoxelChunkDescriptor>& InChunks, FVoxelGrid& InVoxelGrid, const DA::MarchingCubes::FSettings& InSettings, TArray<FDungeonMeshGeometry>& OutMeshes);
		
		static void MeshWorld(FVoxelGrid& InVoxelGrid, const DA::MarchingCubes::FSettings& InSettings, FDungeonMeshGeometry& OutMeshData);

	};
}

