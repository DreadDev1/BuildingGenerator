//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Meshing/MarchingCube/VoxelMeshBuilder.h"

#include "Core/Utils/DungeonRenderUtils.h"
#include "Frameworks/Voxel/Meshing/MarchingCube/TransVoxelLookup.h"

namespace FDAMarchingCubeMeshUtils {
	FORCEINLINE void GetCellCoords(const FIntVector& InGridCoord, DA::FVoxelCellCoords& OutCellCoords)
	{
		// LOD Increment
		const int32 X = InGridCoord.X;
		const int32 Y = InGridCoord.Y;
		const int32 Z = InGridCoord.Z;
		constexpr int32 I = 1;

		OutCellCoords.Coords[0] = FIntVector(X + 0, Y + 0, Z + 0);
		OutCellCoords.Coords[1] = FIntVector(X + I, Y + 0, Z + 0);
		OutCellCoords.Coords[2] = FIntVector(X + 0, Y + I, Z + 0);
		OutCellCoords.Coords[3] = FIntVector(X + I, Y + I, Z + 0);
		OutCellCoords.Coords[4] = FIntVector(X + 0, Y + 0, Z + I);
		OutCellCoords.Coords[5] = FIntVector(X + I, Y + 0, Z + I);
		OutCellCoords.Coords[6] = FIntVector(X + 0, Y + I, Z + I);
		OutCellCoords.Coords[7] = FIntVector(X + I, Y + I, Z + I);
	}

	// New version using getter
	FORCEINLINE float GetSDFValue(const DA::MarchingCubes::FVoxelDataGetter& Getter, const FIntVector& InGridCoord) {
		DA::VDB::FVoxelData VoxelData{};
		if (Getter(InGridCoord, VoxelData)) {
			return VoxelData.SDF;
		}
		return 100.0f;
	}
	
	// Legacy version for backward compatibility
	FORCEINLINE float GetSDFValue(const DA::VDB::FVoxelGrid& VoxelGrid, const FIntVector& InGridCoord) {
		auto Getter = [&VoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
			return VoxelGrid.Get(Coord, OutData);
		};
		return GetSDFValue(Getter, InGridCoord);
	}

	// Get material at a voxel coordinate
	FORCEINLINE uint8 GetMaterialValue(const DA::MarchingCubes::FVoxelDataGetter& Getter, const FIntVector& InGridCoord) {
		DA::VDB::FVoxelData VoxelData{};
		if (Getter(InGridCoord, VoxelData)) {
			return VoxelData.Material;
		}
		return 0;
	}
	
	// Structure to hold both densities and materials for a cell
	struct FVoxelCellData {
		float Densities[8];
		uint8 Materials[8];
		bool bAllCornersActive;
	};
	
	// New version using getter - extended to get materials too
	FORCEINLINE void GetCellData(const DA::MarchingCubes::FVoxelDataGetter& Getter, const FIntVector& InGridCoord, FVoxelCellData& OutCellData)
	{
		static const TArray<FIntVector> LocalCornerOffsets = {
			FIntVector(0, 0, 0),
			FIntVector(1, 0, 0),
			FIntVector(0, 1, 0),
			FIntVector(1, 1, 0),
			FIntVector(0, 0, 1),
			FIntVector(1, 0, 1),
			FIntVector(0, 1, 1),
			FIntVector(1, 1, 1)
		};
		
		OutCellData.bAllCornersActive = true;
		for (int i = 0; i < 8; i++) {
			const FIntVector I = InGridCoord + LocalCornerOffsets[i];
			DA::VDB::FVoxelData VoxelData{};
			if (Getter(I, VoxelData)) {
				OutCellData.Densities[i] = VoxelData.SDF;
				OutCellData.Materials[i] = VoxelData.Material;
			} else {
				OutCellData.Densities[i] = 100.0f;
				OutCellData.Materials[i] = 0;
				OutCellData.bAllCornersActive = false;
			}
		}
	}
	
