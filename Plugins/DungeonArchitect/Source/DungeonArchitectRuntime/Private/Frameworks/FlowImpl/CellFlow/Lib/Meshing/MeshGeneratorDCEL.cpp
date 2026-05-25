//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshGeneratorDCEL.h"

#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Utils/Noise/Noise.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshLib.h"
#include "Frameworks/Lib/Geometry/DCEL.h"

enum class ECellFlowDCELMeshPatchType {
	Ground,
	Cliff,
	Stair
};

class FCellFlowDCELMeshLib {
public:
	FCellFlowDCELMeshLib(const FVector3f& InGridSize, bool bInUseSmoothNormals)
		: GridSize(InGridSize)
		, bUseSmoothNormals(bInUseSmoothNormals)
	{
	}
	
	void RenderStairs(const FDAFlowCellGraphDCELEdgeInfo* StairInfo, int LOD, FFlowVisLib::FGeometry& OutGeometry) {
		static const FVector4f StairColor{1, 0.25f, 0.25f, 1};
		const FVector3f StairScale = FVector3f(StairInfo->LogicalWidth, 1, 1);
		const FQuat Rotation = FQuat::FindBetweenVectors({0, -1, 0}, StairInfo->Direction);
		const FVector BaseLocation = StairInfo->LogicalLocation;
		const FVector Location = BaseLocation + StairInfo->Direction * 0.5f;

		TArray<FFlowVisLib::FDAVertexInfo> Vertices;
		TArray<UE::Geometry::FIndex3i> Triangles;
		FCellFlowLevelMeshLib::RenderStairs(FVector3f(Location), UE::Math::TQuat<float>(Rotation), FVector3f(StairScale),
			{1, 1}, StairColor,
			[&Vertices](const FFlowVisLib::FDAVertexInfo& InVertexInfo) {
				return Vertices.Add(InVertexInfo);
			},
			[&Triangles](const UE::Geometry::FIndex3i& InTriangle) {
				Triangles.Add(InTriangle);
			});

		for (const UE::Geometry::FIndex3i& Triangle : Triangles) {
			FVector3f V0 = FVector3f(Vertices[Triangle.A].Position);
			FVector3f V1 = FVector3f(Vertices[Triangle.B].Position);
			FVector3f V2 = FVector3f(Vertices[Triangle.C].Position);

			TessellatePoly({V0, V1, V2}, Vertices[0].Color, LOD, ECellFlowDCELMeshPatchType::Stair, OutGeometry);
		} 
	};

	int CreateVert(const FVector3f& InLocation, const FVector4f FaceColor, ECellFlowDCELMeshPatchType InPatchType, FFlowVisLib::FGeometry& OutGeometry) {
		const FVector3f WorldLocation = InLocation * GridSize;
		const uint32 LocationHash = FDAVertSpatialCache::GetHash(WorldLocation);
		FDAVertSpatialCache* VertexCache = GetVertexCache(InPatchType);
		int32 Index{};
		auto AddVert = [&]() {
			return OutGeometry.Vertices.Add(FFlowVisLib::CreateVertex(WorldLocation, FVector2f(0.5f, 0.5f), {1, 1}, FaceColor));
		};
		
		if (VertexCache) {
			if (const int32* IndexPtr = VertexCache->LocationToIndexMap.Find(LocationHash)) {
				Index = *IndexPtr;
				check(Index < OutGeometry.Vertices.Num());
			}
			else {
				Index = AddVert();
				VertexCache->LocationToIndexMap.Add(LocationHash, Index);
			
				check(Index < OutGeometry.Vertices.Num());
			}
		}
		else {
			Index = AddVert();
		}
			
		return Index;
	};

