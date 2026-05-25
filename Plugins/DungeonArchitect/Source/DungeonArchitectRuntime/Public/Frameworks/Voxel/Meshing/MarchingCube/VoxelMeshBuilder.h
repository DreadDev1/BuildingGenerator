//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonMeshGeometry.h"
#include "Frameworks/Voxel/VDB/VDBLib.h"

namespace DA {
	struct DUNGEONARCHITECTRUNTIME_API FVoxelCellCoords {
		FIntVector Coords[8];
	};

	struct DUNGEONARCHITECTRUNTIME_API FVoxelCellDensities {
		float Densities[8];
		bool bAllCornersActive;
	};
	
	namespace MarchingCubes {
		struct FSettings {
			double VoxelSize = 50;
			int32 LODLevel = 0;  // 0 = full res, 1 = half res, 2 = quarter res, etc.
			float UVScale = 100.0f;  // UV texture scale for box projection
		};

		typedef TFunction<bool(const FIntVector&, DA::VDB::FVoxelData&)> FVoxelDataGetter;
		typedef TFunction<void(const FIntVector&, DA::VDB::FVoxelData&, bool)> FVoxelDataSetter;
		
		class FMeshBuilder {
		public:
			// Primary functions using getter/setter pattern
			static void GenerateMeshBlock(const DA::MarchingCubes::FSettings& InSettings, const FIntVector& InCoordMin, const FIntVector& InCoordMax, const FVoxelDataGetter& Getter, FDungeonMeshGeometry& OutMeshData);
			static void GenerateMesh(const DA::MarchingCubes::FSettings& InSettings, const FVoxelDataGetter& Getter, const TArray<FIntVector>& InVoxelCoords, FDungeonMeshGeometry& OutMeshData);
			
			// Legacy overloads for backward compatibility
			static void GenerateMeshWorld(const DA::MarchingCubes::FSettings& InSettings, DA::VDB::FVoxelGrid& VoxelGrid, FDungeonMeshGeometry& OutMeshData);
			static void GenerateMeshBlock(const DA::MarchingCubes::FSettings& InSettings, const FIntVector& InCoordMin, const FIntVector& InCoordMax, DA::VDB::FVoxelGrid& VoxelGrid, FDungeonMeshGeometry& OutMeshData);
			static void GenerateMesh(const DA::MarchingCubes::FSettings& InSettings, DA::VDB::FVoxelGrid& VoxelGrid, const TArray<FIntVector>& InVoxelCoords, FDungeonMeshGeometry& OutMeshData);
		};
	}

	namespace SurfaceNets {
		struct FSettings {
			double VoxelSize = 50;
			int32 LODLevel = 0;
			float UVScale = 100.0f;  // UV texture scale for box projection
			FVector ChunkWorldOffset = FVector::ZeroVector;  // World position offset for seamless UV mapping
		};
	
		class FMeshBuilder {
		public:
			static void GenerateMeshWorld(const DA::SurfaceNets::FSettings& InSettings, DA::VDB::FVoxelGrid& VoxelGrid, FDungeonMeshGeometry& OutMeshData);
		};
	}
}