	// New version using getter
	FORCEINLINE void GetCellDensities(const DA::MarchingCubes::FVoxelDataGetter& Getter, const FIntVector& InGridCoord, DA::FVoxelCellDensities& OutCellDensities)
	{
		static const TArray<FIntVector> LocalCornerOffsets = {
			FIntVector(0, 0, 0),
			FIntVector(1, 0, 0),
			FIntVector(0, 1, 0),
			FIntVector(1, 1, 0),
			FIntVector(0, 0, 1),
			FIntVector(1, 0, 1),
			FIntVector(0, 1, 1),
			FIntVector(1, 1, 1)
		};
		
		OutCellDensities.bAllCornersActive = true;
		for (int i = 0; i < 8; i++) {
			const FIntVector I = InGridCoord + LocalCornerOffsets[i];
			DA::VDB::FVoxelData VoxelData{};
			if (Getter(I, VoxelData)) {
				OutCellDensities.Densities[i] = VoxelData.SDF;
			} else {
				OutCellDensities.Densities[i] = 100.0f;
				OutCellDensities.bAllCornersActive = false;
			}
		}
	}
	
	// Legacy version for backward compatibility
	FORCEINLINE void GetCellDensities(DA::VDB::FVoxelGrid& VoxelGrid, const FIntVector& InGridCoord, DA::FVoxelCellDensities& OutCellDensities)
	{
		auto Getter = [&VoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
			return VoxelGrid.Get(Coord, OutData);
		};
		GetCellDensities(Getter, InGridCoord, OutCellDensities);
	}


	FORCEINLINE void GetCutPoint(float D0, float D1, const FIntVector& Coord0, const FIntVector& Coord1, float VoxelSize, FVector& OutP, float& OutT)
	{
		const FVector P0 = FVector(Coord0.X, Coord0.Y, Coord0.Z) * VoxelSize;
		const FVector P1 = FVector(Coord1.X, Coord1.Y, Coord1.Z) * VoxelSize;
		OutT = D1 / (D1 - D0);
		OutP = P0 * OutT + P1 * (1 - OutT);
	}

	// New version using getter
	float Sample(const DA::MarchingCubes::FVoxelDataGetter& Getter, const FVector& InGridCoord) {
		int32 X0, Y0, Z0, X1, Y1, Z1;
		float DX, DY, DZ;
		{
			X0 = FMath::FloorToInt(InGridCoord.X);
			Y0 = FMath::FloorToInt(InGridCoord.Y);
			Z0 = FMath::FloorToInt(InGridCoord.Z);

			X1 = X0 + 1;
			Y1 = Y0 + 1;
			Z1 = Z0 + 1;

			DX = InGridCoord.X - X0;
			DY = InGridCoord.Y - Y0;
			DZ = InGridCoord.Z - Z0;
		}

		const float D000 = GetSDFValue(Getter, FIntVector(X0, Y0, Z0));
		const float D100 = GetSDFValue(Getter, FIntVector(X1, Y0, Z0));
		const float D010 = GetSDFValue(Getter, FIntVector(X0, Y1, Z0));
		const float D110 = GetSDFValue(Getter, FIntVector(X1, Y1, Z0));
	
		const float D001 = GetSDFValue(Getter, FIntVector(X0, Y0, Z1));
		const float D101 = GetSDFValue(Getter, FIntVector(X1, Y0, Z1));
		const float D011 = GetSDFValue(Getter, FIntVector(X0, Y1, Z1));
		const float D111 = GetSDFValue(Getter, FIntVector(X1, Y1, Z1));

		return FMath::Lerp(
			FMath::Lerp(
				FMath::Lerp(D000, D100, DX),
				FMath::Lerp(D010, D110, DX), DY),
			FMath::Lerp(
				FMath::Lerp(D001, D101, DX),
				FMath::Lerp(D011, D111, DX), DY), DZ);
	}
	
	// Legacy version for backward compatibility
	float Sample(const DA::VDB::FVoxelGrid& InVoxelGrid, const FVector& InGridCoord) {
		auto Getter = [&InVoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
			return InVoxelGrid.Get(Coord, OutData);
		};
		return Sample(Getter, InGridCoord);
	}