	int EmitTriangle(const FVector3f& A, const FVector3f& B, const FVector3f& C, const FVector4f& InFaceColor, ECellFlowDCELMeshPatchType InPatchType, FFlowVisLib::FGeometry& OutGeometry) {
		const int32 I0 = CreateVert(A, InFaceColor, InPatchType, OutGeometry);
		const int32 I1 = CreateVert(B, InFaceColor, InPatchType, OutGeometry);
		const int32 I2 = CreateVert(C, InFaceColor, InPatchType, OutGeometry);
		return OutGeometry.Triangles.Add(UE::Geometry::FIndex3i(I0, I1, I2));
	};
	void TessellatePoly(const TArray<FVector3f>& InLocations, const FVector4f& InFaceColor, int32 LOD, ECellFlowDCELMeshPatchType InPatchType, FFlowVisLib::FGeometry& OutGeometry, TArray<int32>* OutTriangleIndices = {}) {
		if (InLocations.Num() == 0) {
			return;
		}
						
		if (LOD == 0) {
			// Triangulate
			for (int i = 1; i + 1 < InLocations.Num(); i++) {
				const FVector3f& A = InLocations[0];
				const FVector3f& B = InLocations[i];
				const FVector3f& C = InLocations[i + 1];
				int TriangleIdx = EmitTriangle(A, B, C, InFaceColor, InPatchType, OutGeometry);
				if (OutTriangleIndices) {
					OutTriangleIndices->Add(TriangleIdx);
				}
			}

			return;
		}

		FVector3f Center{ FVector3f::ZeroVector };
		for (const FVector3f& Location : InLocations) {
			Center += Location;
		}
		Center /= InLocations.Num();

		TArray<FVector3f> EdgeMidPoints;
		EdgeMidPoints.SetNum(InLocations.Num());
		for (int i = 0; i < InLocations.Num(); i++) {
			const FVector3f& A = InLocations[i];
			const FVector3f& B = InLocations[(i + 1) % InLocations.Num()];
			EdgeMidPoints[i] = (A + B) * 0.5;
		}

		for (int i = 0; i < InLocations.Num(); i++) {
			FVector3f NextMid = EdgeMidPoints[i];
			FVector3f PrevMid = EdgeMidPoints[(i - 1 + EdgeMidPoints.Num()) % EdgeMidPoints.Num()];
			TessellatePoly({ InLocations[i], NextMid, Center, PrevMid }, InFaceColor, LOD - 1, InPatchType, OutGeometry, OutTriangleIndices);
		}
	};
	
private:
	struct FDAVertSpatialCache {
		TMap<uint32, int32> LocationToIndexMap;

		template<typename TVector>
		static uint32 GetHash(const TVector& InVector) {
			return GetTypeHash(FIntVector(
				FMath::RoundToInt(InVector.X),
				FMath::RoundToInt(InVector.Y),
				FMath::RoundToInt(InVector.Z)));
		}
	};
	
	FDAVertSpatialCache* GetVertexCache(ECellFlowDCELMeshPatchType InPatchType) {
		if (InPatchType == ECellFlowDCELMeshPatchType::Ground) {
			return &SurfaceVertexCache;
		}

		if (bUseSmoothNormals) {
			if (InPatchType == ECellFlowDCELMeshPatchType::Cliff) {
				return &CliffVertexCache;
			}
			else if (InPatchType == ECellFlowDCELMeshPatchType::Stair) {
				return &SurfaceVertexCache;
			}
		}
		return nullptr;
	}
	
private:
	FVector3f GridSize;
	bool bUseSmoothNormals{};
	FDAVertSpatialCache SurfaceVertexCache;
	FDAVertSpatialCache CliffVertexCache;
};

