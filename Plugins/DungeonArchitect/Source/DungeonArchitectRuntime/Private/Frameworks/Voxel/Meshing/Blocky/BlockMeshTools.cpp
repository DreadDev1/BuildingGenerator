//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Meshing/Blocky/BlockMeshTools.h"

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Math/RandomStream.h"

void FDAVoxelMeshGenerator::GenerateBlockMesh(UE::Geometry::FDynamicMesh3& Mesh, DA::VDB::FVoxelGrid& VoxelGrid, const FDAVoxelMeshGenSettings& InSettings) {
	using namespace UE::Geometry;

	// Generate a color palette
	const FRandomStream Random(InSettings.Seed);
	TArray<FVector3f> ColorPalette;
	for (int i = 0; i < 256; i++) {
		const uint8 Hue = Random.RandRange(0, 255);
		FVector3f Color = FVector3f(FLinearColor::MakeFromHSV8(Hue, 128, 255));
		ColorPalette.Add(Color);
	}

	Mesh.Clear();
	Mesh.EnableTriangleGroups();
	Mesh.EnableAttributes();
	Mesh.Attributes()->EnablePrimaryColors();
	Mesh.Attributes()->EnableMaterialID();
	
	FDynamicMeshUVOverlay* UVOverlay = Mesh.Attributes()->PrimaryUV();
	FDynamicMeshNormalOverlay* NormalOverlay = Mesh.Attributes()->PrimaryNormals();
	FDynamicMeshColorOverlay* ColorOverlay = Mesh.Attributes()->PrimaryColors();

	auto AppendVertex = [&](const FVertexInfo& VertexInfo) {
		const int32 Index = Mesh.AppendVertex(VertexInfo.Position);
		UVOverlay->AppendElement(VertexInfo.UV);
		NormalOverlay->AppendElement(VertexInfo.Normal);
		ColorOverlay->AppendElement(VertexInfo.Color);
		return Index;
	};

	auto AppendTriangle = [&](const FIndex3i& Tri, int32 PolyID) {
		const int32 TriId = Mesh.AppendTriangle(Tri, PolyID);
		UVOverlay->SetTriangle(TriId, Tri);
		NormalOverlay->SetTriangle(TriId, Tri);
		ColorOverlay->SetTriangle(TriId, Tri);
	};

#define DAIDX(x, y) (((y) - CoordMin[AxisMap.Y]) * DeckWidth + ((x) - CoordMin[AxisMap.X]))
	// Build the geometry
	// XY - axis

	// Axis mapping
	TArray<FIntVector> AxisMappings = {
		{0, 1, 2},
		{2, 0, 1},
		{1, 2, 0}
	};
	
	int32 PolyID = 0;
	//FIntVector AxisMap = { 0, 1, 2 };
	for (const FIntVector& AxisMap : AxisMappings) {
		auto TransformVec = [&](const FVector& InV) {
			FVector V;
			V[AxisMap.X] = InV.X;
			V[AxisMap.Y] = InV.Y;
			V[AxisMap.Z] = InV.Z;
			return V;
		};
		auto TransformIntVec = [&](const FIntVector& InV) {
			FIntVector V;
			V[AxisMap.X] = InV.X;
			V[AxisMap.Y] = InV.Y;
			V[AxisMap.Z] = InV.Z;
			return V;
		};

		const FIntVector CoordMin = InSettings.CoordMin;
		const FIntVector CoordMax = InSettings.CoordMax;

		const FIntVector Axis = TransformIntVec(FIntVector(0, 0, 1));
		const int32 DeckWidth = CoordMax[AxisMap.X] - CoordMin[AxisMap.X] + 1;
		const int32 DeckHeight = CoordMax[AxisMap.Y] - CoordMin[AxisMap.Y] + 1;
		TArray<DA::VDB::VoxelMaterial_t> DeckLo;
		TArray<DA::VDB::VoxelMaterial_t> DeckHi;
		DeckLo.AddDefaulted(DeckWidth * DeckHeight);
		DeckHi.AddDefaulted(DeckWidth * DeckHeight);

		TArray<bool> MeshingVisited;
		MeshingVisited.AddDefaulted(DeckWidth * DeckHeight);
		
		DA::VDB::FVoxelData VoxelDataLo, VoxelDataHi;
		for (int h = CoordMin[AxisMap.Z]; h <= CoordMax[AxisMap.Z]; h++) {
			TArray<DA::VDB::VoxelMaterial_t> DeckLoUniqueMats;
			TArray<DA::VDB::VoxelMaterial_t> DeckHiUniqueMats;
			FMemory::Memset(DeckLo.GetData(), 0, sizeof(DA::VDB::VoxelMaterial_t) * DeckLo.Num());
			FMemory::Memset(DeckHi.GetData(), 0, sizeof(DA::VDB::VoxelMaterial_t) * DeckHi.Num());
			
			for (int a2 = CoordMin[AxisMap.Y]; a2 <= CoordMax[AxisMap.Y]; a2++) {
				for (int a1 = CoordMin[AxisMap.X]; a1 <= CoordMax[AxisMap.X]; a1++) {
					FIntVector Coord = TransformIntVec(FIntVector(a1, a2, h));
					DA::VDB::FVoxelData VoxelData;
					if (!VoxelGrid.Get(Coord, VoxelData)) {
						continue;
					}
					if (VoxelData.Material == 0) {
						// Empty material
						continue;
					}

					// Update the Lower deck
					{
						const bool bActive = VoxelGrid.Get(Coord - Axis, VoxelDataLo);
						const bool bEmpty = !bActive || VoxelDataLo.Material == 0;
						if (bEmpty) {
							DeckLo[DAIDX(a1, a2)] = VoxelData.Material;
							DeckLoUniqueMats.AddUnique(VoxelData.Material);
						}
					}

					// Update the upper deck
					{
						const bool bActive = VoxelGrid.Get(Coord + Axis, VoxelDataHi);
						const bool bEmpty = !bActive || VoxelDataHi.Material == 0;
						if (bEmpty) {
							DeckHi[DAIDX(a1, a2)] = VoxelData.Material;
							DeckHiUniqueMats.AddUnique(VoxelData.Material);
						}
					}
				}
			}

			struct FQuadInfo {
				FIntPoint Location;
				FIntPoint Size;
				DA::VDB::VoxelMaterial_t MaterialId;
			};

			// Run greedy meshing on the decks to reduce the quads
			auto ReduceQuads = [&](const TArray<DA::VDB::VoxelMaterial_t>& Deck, const TArray<DA::VDB::VoxelMaterial_t>& InDeckUniqueMats, TArray<FQuadInfo>& OutQuadList) {
				FMemory::Memset(MeshingVisited.GetData(), 0, sizeof(bool) * MeshingVisited.Num());
		
				for (const DA::VDB::VoxelMaterial_t MaterialId : InDeckUniqueMats) {
					const int32 MinA1 = CoordMin[AxisMap.X];
					const int32 MaxA1 = CoordMax[AxisMap.X];
					const int32 MinA2 = CoordMin[AxisMap.Y];
					const int32 MaxA2 = CoordMax[AxisMap.Y];

					if (InSettings.bOptimizeMesh) {
						for (int a2 = MinA2; a2 <= MaxA2; a2++) {
							for (int a1 = MinA1; a1 <= MaxA1; a1++) {
								const int32 Idx = DAIDX(a1, a2);
								if (MeshingVisited[Idx] || Deck[Idx] != MaterialId) {
									continue;
								}
								// Try to get as many cells in the logical x coordinate
								int32 na1;
								for (na1 = a1 + 1; na1 <= MaxA1; na1++) {
									const int32 NIdx = DAIDX(na1, a2);
									if (MeshingVisited[NIdx] || Deck[NIdx] != MaterialId) {
										break;
									}
								}
								const int32 Width = na1 - a1;
								int32 Height = 1;
								// Try to increase along the height
								for (int na2 = a2 + 1; na2 <= MaxA2; na2++) {
									bool bValid = true;
									for (na1 = a1; na1 < a1 + Width; na1++) {
										const int32 NIdx = DAIDX(na1, na2);
										if (MeshingVisited[NIdx] || Deck[NIdx] != MaterialId) {
											bValid = false;
											break;
										}
									}

									if (bValid) {
										Height++;
									}
									else {
										break;
									}
								}

								// Save the Quad
								OutQuadList.Add({
									FIntPoint(a1, a2),
									FIntPoint(Width, Height),
									MaterialId
								});

								// Tag the quad area as visited
								for (int iw = 0; iw < Width; iw++) {
									for (int ih = 0; ih < Height; ih++) {
										MeshingVisited[DAIDX(a1 + iw, a2 + ih)] = true;
									}
								}
							}
						}
					}
					else {
						for (int a2 = MinA2; a2 <= MaxA2; a2++) {
							for (int a1 = MinA1; a1 <= MaxA1; a1++) {
								const int32 Idx = DAIDX(a1, a2);
								if (MeshingVisited[Idx] || Deck[Idx] != MaterialId) {
									continue;
								}

								// Save the Quad
								OutQuadList.Add({
									FIntPoint(a1, a2),
									FIntPoint(1, 1),
									MaterialId
								});
							}
						}
					}
				}
			};
	
			TArray<FQuadInfo> QuadListLo;
			TArray<FQuadInfo> QuadListHi;
			ReduceQuads(DeckLo, DeckLoUniqueMats, QuadListLo);
			ReduceQuads(DeckHi, DeckHiUniqueMats, QuadListHi);

			auto EmitDeckGeometry = [&](const TArray<FQuadInfo>& InQuadList, bool bLowerDeck) {
				FVector3f Normal = FVector3f(TransformVec(bLowerDeck ? FVector(0, 0, -1) : FVector(0, 0, 1)));
				int32 OffsetH = bLowerDeck ? 0 : 1;
				for (const FQuadInfo& QuadInfo : InQuadList) {
					FVector3f Color = ColorPalette[QuadInfo.MaterialId];
					const int32 x0 = QuadInfo.Location.X;
					const int32 x1 = QuadInfo.Location.X + QuadInfo.Size.X;
					const int32 y0 = QuadInfo.Location.Y;
					const int32 y1 = QuadInfo.Location.Y + QuadInfo.Size.Y;

					FVertexInfo V00(TransformVec(FVector(x0, y0, h + OffsetH) * InSettings.VoxelSize), Normal, Color, FVector2f(x0, y0));
					FVertexInfo V10(TransformVec(FVector(x1, y0, h + OffsetH) * InSettings.VoxelSize), Normal, Color, FVector2f(x1, y0));
					FVertexInfo V11(TransformVec(FVector(x1, y1, h + OffsetH) * InSettings.VoxelSize), Normal, Color, FVector2f(x1, y1));
					FVertexInfo V01(TransformVec(FVector(x0, y1, h + OffsetH) * InSettings.VoxelSize), Normal, Color, FVector2f(x0, y1));

					int32 I00 = AppendVertex(V00);
					int32 I10 = AppendVertex(V10);
					int32 I11 = AppendVertex(V11);
					int32 I01 = AppendVertex(V01);

					if (bLowerDeck) {
						AppendTriangle(FIndex3i(I00, I11, I01), PolyID);
						AppendTriangle(FIndex3i(I00, I10, I11), PolyID);
					}
					else {
						AppendTriangle(FIndex3i(I00, I01, I11), PolyID);
						AppendTriangle(FIndex3i(I00, I11, I10), PolyID);
					}
					PolyID++;
				}
			};

			EmitDeckGeometry(QuadListLo, true);
			EmitDeckGeometry(QuadListHi, false);
		}
	}
#undef DAIDX
}

