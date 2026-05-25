//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Chunk/VoxelChunkDataCPU.h"


void FDAVoxelChunkDataCPU::Initialize(const FIntVector& InChunkCoord, const FIntVector& InVoxelMin, const FIntVector& InVoxelMax) {
	ChunkCoord = InChunkCoord;
	VoxelMin = InVoxelMin;
	VoxelMax = InVoxelMax;
	const int32 SizeX = InVoxelMax.X - InVoxelMin.X + 1;
	const int32 SizeY = InVoxelMax.Y - InVoxelMin.Y + 1;
	const int32 SizeZ = InVoxelMax.Z - InVoxelMin.Z + 1;
	const int32 TotalSize = SizeX * SizeY * SizeZ;
	DensityData.SetNum(TotalSize);
	MaterialData.SetNum(TotalSize);
	for (int32 i = 0; i < TotalSize; i++) {
		DensityData[i] = 1.0f;
		MaterialData[i] = 0;
	}
}