void FCellFlowMeshGenDCEL::Generate(const UCellFlowLayoutGraph* InLayoutGraph,
                                    const UDAFlowCellGraph* InCellGraph,
                                    const UCellFlowVoronoiGraph* InVoronoiData,
                                    const FCellFlowLevelMeshGenSettings& InSettings,
                                    TArray<FFlowVisLib::FGeometry>& OutGeometries,
                                    FFlowVisLib::FGeometry* OutLineGeometry) {
	if (!InVoronoiData) {
		return;
	}
	bool bDrawLines = (OutLineGeometry != nullptr);

	const FVector3f GridSize = FVector3f(InSettings.VisualizationScale);
	
	TSet<int> DoorEdgeIndices(InCellGraph->DCELInfo.DoorEdges);
	const TMap<int, FDAFlowCellGraphDCELEdgeInfo>& Stairs = InCellGraph->DCELInfo.Stairs;

	TMap<FGuid, const UFlowAbstractNode*> LayoutNodes;
	for (const UFlowAbstractNode* GraphNode : InLayoutGraph->GraphNodes) {
		const UFlowAbstractNode*& NodeRef = LayoutNodes.FindOrAdd(GraphNode->NodeId);
		NodeRef = GraphNode;
	}

	FCellFlowLevelMeshLib::FUniqueLineCollection GroupLines(GridSize, FVector4f{0, 0, 0, 0.75f}, 3);
	FCellFlowLevelMeshLib::FUniqueLineCollection LeafLines(GridSize, FVector4f{0, 0, 0, 0.2f}, 1);
	const FVector4f InActiveColor = FVector4f{0.25f, 0.25f, 0.25f, 1.0f};

	TMap<int, int> LeafToGroupMap;
	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			LeafToGroupMap.Add(LeafNodeId, GroupNode.GroupId);
		}
	}

	int GlobalCliffDepth = FCellFlowLevelMeshLib::GetGlobalCliffDepth(InLayoutGraph, InCellGraph, LayoutNodes);
	FGradientNoiseTable2D NoiseTable;
	{
		FRandomStream FallbackRandomStream(0);
		FRandomStream* NoiseRandomStream = InSettings.Random ? InSettings.Random : &FallbackRandomStream;
		NoiseTable.Init(128, *NoiseRandomStream);
	}
	
	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		FCellFlowDCELMeshLib MeshGen(GridSize, InSettings.bSmoothNormals);
		const UFlowAbstractNode** LayoutNodePtr = LayoutNodes.Find(GroupNode.LayoutNodeID);
		const UFlowAbstractNode* LayoutNode = LayoutNodePtr ? *LayoutNodePtr : nullptr;

		bool bRenderInactiveGroups = false;
#if WITH_EDITORONLY_DATA
		bRenderInactiveGroups = InCellGraph->bRenderInactiveGroups;
