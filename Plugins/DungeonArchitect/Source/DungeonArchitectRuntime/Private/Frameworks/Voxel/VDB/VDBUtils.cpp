//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/VDB/VDBUtils.h"

#include "Core/Utils/DungeonRenderUtils.h"
#include "Frameworks/Voxel/Meshing/MarchingCube/TransVoxelLookup.h"

void DA::VDB::FVoxelUtils::GatherChunks(const FVoxelGrid& InGrid, int32 InChunkVoxelSize, TArray<FVoxelChunkDescriptor>& OutChunks) {
	OutChunks.Reset();
	
	if (InChunkVoxelSize <= 0) {
		return;
	}
	
	TMap<FIntVector, int32> ChunkIndexMap;
	
	auto ActiveIterator = InGrid.GetActiveIterator();
	for (; ActiveIterator.IsValid(); ++ActiveIterator) {
		FIntVector VoxelCoord = ActiveIterator.GetCoordinate();
		
		auto DivFloor = [](int32 Value, int32 Divisor) -> int32 {
			return FMath::FloorToInt(static_cast<float>(Value) / static_cast<float>(Divisor));
		};
		
		FIntVector ChunkCoord = FIntVector(
			DivFloor(VoxelCoord.X, InChunkVoxelSize),
			DivFloor(VoxelCoord.Y, InChunkVoxelSize),
			DivFloor(VoxelCoord.Z, InChunkVoxelSize)
		);
		
		int32 ChunkIndex = -1;
		if (int32* ExistingChunkIndex = ChunkIndexMap.Find(ChunkCoord)) {
			ChunkIndex = *ExistingChunkIndex;
		} else {
			ChunkIndex = OutChunks.Add(FVoxelChunkDescriptor());
			ChunkIndexMap.Add(ChunkCoord, ChunkIndex);
			
			FVoxelChunkDescriptor& NewChunk = OutChunks[ChunkIndex];
			NewChunk.CoordMin = ChunkCoord * InChunkVoxelSize;
			NewChunk.CoordMax = NewChunk.CoordMin + FIntVector(InChunkVoxelSize - 1);
		}
		
		OutChunks[ChunkIndex].Voxels.Add(VoxelCoord);
	}
}

void DA::VDB::FVoxelUtils::MeshChunk(const FVoxelChunkDescriptor& InChunk, FVoxelGrid& InVoxelGrid, const DA::MarchingCubes::FSettings& InSettings, FDungeonMeshGeometry& OutMeshData) {
	DA::MarchingCubes::FSettings MeshSettings;
	MeshSettings.VoxelSize = InSettings.VoxelSize;
	MeshSettings.LODLevel = InSettings.LODLevel;
	DA::MarchingCubes::FMeshBuilder::GenerateMesh(MeshSettings, InVoxelGrid, InChunk.Voxels, OutMeshData);
}

void DA::VDB::FVoxelUtils::MeshChunks(const TArray<FVoxelChunkDescriptor>& InChunks, FVoxelGrid& InVoxelGrid, const DA::MarchingCubes::FSettings& InSettings, TArray<FDungeonMeshGeometry>& OutMeshes) {
	OutMeshes.Reset(InChunks.Num());
	
	for (const FVoxelChunkDescriptor& Chunk : InChunks) {
		FDungeonMeshGeometry& ChunkMesh = OutMeshes.AddDefaulted_GetRef();
		MeshChunk(Chunk, InVoxelGrid, InSettings, ChunkMesh);
	}
}

void DA::VDB::FVoxelUtils::MeshWorld(FVoxelGrid& InVoxelGrid, const DA::MarchingCubes::FSettings& InSettings, FDungeonMeshGeometry& OutMeshData) {
	DA::MarchingCubes::FSettings MeshSettings;
	MeshSettings.VoxelSize = InSettings.VoxelSize;
	MeshSettings.LODLevel = InSettings.LODLevel;
	DA::MarchingCubes::FMeshBuilder::GenerateMeshWorld(MeshSettings, InVoxelGrid, OutMeshData);
}