	// New version using getter
	FVector GetNormalAt(const DA::MarchingCubes::FVoxelDataGetter& Getter, const FVector& InGridCoordF) {
		FVector Normal;

		constexpr float Delta = 1.0f;
		Normal.X = Sample(Getter, FVector(InGridCoordF.X + Delta, InGridCoordF.Y, InGridCoordF.Z)) - Sample(Getter, FVector(InGridCoordF.X - Delta, InGridCoordF.Y, InGridCoordF.Z));
		Normal.Y = Sample(Getter, FVector(InGridCoordF.X, InGridCoordF.Y + Delta, InGridCoordF.Z)) - Sample(Getter, FVector(InGridCoordF.X, InGridCoordF.Y - Delta, InGridCoordF.Z));
		Normal.Z = Sample(Getter, FVector(InGridCoordF.X, InGridCoordF.Y, InGridCoordF.Z + Delta)) - Sample(Getter, FVector(InGridCoordF.X, InGridCoordF.Y, InGridCoordF.Z - Delta));
		Normal.Normalize();

		return Normal;
	}
	
	// Legacy version for backward compatibility
	FVector GetNormalAt(const DA::VDB::FVoxelGrid& InVoxelGrid, const FVector& InGridCoordF) {
		auto Getter = [&InVoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
			return InVoxelGrid.Get(Coord, OutData);
		};
		return GetNormalAt(Getter, InGridCoordF);
	}
}

// New version using getter
void DA::MarchingCubes::FMeshBuilder::GenerateMeshBlock(const DA::MarchingCubes::FSettings& InSettings, const FIntVector& InCoordMin, const FIntVector& InCoordMax, const FVoxelDataGetter& Getter, FDungeonMeshGeometry& OutMeshData) {
	using namespace UE::Geometry;
	
	struct FCellGeometry {
		int32 VertexBufferIndex[12];
		int32 CreatedIndices[12];
	};

	//constexpr int32 Start = 0;
	//const int32 End = InData.GetNumCellsPerChunk();
	//const int32 NumVoxels = End - Start;

	FIntVector ChunkSize = InCoordMax - InCoordMin + FIntVector(1);
	
	TArray<FCellGeometry> VoxelDecks[2];

	const int32 DeckSize = ChunkSize.X * ChunkSize.Y;
	VoxelDecks[0].AddUninitialized(DeckSize);
	VoxelDecks[1].AddUninitialized(DeckSize);
	int32 ActiveDeckIndex = 0;

	FVoxelCellCoords CellCoords;
	FVoxelCellDensities CellDensities;

	for (int iz = 0; iz < ChunkSize.Z; iz++) {
		const uint8 CanMoveZ = (iz == 0 ? 0 : 1) << 2;
		for (int iy = 0; iy < ChunkSize.Y; iy++) {
			const uint8 CanMoveY = (iy == 0 ? 0 : 1) << 1;
			for (int ix = 0; ix < ChunkSize.X; ix++) {
				const uint8 CanMoveX = (ix == 0) ? 0 : 1;

				FIntVector Coord = InCoordMin + FIntVector(ix, iy, iz);

				FDAMarchingCubeMeshUtils::GetCellCoords(Coord, CellCoords);
				FDAMarchingCubeMeshUtils::GetCellDensities(Getter, Coord, CellDensities);

				if (!CellDensities.bAllCornersActive) {
					continue;
				}

				uint32 CubeIndex = 0;
				if (CellDensities.Densities[0] > 0) CubeIndex |= 0x01;
				if (CellDensities.Densities[1] > 0) CubeIndex |= 0x02;
				if (CellDensities.Densities[2] > 0) CubeIndex |= 0x04;
				if (CellDensities.Densities[3] > 0) CubeIndex |= 0x08;
				if (CellDensities.Densities[4] > 0) CubeIndex |= 0x10;
				if (CellDensities.Densities[5] > 0) CubeIndex |= 0x20;
				if (CellDensities.Densities[6] > 0) CubeIndex |= 0x40;
				if (CellDensities.Densities[7] > 0) CubeIndex |= 0x80;

				if (CubeIndex == 0 || CubeIndex == 255) {
					continue;
				}

				const uint8 ValidMovementMask = CanMoveX | CanMoveY | CanMoveZ;

				const uint8 CellClass = RegularCellClass[CubeIndex];
				const FRegularCellData& TriangulationData = RegularCellData[CellClass];
				const uint16* VertexData = RegularVertexData[CubeIndex];

				FCellGeometry& CurrentCellGeometry = VoxelDecks[ActiveDeckIndex][ix + iy * ChunkSize.X];
				const uint32 NumVertices = TriangulationData.GetVertexCount();
				for (uint32 vi = 0; vi < NumVertices; vi++) {
					const uint16 EdgeCode = VertexData[vi];

					const uint16 CornerIndex0 = (EdgeCode >> 4) & 0x0F;
					const uint16 CornerIndex1 = EdgeCode & 0x0F;

					const uint8 MappingCode = (EdgeCode >> 8);
					const uint8 MovementMask = (MappingCode >> 4) & 0x0F;
					const uint8 VertexLocalIndex = MappingCode & 0x0F;

					const bool bCreateVertex = (MovementMask & ValidMovementMask) != MovementMask;

					int32 VertexBufferIndex = -1;
					if (bCreateVertex) {
						float D0 = CellDensities.Densities[CornerIndex0];
						float D1 = CellDensities.Densities[CornerIndex1];
						FIntVector Coord0 = CellCoords.Coords[CornerIndex0];
						FIntVector Coord1 = CellCoords.Coords[CornerIndex1];
						//FVector P;
						FVertexInfo Vertex;
						
						float t = 0;

						FDAMarchingCubeMeshUtils::GetCutPoint(D0, D1, Coord0, Coord1, InSettings.VoxelSize, Vertex.Position, t);
						Vertex.UV = FVector2f::ZeroVector;
					
						FVector GridLocation = FVector(Coord0) * t + FVector(Coord1) * (1 - t);

						Vertex.Normal = FVector3f(FDAMarchingCubeMeshUtils::GetNormalAt(Getter, GridLocation));

						VertexBufferIndex = OutMeshData.Vertices.Add(Vertex);
						if (MovementMask == 8) {
							CurrentCellGeometry.CreatedIndices[VertexLocalIndex] = VertexBufferIndex;
						}

					}
					else {
						int32 px = ix;
						int32 py = iy;
						int32 pz = ActiveDeckIndex;

						if (MovementMask & 0x01) px = FMath::Max(0, px - 1);
						if (MovementMask & 0x02) py = FMath::Max(0, py - 1);
						if (MovementMask & 0x04) {
							pz = (pz + 1) % 2;
						}
						const FCellGeometry& PreviousCellGeometry = VoxelDecks[pz][px + py * ChunkSize.X];
						VertexBufferIndex = PreviousCellGeometry.CreatedIndices[VertexLocalIndex];
					}

					CurrentCellGeometry.VertexBufferIndex[vi] = VertexBufferIndex;
				}

				const int32 NumTriangles = TriangulationData.GetTriangleCount();
				for (int t = 0; t < NumTriangles; t++) {
					const int32 i0 = TriangulationData.vertexIndex[t * 3 + 0];
					const int32 i1 = TriangulationData.vertexIndex[t * 3 + 1];
					const int32 i2 = TriangulationData.vertexIndex[t * 3 + 2];

					const int32 v0 = CurrentCellGeometry.VertexBufferIndex[i0];
					const int32 v1 = CurrentCellGeometry.VertexBufferIndex[i1];
					const int32 v2 = CurrentCellGeometry.VertexBufferIndex[i2];

					FIndex3i Triangle(v0, v1, v2);
					OutMeshData.Triangles.Add(Triangle);
				}
			}
		}

		ActiveDeckIndex = (ActiveDeckIndex + 1) % 2;
	}
}