#endif // WITH_EDITORONLY_DATA

		bool bGroupActive = LayoutNode && LayoutNode->bActive;
		if (!bRenderInactiveGroups && !bGroupActive) {
			continue;
		}

		const TArray<FVector2d>& Sites = InVoronoiData->Sites;
		const DA::DCELGraph& DGraph = InVoronoiData->DGraph;
		
		if (InSettings.bGeneratedMergedMesh && OutGeometries.Num() == 0) {
			OutGeometries.AddDefaulted();
		}
		const FVector4f FaceColor = bGroupActive ? FVector4f(GroupNode.GroupColor) : InActiveColor;
		FFlowVisLib::FGeometry SurfaceGeometry;
		FFlowVisLib::FGeometry CliffGeometry;
		TSet<int32> ProcessedCliffVertices;
		
		const TArray<DA::DCEL::FFace*>& Faces = DGraph.GetFaces();
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (Faces.IsValidIndex(LeafNodeId)) {
				UDAFlowCellLeafNode* LeafNode = InCellGraph->LeafNodes[LeafNodeId];
				if (!LeafNode) continue;
				const int LogicalHeightZ = LeafNode->LogicalZ;

				const DA::DCEL::FFace* Face = Faces[LeafNodeId];
				if (!Face || !Face->bValid || !Face->Outer) continue;
				if (!Sites.IsValidIndex(LeafNodeId)) {
					continue;
				}

				auto DrawEdgeLine = [&](const DA::DCEL::FEdge* InEdge, bool bGroupBorderEdge) {
					if (bGroupBorderEdge) {
						FVector4f LineColor = GroupLines.GetColor();
						if (DoorEdgeIndices.Contains(InEdge->Index)) {
							LineColor = FVector4f{1, 0, 0, 1};
						}
						if (Stairs.Contains(InEdge->Index)) {
							LineColor = FVector4f{1, 1, 0, 1};
						}

						GroupLines.Add(InEdge, LogicalHeightZ, LineColor);
					}
					else {
						LeafLines.Add(InEdge, LogicalHeightZ);
					}
				};

				auto GetLayoutNodeFromFaceId = [&](int FaceId) -> const UFlowAbstractNode* {
					if (const int* FlowGroupIdPtr = LeafToGroupMap.Find(FaceId)) {
						const int FlowGroupId = *FlowGroupIdPtr;
						const FDAFlowCellGroupNode& FlowGroupNode = InCellGraph->GroupNodes[FlowGroupId];
						if (const UFlowAbstractNode** FlowLayoutNodePtr = LayoutNodes.Find(FlowGroupNode.LayoutNodeID)) {
							return *FlowLayoutNodePtr;
						}
					}
					return nullptr;
				};

				auto IsGroupBorderEdge = [&GroupNode](const DA::DCEL::FEdge* InEdge) {
					const DA::DCEL::FFace* TwinFace = InEdge->Twin->LeftFace;
					if (TwinFace && TwinFace->bValid) {
						if (GroupNode.LeafNodes.Contains(TwinFace->FaceId)) {
							// Belongs to the same group
							return false;
						}
					}
					return true;
				};
				
				auto GetNextGroupBorderEdge = [&](const DA::DCEL::FEdge* InEdge) {
					check(IsGroupBorderEdge(InEdge));
					TSet<const DA::DCEL::FEdge*> Visited;
					Visited.Add(InEdge);

					DA::DCEL::FEdge* NextEdge = InEdge->Next;
					while (!Visited.Contains(NextEdge)) {
						if (IsGroupBorderEdge(NextEdge)) {
							break;
						}
						Visited.Add(NextEdge);
						NextEdge = NextEdge->Twin->Next;
					}

					return NextEdge;
				};
				
				auto GetPrevGroupBorderEdge = [&](const DA::DCEL::FEdge* InEdge) {
					check(IsGroupBorderEdge(InEdge));
					TSet<const DA::DCEL::FEdge*> Visited;
					Visited.Add(InEdge);

					DA::DCEL::FEdge* PrevEdge = InEdge->Prev;
					while (!Visited.Contains(PrevEdge)) {
						if (IsGroupBorderEdge(PrevEdge)) {
							break;
						}
						Visited.Add(PrevEdge);
						PrevEdge = PrevEdge->Twin->Prev;
					}

					return PrevEdge;
				};

				auto IsValidBorderEdge = [&](const DA::DCEL::FEdge* InEdge) {
					const DA::DCEL::FFace* TwinFace = InEdge->Twin->LeftFace;
					if (TwinFace && TwinFace->bValid) {
						if (const UFlowAbstractNode* TwinLayoutNode = GetLayoutNodeFromFaceId(TwinFace->FaceId)) {
							return TwinLayoutNode->bActive;
						}
					}
					return false;
				};
				
				auto GetNextActiveBorderEdge = [&](const DA::DCEL::FEdge* InEdge) {
					check(IsGroupBorderEdge(InEdge));
					TSet<const DA::DCEL::FEdge*> Visited;
					Visited.Add(InEdge);

					DA::DCEL::FEdge* NextEdge = InEdge->Next;
					while (!Visited.Contains(NextEdge)) {
						if (!IsValidBorderEdge(NextEdge)) {
							break;
						}
						Visited.Add(NextEdge);
						NextEdge = NextEdge->Twin->Next;
					}

					return NextEdge;
				};
				
				auto GetPrevActiveBorderEdge = [&](const DA::DCEL::FEdge* InEdge) {
					check(IsGroupBorderEdge(InEdge));
					TSet<const DA::DCEL::FEdge*> Visited;
					Visited.Add(InEdge);

					DA::DCEL::FEdge* PrevEdge = InEdge->Prev;
					while (!Visited.Contains(PrevEdge)) {
						if (!IsValidBorderEdge(PrevEdge)) {
							break;
						}
						Visited.Add(PrevEdge);
						PrevEdge = PrevEdge->Twin->Prev;
					}

					return PrevEdge;
				};

				struct FEdgeCliffTriInfo {
					int H0;
					int H1;
					TArray<int32> Triangles;
				};
				TMap<const DA::DCEL::FEdge*, FEdgeCliffTriInfo> EdgeCliffTriangles;
				DA::DCEL::TraverseFaceEdges(Face->Outer, [&](const DA::DCEL::FEdge* InEdge) {
					bool bGroupBorderEdge = IsGroupBorderEdge(InEdge);
					int LogicalTwinHeightZ = GlobalCliffDepth;
					const DA::DCEL::FFace* TwinFace = InEdge->Twin->LeftFace;

					if (bDrawLines) {
						DrawEdgeLine(InEdge, bGroupBorderEdge);
					}

					if (bGroupBorderEdge) {
						// Check if we have a stair along this edge
						if (const FDAFlowCellGraphDCELEdgeInfo* StairInfo = Stairs.Find(InEdge->Index)) {
							MeshGen.RenderStairs(StairInfo, InSettings.NumSubDiv, SurfaceGeometry);
						}

						// Adjust the cliff height
						if (TwinFace && TwinFace->bValid) {
							if (const UDAFlowCellLeafNode* TwinLayoutLeafNode = InCellGraph->LeafNodes[TwinFace->FaceId]) {
								if (const UFlowAbstractNode* TwinLayoutNode = GetLayoutNodeFromFaceId(TwinFace->FaceId)) {
									if (bRenderInactiveGroups || TwinLayoutNode->bActive) {
										LogicalTwinHeightZ = TwinLayoutLeafNode->LogicalZ;
									}
								}
							}
						}
					}

					// Tessellate the surface poly
					{
						const int32 SiteIdx = LeafNodeId;
						const FVector2d& A2 = Sites[SiteIdx];
						const FVector2d& B2 = InEdge->Twin->Origin->Location;
						const FVector2d& C2 = InEdge->Origin->Location;

						FVector3f A(A2.X, A2.Y, LogicalHeightZ);
						FVector3f B(B2.X, B2.Y, LogicalHeightZ);
						FVector3f C(C2.X, C2.Y, LogicalHeightZ);
						
						MeshGen.TessellatePoly({A, B, C}, FaceColor, InSettings.NumSubDiv, ECellFlowDCELMeshPatchType::Ground, SurfaceGeometry);
					}

					// Tessellate the cliff poly
					if (bGroupBorderEdge && LogicalHeightZ > LogicalTwinHeightZ) {
						const FVector2d Start2D = InEdge->Origin->Location;
						const FVector2d End2D = InEdge->Twin->Origin->Location;

						constexpr int HeightSteps = 1;
						int H1 = LogicalHeightZ;		// Upper
						int H0 = LogicalTwinHeightZ;	// Lower
						int ExtraHeightPadding = (H1 - H0) % HeightSteps;
						H0 -= ExtraHeightPadding;
						
						if (H0 != H1) {
							
							for (int H = H1; H > H0; H -= HeightSteps) {
								const FVector3f LocU0(Start2D.X, Start2D.Y, H);
								const FVector3f LocU1(End2D.X, End2D.Y, H);

								const FVector3f LocL0(Start2D.X, Start2D.Y, H - HeightSteps);
								const FVector3f LocL1(End2D.X, End2D.Y, H - HeightSteps);

								FEdgeCliffTriInfo& CliffTriInfo = EdgeCliffTriangles.FindOrAdd(InEdge);
								CliffTriInfo.H0 = H0;
								CliffTriInfo.H1 = H1;
								MeshGen.TessellatePoly({LocU0, LocU1, LocL1, LocL0},
									FaceColor, InSettings.NumSubDiv, ECellFlowDCELMeshPatchType::Cliff, CliffGeometry, &CliffTriInfo.Triangles);
							}
						}
					}
				});

				// Modify the cliff vertices
				{
					TSet<int32> VertexIndices;
					for (auto& Entry : EdgeCliffTriangles) {
						const DA::DCEL::FEdge* Edge = Entry.Key;
						FEdgeCliffTriInfo& CliffTriInfo = Entry.Value;
						for (int32 TriangleIndex : CliffTriInfo.Triangles) {
							UE::Geometry::FIndex3i& Triangle = CliffGeometry.Triangles[TriangleIndex];
							VertexIndices.Add(Triangle.A);
							VertexIndices.Add(Triangle.B);
							VertexIndices.Add(Triangle.C);
						}
					}

					/*
					for (auto& Entry : EdgeCliffTriangles) {
						const DA::DCEL::FEdge* Edge = Entry.Key;
						
						DA::DCEL::FEdge* PrevEdge = GetPrevActiveBorderEdge(Edge); // Edge->Twin->Next->Twin; // GetPrevGroupBorderEdge(Edge);
						DA::DCEL::FEdge* NextEdge = GetNextActiveBorderEdge(Edge); //Edge->Twin->Prev->Twin; // GetNextGroupBorderEdge(Edge);
						const FVector2d EdgeNormal = FVector2d(1, 0).GetRotated(FMath::RadiansToDegrees(Edge->Angle) + 90);
						const FVector2d PrevEdgeNormal = FVector2d(1, 0).GetRotated(FMath::RadiansToDegrees(PrevEdge->Angle) + 90);
						const FVector2d NextEdgeNormal = FVector2d(1, 0).GetRotated(FMath::RadiansToDegrees(NextEdge->Angle) + 90);
						FVector2d NormalStart = (PrevEdgeNormal + EdgeNormal).GetSafeNormal();
						FVector2d NormalEnd = (EdgeNormal + NextEdgeNormal).GetSafeNormal();
						FVector2d EdgeStartWS = Edge->Origin->Location * FVector2d(GridSize.X, GridSize.Y);
						FVector2d EdgeEndWS = Edge->Twin->Origin->Location * FVector2d(GridSize.X, GridSize.Y);
						double EdgeLength = (EdgeEndWS - EdgeStartWS).Length();
						if (EdgeLength < 1e-6f) { EdgeLength = 1; }

						auto ProcessEdgeCliffTriangles = [&](int VertexIdx, int H0, int H1) {
							if (ProcessedCliffVertices.Contains(VertexIdx)) {
								return;
							}
							ProcessedCliffVertices.Add(VertexIdx);
					
							constexpr float InwardDistance = 200;
									
							FFlowVisLib::FDAVertexInfo& Vertex = CliffGeometry.Vertices[VertexIdx];

							// Find the point where the vertex lies in the edge
							FVector2d OffsetNormal{};
							{
								const double LengthFromOrigin = (FVector2d(Vertex.Position.X, Vertex.Position.Y) - EdgeStartWS).Length();
								const double RatioOnEdge = LengthFromOrigin / EdgeLength;
								OffsetNormal = FMath::Lerp(NormalStart, NormalEnd, RatioOnEdge).GetSafeNormal();
							}

							float HF = Vertex.Position.Z / GridSize.Z;
							float T = (HF - H0) / static_cast<float>(H1 - H0);
							T = FMath::Clamp(T, 0, 1);
							float OffsetDistance = FMath::Sin(T * PI) * InwardDistance;
							FVector3d Offset = FVector3d(OffsetNormal.X, OffsetNormal.Y, 0) * OffsetDistance;
							Vertex.Position += Offset;
						};
						
						const FEdgeCliffTriInfo& CliffTriInfo = Entry.Value;
						const int32 H0 = CliffTriInfo.H0;
						const int32 H1 = CliffTriInfo.H1;
						for (int32 TriangleIndex : CliffTriInfo.Triangles) {
							UE::Geometry::FIndex3i& Triangle = CliffGeometry.Triangles[TriangleIndex];
							ProcessEdgeCliffTriangles(Triangle.A, H0, H1);
							ProcessEdgeCliffTriangles(Triangle.B, H0, H1);
							ProcessEdgeCliffTriangles(Triangle.C, H0, H1);
						}
					}
					*/
					
				}
			}
		}

		auto FnGetVertexHeightOffset = [&NoiseTable](const FVector2D& InLocation, int NumOctaves, float Scale, float AmplitudeMin, float AmplitudeMax) {
			const FVector2D NoiseLoc = FVector2D(InLocation.X, InLocation.Y) / Scale; 
			const float LocalNoise01 = NoiseTable.GetFbmNoise(NoiseLoc, NumOctaves);
			return AmplitudeMin + LocalNoise01 * (AmplitudeMax - AmplitudeMin);
		};

		if (InSettings.bApplyNoise) {
			const float NoiseScale = InSettings.NoiseSettings.NoiseScale;
			const float NoiseAmpMin = InSettings.NoiseSettings.NoiseAmplitudeMin;
			const float NoiseAmpMax = InSettings.NoiseSettings.NoiseAmplitudeMax;
			const int NumOctaves = InSettings.NoiseSettings.NumOctaves;
			for (FFlowVisLib::FDAVertexInfo& Vertex : SurfaceGeometry.Vertices) {
				Vertex.Position.Z += FnGetVertexHeightOffset(FVector2D(Vertex.Position.X, Vertex.Position.Y), NumOctaves, NoiseScale, NoiseAmpMin, NoiseAmpMax);
			}
			
			for (FFlowVisLib::FDAVertexInfo& Vertex : CliffGeometry.Vertices) {
				Vertex.Position.Z += FnGetVertexHeightOffset(FVector2D(Vertex.Position.X, Vertex.Position.Y), NumOctaves, NoiseScale, NoiseAmpMin, NoiseAmpMax);
			}
		}

		constexpr float UV_SCALE = 200;	// TODO: Parameterize me
		for (FFlowVisLib::FDAVertexInfo& Vertex : SurfaceGeometry.Vertices) {
			Vertex.UV0 = FVector2f(Vertex.Position.X, Vertex.Position.Y) / UV_SCALE;
		}
		for (FFlowVisLib::FDAVertexInfo& Vertex : CliffGeometry.Vertices) {
			Vertex.UV0 = FVector2f(Vertex.Position.X + Vertex.Position.Y, Vertex.Position.Z) / UV_SCALE;
		}
		
		FFlowVisLib::CalculateNormals(SurfaceGeometry.Triangles, SurfaceGeometry.Vertices);
		FFlowVisLib::CalculateNormals(CliffGeometry.Triangles, CliffGeometry.Vertices);
		
		FFlowVisLib::FGeometry& OutGeometry = InSettings.bGeneratedMergedMesh
				? OutGeometries[0]
				: OutGeometries.AddDefaulted_GetRef();

		FFlowVisLib::AppendGeometry(SurfaceGeometry, OutGeometry);
		FFlowVisLib::AppendGeometry(CliffGeometry, OutGeometry);
	}

	if (bDrawLines) {
		constexpr float LineWidth = 10;
		FFlowVisLib::EmitLine(GroupLines.GetLines(), LineWidth, *OutLineGeometry);
		FFlowVisLib::EmitLine(LeafLines.GetLines(), LineWidth, *OutLineGeometry);
	}
}