// Legacy overload for backward compatibility
void DA::MarchingCubes::FMeshBuilder::GenerateMeshBlock(const DA::MarchingCubes::FSettings& InSettings, const FIntVector& InCoordMin, const FIntVector& InCoordMax, DA::VDB::FVoxelGrid& VoxelGrid, FDungeonMeshGeometry& OutMeshData) {
	auto Getter = [&VoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
		return VoxelGrid.Get(Coord, OutData);
	};
	GenerateMeshBlock(InSettings, InCoordMin, InCoordMax, Getter, OutMeshData);
}

// New version using getter
void DA::MarchingCubes::FMeshBuilder::GenerateMesh(const DA::MarchingCubes::FSettings& InSettings, const FVoxelDataGetter& Getter, const TArray<FIntVector>& InVoxelCoords, FDungeonMeshGeometry& OutMeshData) {
	using namespace UE::Geometry;
	FVoxelCellCoords CellCoords;
	FVoxelCellDensities CellDensities;
	
	// Calculate LOD step size (1, 2, 4, 8, etc.)
	const int32 LODStep = 1 << InSettings.LODLevel;
	
	TMap<uint64, int32> EdgeToVertexMap;
	auto MakeEdgeHash = [](const FIntVector& V0, const FIntVector& V1) -> uint64 {
		FIntVector MinV = V0;
		FIntVector MaxV = V1;
		if (V0.X > V1.X || (V0.X == V1.X && V0.Y > V1.Y) || (V0.X == V1.X && V0.Y == V1.Y && V0.Z > V1.Z)) {
			MinV = V1;
			MaxV = V0;
		}
		
		uint64 Hash = 0;
		Hash |= (uint64(MinV.X & 0xFFFFF) << 44);  // 20 bits
		Hash |= (uint64(MinV.Y & 0x3FF) << 34);     // 10 bits  
		Hash |= (uint64(MinV.Z & 0x3FF) << 24);     // 10 bits
		Hash |= (uint64(MaxV.X & 0xFF) << 16);      // 8 bits
		Hash |= (uint64(MaxV.Y & 0xFF) << 8);       // 8 bits
		Hash |= (uint64(MaxV.Z & 0xFF));            // 8 bits
		return Hash;
	};
	
	// For LOD > 0, we need to collect all active voxels and find LOD-aligned cells
	TSet<FIntVector> ProcessedLODCells;
	
	for (const FIntVector& VoxelCoord : InVoxelCoords) {
		// Find the LOD cell this voxel belongs to
		FIntVector LODCellBase(
			(VoxelCoord.X / LODStep) * LODStep,
			(VoxelCoord.Y / LODStep) * LODStep,
			(VoxelCoord.Z / LODStep) * LODStep
		);
		
		// Skip if we've already processed this LOD cell
		if (ProcessedLODCells.Contains(LODCellBase)) {
			continue;
		}
		ProcessedLODCells.Add(LODCellBase);
		
		// Build the LOD cell corners
		FIntVector LODCellCoords[8];
		for (int i = 0; i < 8; i++) {
			const FIntVector Offset(
				(i & 1) ? LODStep : 0,
				(i & 2) ? LODStep : 0,
				(i & 4) ? LODStep : 0
			);
			LODCellCoords[i] = LODCellBase + Offset;
		}
		
		// Copy to CellCoords
		for (int i = 0; i < 8; i++) {
			CellCoords.Coords[i] = LODCellCoords[i];
		}
		
		// Get densities for LOD cell corners
		CellDensities.bAllCornersActive = true;
		for (int i = 0; i < 8; i++) {
			DA::VDB::FVoxelData VoxelData;
			if (Getter(LODCellCoords[i], VoxelData)) {
				CellDensities.Densities[i] = VoxelData.SDF;
			} else {
				CellDensities.bAllCornersActive = false;
				break;
			}
		}
		
		// Skip if no data at any corner
		if (!CellDensities.bAllCornersActive) {
			continue;
		}
		
		uint32 CubeIndex = 0;
		if (CellDensities.Densities[0] > 0) CubeIndex |= 0x01;
		if (CellDensities.Densities[1] > 0) CubeIndex |= 0x02;
		if (CellDensities.Densities[2] > 0) CubeIndex |= 0x04;
		if (CellDensities.Densities[3] > 0) CubeIndex |= 0x08;
		if (CellDensities.Densities[4] > 0) CubeIndex |= 0x10;
		if (CellDensities.Densities[5] > 0) CubeIndex |= 0x20;
		if (CellDensities.Densities[6] > 0) CubeIndex |= 0x40;
		if (CellDensities.Densities[7] > 0) CubeIndex |= 0x80;
		
		if (CubeIndex == 0 || CubeIndex == 255) {
			continue;
		}
		
		const uint8 CellClass = RegularCellClass[CubeIndex];
		const FRegularCellData& TriangulationData = RegularCellData[CellClass];
		const uint16* VertexData = RegularVertexData[CubeIndex];
		
		int32 EdgeVertexIndices[12];
		FMemory::Memset(EdgeVertexIndices, -1, sizeof(EdgeVertexIndices));
		
		static const int32 EdgeTable[12][2] = {
			{0, 1}, {1, 3}, {3, 2}, {2, 0},  // Bottom
			{4, 5}, {5, 7}, {7, 6}, {6, 4},  // Top
			{0, 4}, {1, 5}, {3, 7}, {2, 6}   // Vertical
		};
		
		const uint32 NumVertices = TriangulationData.GetVertexCount();
		TArray<int32> LocalVertexIndices;
		LocalVertexIndices.Reserve(NumVertices);
		
		for (uint32 vi = 0; vi < NumVertices; vi++) {
			const uint16 EdgeCode = VertexData[vi];
			
			const uint16 CornerIndex0 = (EdgeCode >> 4) & 0x0F;
			const uint16 CornerIndex1 = EdgeCode & 0x0F;
			
			int32 EdgeIndex = -1;
			for (int32 ei = 0; ei < 12; ei++) {
				if ((EdgeTable[ei][0] == CornerIndex0 && EdgeTable[ei][1] == CornerIndex1) ||
					(EdgeTable[ei][1] == CornerIndex0 && EdgeTable[ei][0] == CornerIndex1)) {
					EdgeIndex = ei;
					break;
				}
			}
			
			if (EdgeIndex >= 0 && EdgeVertexIndices[EdgeIndex] >= 0) {
				LocalVertexIndices.Add(EdgeVertexIndices[EdgeIndex]);
				continue;
			}
			
			FIntVector Coord0 = CellCoords.Coords[CornerIndex0];
			FIntVector Coord1 = CellCoords.Coords[CornerIndex1];
			
			uint64 EdgeHash = MakeEdgeHash(Coord0, Coord1);
			
			int32 VertexIndex = -1;
			if (int32* ExistingVertex = EdgeToVertexMap.Find(EdgeHash)) {
				VertexIndex = *ExistingVertex;
			} else {
				float D0 = CellDensities.Densities[CornerIndex0];
				float D1 = CellDensities.Densities[CornerIndex1];
				
				FVertexInfo Vertex;
				float t = 0;
				FDAMarchingCubeMeshUtils::GetCutPoint(D0, D1, Coord0, Coord1, InSettings.VoxelSize, Vertex.Position, t);
				
				FVector GridLocation = FVector(Coord0) * t + FVector(Coord1) * (1 - t);
				Vertex.Normal = FVector3f(FDAMarchingCubeMeshUtils::GetNormalAt(Getter, GridLocation));
				// Use world position for UV calculation to ensure seamless texturing across chunks
				FVector WorldPosition = Vertex.Position; // + InSettings.ChunkWorldOffset;
				Vertex.UV = FDungeonRenderUtils::CalculateTriplanarUV(WorldPosition, Vertex.Normal, InSettings.UVScale);
				
				VertexIndex = OutMeshData.Vertices.Add(Vertex);
				EdgeToVertexMap.Add(EdgeHash, VertexIndex);
			}
			
			if (EdgeIndex >= 0) {
				EdgeVertexIndices[EdgeIndex] = VertexIndex;
			}
			LocalVertexIndices.Add(VertexIndex);
		}
		
		const int32 NumTriangles = TriangulationData.GetTriangleCount();
		for (int t = 0; t < NumTriangles; t++) {
			const int32 i0 = TriangulationData.vertexIndex[t * 3 + 0];
			const int32 i1 = TriangulationData.vertexIndex[t * 3 + 1];
			const int32 i2 = TriangulationData.vertexIndex[t * 3 + 2];
			
			const int32 v0 = LocalVertexIndices[i0];
			const int32 v1 = LocalVertexIndices[i1];
			const int32 v2 = LocalVertexIndices[i2];
			
			FIndex3i Triangle(v0, v1, v2);
			OutMeshData.Triangles.Add(Triangle);
		}
	}
}

// Legacy overload for backward compatibility
void DA::MarchingCubes::FMeshBuilder::GenerateMesh(const DA::MarchingCubes::FSettings& InSettings, DA::VDB::FVoxelGrid& VoxelGrid, const TArray<FIntVector>& InVoxelCoords, FDungeonMeshGeometry& OutMeshData) {
	auto Getter = [&VoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
		return VoxelGrid.Get(Coord, OutData);
	};
	GenerateMesh(InSettings, Getter, InVoxelCoords, OutMeshData);
}

// Legacy overload for backward compatibility
void DA::MarchingCubes::FMeshBuilder::GenerateMeshWorld(const DA::MarchingCubes::FSettings& InSettings, DA::VDB::FVoxelGrid& VoxelGrid, FDungeonMeshGeometry& OutMeshData) {
	using namespace UE::Geometry;

	TArray<FIntVector> VoxelCoords;
	
	auto ActiveIterator = VoxelGrid.GetActiveIterator();
	for (; ActiveIterator.IsValid(); ++ActiveIterator) {
		VoxelCoords.Add(ActiveIterator.GetCoordinate());
	}

	GenerateMesh(InSettings, VoxelGrid, VoxelCoords, OutMeshData);
}


///////////////////////// Surface Nets /////////////////////////
void DA::SurfaceNets::FMeshBuilder::GenerateMeshWorld(const DA::SurfaceNets::FSettings& InSettings, DA::VDB::FVoxelGrid& VoxelGrid, FDungeonMeshGeometry& OutMeshData) {
	using namespace UE::Geometry;

	struct FEdgeData {
		float Delta{};
		bool bFlipped{};
	};
	TMap<FIntVector4, FEdgeData> EdgeIntersections;

	struct FCellData {
		int32 Count = 0;
		FVector CoordSum = FVector::ZeroVector;
		int32 VertexIndex = INDEX_NONE;
	};
	TMap<FIntVector, FCellData> CellVertices;
	
	auto ActiveIterator = VoxelGrid.GetActiveIterator();
	for (; ActiveIterator.IsValid(); ++ActiveIterator) {
		FIntVector VoxelCoord = ActiveIterator.GetCoordinate();

		auto GetSDFValue = [&VoxelGrid](const FIntVector& InVoxelCoord, float& OutSDFValue) {
			DA::VDB::FVoxelData VoxelData{};
			if (VoxelGrid.Get(InVoxelCoord, VoxelData)) {
				OutSDFValue = VoxelData.SDF;
				return true;
			}
			return false;
		};

		// Write out the edge intersetion to all the surrounding 4 cells
		const TArray<FIntVector> NeighborCoordDeltasX = {
			{ 0, 0, 0 },
			{ 0, -1, 0 },
			{ 0, -1, -1 },
			{ 0, 0, -1 }
		};

		const TArray<FIntVector> NeighborCoordDeltasY = {
			{ 0, 0, 0 },
			{ -1, 0, 0 },
			{ -1, 0, -1 },
			{ 0, 0, -1 }
		};
		
		const TArray<FIntVector> NeighborCoordDeltasZ = {
			{ 0, 0, 0 },
			{ -1, 0, 0 },
			{ -1, -1, 0 },
			{ 0, -1, 0 }
		};
		
		auto ProcessVoxelEdge = [&EdgeIntersections, &CellVertices, VoxelCoord](int32 D, int32 DE, int32 AxisIndex, const FIntVector& VoxelCoordEdgeEnd, const TArray<FIntVector>& NeighborCoordDeltas) {
			// Register the edge information
			FEdgeData& EdgeData = EdgeIntersections.FindOrAdd(FIntVector4(VoxelCoord, AxisIndex));
			EdgeData.bFlipped = D > DE;
			EdgeData.Delta = DE / (DE - D);

			// Accumulate the point
			FVector EdgeIntersection = FVector(VoxelCoord) * EdgeData.Delta + FVector(VoxelCoordEdgeEnd) * (1 - EdgeData.Delta);
					
			for (int i = 0; i < 4; i++) {
				FCellData& CellData = CellVertices.FindOrAdd(VoxelCoord + NeighborCoordDeltas[i]);
				CellData.CoordSum += EdgeIntersection;
				CellData.Count++;
			}
		};
		
		float DBase = 0;
		if (GetSDFValue(VoxelCoord, DBase)) {
			// X-Axis edge
			{
				float DE = 0;
				FIntVector VoxelCoordEdgeX = VoxelCoord + FIntVector(1, 0, 0);
				if (GetSDFValue(VoxelCoordEdgeX, DE)) {
					if (FMath::Sign(DBase) != FMath::Sign(DE)) {
						// Register the edge information
						constexpr int32 AxisIndex = 0;
						ProcessVoxelEdge(DBase, DE, AxisIndex, VoxelCoordEdgeX, NeighborCoordDeltasX);
					}
				}
			}
			
			// Y-Axis edge
			{
				float DE = 0;
				FIntVector VoxelCoordEdgeY = VoxelCoord + FIntVector(0, 1, 0);
				if (GetSDFValue(VoxelCoordEdgeY, DE)) {
					if (FMath::Sign(DBase) != FMath::Sign(DE)) {
						// Register the edge information
						constexpr int32 AxisIndex = 1;
						ProcessVoxelEdge(DBase, DE, AxisIndex, VoxelCoordEdgeY, NeighborCoordDeltasY); // Fixed: was using X deltas
					}
				}
			}
			
			// Z-Axis edge
			{
				float DE = 0;
				FIntVector VoxelCoordEdgeZ = VoxelCoord + FIntVector(0, 0, 1);
				if (GetSDFValue(VoxelCoordEdgeZ, DE)) {
					if (FMath::Sign(DBase) != FMath::Sign(DE)) {
						// Register the edge information
						constexpr int32 AxisIndex = 2;
						ProcessVoxelEdge(DBase, DE, AxisIndex, VoxelCoordEdgeZ, NeighborCoordDeltasZ); // Fixed: was using X deltas
					}
				}
			}
		}
	}
	
	// Create vertices for cells that have accumulated edge intersections
	for (auto& Cell : CellVertices) {
		FCellData& CellData = Cell.Value;
		if (CellData.Count > 0) {
			// Average the accumulated positions
			FVector VertexPos = (CellData.CoordSum / float(CellData.Count)) * InSettings.VoxelSize;
			
			// Calculate normal at cell center
			FVector GridCenter = FVector(Cell.Key) + FVector(0.5f);
			auto Getter = [&VoxelGrid](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
				return VoxelGrid.Get(Coord, OutData);
			};
			FVector Normal = FDAMarchingCubeMeshUtils::GetNormalAt(Getter, GridCenter);
			
			FVertexInfo Vertex;
			Vertex.Position = VertexPos;
			Vertex.Normal = FVector3f(Normal);
			
			// Calculate triplanar UV using world position for seamless texturing
			FVector WorldPosition = VertexPos + InSettings.ChunkWorldOffset;
			Vertex.UV = FDungeonRenderUtils::CalculateTriplanarUV(WorldPosition, Vertex.Normal, InSettings.UVScale);
			
			CellData.VertexIndex = OutMeshData.Vertices.Add(Vertex);
		}
	}
	
	// Emit quads for each edge intersection
	for (const auto& Edge : EdgeIntersections) {
		FIntVector VoxelCoord = FIntVector(Edge.Key.X, Edge.Key.Y, Edge.Key.Z);
		int32 AxisIndex = Edge.Key.W;
		bool bFlipped = Edge.Value.bFlipped;
		
		// Get the 4 cells surrounding this edge
		TArray<FIntVector> CellCoords;
		
		if (AxisIndex == 0) { // X-axis edge
			CellCoords = {
				VoxelCoord,
				VoxelCoord + FIntVector(0, 0, -1),
				VoxelCoord + FIntVector(0, -1, -1),
				VoxelCoord + FIntVector(0, -1, 0)
			};
		} else if (AxisIndex == 1) { // Y-axis edge
			CellCoords = {
				VoxelCoord,
				VoxelCoord + FIntVector(-1, 0, 0),
				VoxelCoord + FIntVector(-1, 0, -1),
				VoxelCoord + FIntVector(0, 0, -1)
			};
		} else { // Z-axis edge
			CellCoords = {
				VoxelCoord,
				VoxelCoord + FIntVector(0, -1, 0),
				VoxelCoord + FIntVector(-1, -1, 0),
				VoxelCoord + FIntVector(-1, 0, 0)
			};
		}
		
		// Get vertex indices for the 4 cells
		int32 V[4];
		bool bAllVerticesExist = true;
		for (int i = 0; i < 4; i++) {
			FCellData* CellData = CellVertices.Find(CellCoords[i]);
			if (CellData && CellData->VertexIndex != INDEX_NONE) {
				V[i] = CellData->VertexIndex;
			} else {
				bAllVerticesExist = false;
				break;
			}
		}
		
		// Emit quad if all 4 vertices exist
		if (bAllVerticesExist) {
			if (bFlipped) {
				// Flipped winding
				OutMeshData.Triangles.Add(FIndex3i(V[0], V[3], V[1]));
				OutMeshData.Triangles.Add(FIndex3i(V[1], V[3], V[2]));
			} else {
				// Normal winding
				OutMeshData.Triangles.Add(FIndex3i(V[0], V[1], V[3]));
				OutMeshData.Triangles.Add(FIndex3i(V[1], V[2], V[3]));
			}
		}
	}
}
