//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLibVoronoi.h"

#include "Core/Dungeon.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowUtils.h"

bool FCellFlowLibVoronoi::IsGroupBorderEdge(const FDAFlowCellGroupNode& GroupNode, const DA::DCEL::FEdge* InEdge) {
	const DA::DCEL::FFace* TwinFace = InEdge->Twin->LeftFace;
	if (TwinFace && TwinFace->bValid) {
		if (GroupNode.LeafNodes.Contains(TwinFace->FaceId)) {
			// Belongs to the same group
			return false;
		}
	}
	return true;
}

bool FCellFlowLibVoronoi::IsDungeonBorderEdge(const FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* InEdge) {
	const DA::DCEL::FFace* TwinFace = InEdge->Twin->LeftFace;
	if (TwinFace && TwinFace->bValid) {
		if (const UFlowAbstractNode* TwinLayoutNode = GetLayoutNodeFromFaceId(EmitMarkerContext, TwinFace->FaceId)) {
			return !TwinLayoutNode->bActive;
		}
	}
	return true;
}

FCellFlowLibVoronoi::ECellFlowEdgeType FCellFlowLibVoronoi::GetEdgeType(const FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* InEdge) {
	ECellFlowEdgeType EdgeType{ECellFlowEdgeType::Unknown};
	if (IsDungeonBorderEdge(EmitMarkerContext, InEdge)) {
		EdgeType = ECellFlowEdgeType::BoundaryEdge;
	}
	else {
		const DA::DCEL::FFace* Face = InEdge->LeftFace;
		const DA::DCEL::FFace* TwinFace = InEdge->Twin->LeftFace;
		check(Face && TwinFace && Face->bValid && TwinFace->bValid);

		const UDAFlowCellGraph* CellGraph = EmitMarkerContext.CellGraph;
		const UDAFlowCellLeafNode* LeafNode = CellGraph->LeafNodes.IsValidIndex(Face->FaceId) ? CellGraph->LeafNodes[Face->FaceId] : nullptr;
		const UDAFlowCellLeafNode* TwinLeafNode = CellGraph->LeafNodes.IsValidIndex(TwinFace->FaceId) ? CellGraph->LeafNodes[TwinFace->FaceId] : nullptr;
		if (LeafNode && TwinLeafNode) {
			if (LeafNode->LogicalZ > TwinLeafNode->LogicalZ) {
				EdgeType = ECellFlowEdgeType::SharedEdgeHigh;
			}
			else if (LeafNode->LogicalZ < TwinLeafNode->LogicalZ) {
				EdgeType = ECellFlowEdgeType::SharedEdgeLow;
			}
			else {
				EdgeType = ECellFlowEdgeType::SharedEdgeSameHeight;
			}
		}
	}
	return EdgeType;
}

const UFlowAbstractNode* FCellFlowLibVoronoi::GetLayoutNodeFromFaceId(const FEmitMarkersContext& EmitMarkerContext, int FaceId) {
	if (const int* FlowGroupIdPtr = EmitMarkerContext.LeafToGroupMap.Find(FaceId)) {
		const int FlowGroupId = *FlowGroupIdPtr;
		const FDAFlowCellGroupNode& FlowGroupNode = EmitMarkerContext.CellGraph->GroupNodes[FlowGroupId];
		if (const UFlowAbstractNode* const* FlowLayoutNodePtr = EmitMarkerContext.LayoutNodes.Find(FlowGroupNode.LayoutNodeID)) {
			return *FlowLayoutNodePtr;
		}
	}
	return nullptr;
}

FTransform FCellFlowLibVoronoi::GetEdgeMarkerTransform(const DA::DCEL::FEdge* InEdge, const FVector& GridSize,
		const UDAFlowCellLeafNode* LeafNode, float StartOffset, float MarkerScale, const FCellFlowSizedMarkerDef& MarkerDef, bool bCenterOnOcclusionBounds) {
	const FVector Start = FVector(InEdge->Origin->Location, LeafNode->LogicalZ) * GridSize;
	const FVector End = FVector(InEdge->Twin->Origin->Location, LeafNode->LogicalZ) * GridSize;
	const float Offset = StartOffset + MarkerDef.Size * 0.5f * MarkerScale;	// Get this at the center
	const FVector EdgeDirection = (End - Start).GetSafeNormal();
	FVector Location = Start + EdgeDirection * Offset;
	const FQuat Rotation = EdgeDirection.ToOrientationQuat();
	const FVector Scale = MarkerDef.bAutoScaleUniformly
		                      ? FVector(MarkerScale, MarkerScale, MarkerScale)
		                      : FVector(MarkerScale, 1, 1);

	if (MarkerDef.bOccludes && bCenterOnOcclusionBounds) {
		const float HalfOcclusionDepth = MarkerDef.OcclusionDepth * 0.5;
		const float EdgeLeftAngle = InEdge->Angle + PI * 0.5f;
		const FVector EdgeLeftDir(
			FMath::Cos(EdgeLeftAngle),
			FMath::Sin(EdgeLeftAngle),
			0);

		const FVector CenterOffset = EdgeLeftDir * HalfOcclusionDepth * MarkerScale;
		Location += CenterOffset;
	}

	return FTransform(Rotation, Location, Scale);
}

void FCellFlowLibVoronoi::EmitEdgeMarkers(FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* Edge, const  TFuncEmitMarker& EmitMarker,
		const TFuncEmitVoxelMarker& EmitVoxelMarker, const FString& VoxelMarkerName, float StartT, float EndT) {
	const int32 LeafNodeId = Edge->LeftFace->FaceId;
	ECellFlowEdgeType EdgeType = GetEdgeType(EmitMarkerContext, Edge);
		
	if (const int32* GroupNodeIdPtr = EmitMarkerContext.LeafToGroupMap.Find(LeafNodeId)) {
		const int32 GroupNodeId = *GroupNodeIdPtr;
		TArray<FDAShapePolygon>& ExistingGroupShapes = EmitMarkerContext.GroupFaceShapes.FindOrAdd(GroupNodeId);
		if (EmitMarkerContext.CellGraph->LeafNodes.IsValidIndex(LeafNodeId)) {
			const UDAFlowCellLeafNode* LeafNode = EmitMarkerContext.CellGraph->LeafNodes[LeafNodeId];
			const FVector Start = FVector(Edge->Origin->Location, LeafNode->LogicalZ) * EmitMarkerContext.GridSize;
			const FVector End = FVector(Edge->Twin->Origin->Location, LeafNode->LogicalZ) * EmitMarkerContext.GridSize;
			const FVector EdgeDirection = (End - Start).GetSafeNormal();
			const float EdgeLength = (End - Start).Size();
			
			if (const FCellFlowVoronoiMarkerSetup* MarkerSetup = EmitMarkerContext.GroupNodeChunkMarkers.Find(GroupNodeId)) {
				const TSet<const DA::DCEL::FEdge*>& GroupBoundaryEdges = EmitMarkerContext.GroupBoundaryEdges.FindOrAdd(GroupNodeId);
				
				for (const FCellFlowVoronoiEdgeMarkerLayer& EdgeLayer : MarkerSetup->EdgeLayers) {
					bool bValidLayer = (EdgeLayer.bBoundaryEdges && EdgeType == ECellFlowEdgeType::BoundaryEdge)
							|| (EdgeLayer.bSharedEdgesHigher && EdgeType == ECellFlowEdgeType::SharedEdgeHigh)
							|| (EdgeLayer.bSharedEdgesLower && EdgeType == ECellFlowEdgeType::SharedEdgeLow)
							|| (EdgeLayer.bSharedEdgesSameHeight && EdgeType == ECellFlowEdgeType::SharedEdgeSameHeight)
						;

					if (bValidLayer && EdgeType == ECellFlowEdgeType::SharedEdgeSameHeight && !EdgeLayer.bEnableTwoSidedSameHeightEdge) {
						// Pick the face with the smaller face id
						const int32 FaceId = Edge->LeftFace ? Edge->LeftFace->FaceId : 0;
						const int32 TwinFaceId = Edge->Twin->LeftFace ? Edge->Twin->LeftFace->FaceId : 0;
						if (FaceId < TwinFaceId) {
							bValidLayer = false;
						}
					}
						
					if (!bValidLayer) {
						continue;
					}
						
					const TArray<FCellFlowSizedMarkerDef>& EdgeMarkers = EdgeLayer.EdgeMarkers;
					if (EdgeMarkers.Num() > 0) {
						auto CreateEdgeMarkerShapePoly = [&EmitMarkerContext, LeafNode](const DA::DCEL::FEdge* InEdge, float StartOffset, float MarkerScale, const FCellFlowSizedMarkerDef& MarkerDef) {
							const float OcclusionDepth = MarkerDef.bOccludes ? MarkerDef.OcclusionDepth : 0;
							const FTransform MarkerTransform = GetEdgeMarkerTransform(InEdge, EmitMarkerContext.GridSize, LeafNode, StartOffset, MarkerScale, MarkerDef, true);
							const FVector BoxExtent(MarkerDef.Size * 0.5f, OcclusionDepth * 0.5f, 0);
							FDAShapePolygon CandidateShapePoly;
							FDAShapeCollision::ConvertBoxToConvexPoly(MarkerTransform, BoxExtent, CandidateShapePoly);
							CandidateShapePoly.Height = 200;	// TODO: Parameterize me.  Setting this to zero treats it as non-overlapping shapes
							return CandidateShapePoly;
						};

						struct FMarkerIndexToAdd {
							int32 MarkerIndex = INDEX_NONE;
							float Size{};
						};
						TArray<FMarkerIndexToAdd> MarkerIndicesToAdd;

						// Randomly pack the markers along the edge length
						{
							struct FBestFitInfo {
								int32 BestIndex = INDEX_NONE;
								float BestDistance = MAX_flt;
								void Update(int32 Index, float Distance) {
									if (Distance < BestDistance) {
										BestDistance = Distance;
										BestIndex = Index; 
									}
								}
							};
					
							float RemainingLength = EdgeLength * (EndT - StartT);
							float StartOffset = EdgeLength * StartT;
							while (RemainingLength > 1e-4f) {
								int32 SelectedMarkerIndex = INDEX_NONE;
								TArray<int32> CandidateIndices;
								float TotalWeight = 0;
								float BestFallbackSkipDistance = 1e6f;
								bool bFillUpOccludedEmptySpace{};
								for (int MarkerIdx = 0; MarkerIdx < EdgeMarkers.Num(); MarkerIdx++) {
									const FCellFlowSizedMarkerDef& MarkerDef = EdgeMarkers[MarkerIdx];
									bool bValidCandidate{};
									if (MarkerDef.Size < BestFallbackSkipDistance) {
										BestFallbackSkipDistance = MarkerDef.Size;
									}
									
									if (MarkerDef.Size <= RemainingLength) {
										bValidCandidate = true;
									}
									else if (MarkerDef.bAllowAutoScaling) {
										const float RemainingRatio = RemainingLength / MarkerDef.Size;
										if (RemainingRatio >= MarkerDef.MinAutoScalingAllowed) {
											bValidCandidate = true;
										}
										else {
											bFillUpOccludedEmptySpace = true;
										}
									}

									if (MarkerDef.bOccludes) {
										constexpr float MarkerScale = 1.0f;
										FDAShapePolygon CandidateShapePoly = CreateEdgeMarkerShapePoly(Edge, StartOffset, MarkerScale, MarkerDef);
									
										if (bValidCandidate) {
											// Check if it collides with all the other existing shapes
											for (const FDAShapePolygon& ExistingGroupShape : ExistingGroupShapes) {
												if (FDAShapeCollision::Intersects(CandidateShapePoly, ExistingGroupShape, 10)) {
													bValidCandidate = false;
													break;
												}
											}
										}

										if (bValidCandidate) {
											// Check if the shape collides with any of the boundary edges
											for (const DA::DCEL::FEdge* GroupBoundaryEdge : GroupBoundaryEdges) {
												FDAShapeLine ShapeLine;
												ShapeLine.Height = CandidateShapePoly.Height;
												ShapeLine.Transform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, EmitMarkerContext.GridSize);
												ShapeLine.LineStart = GroupBoundaryEdge->Origin->Location;
												ShapeLine.LineEnd = GroupBoundaryEdge->Twin->Origin->Location;
											
												if (FDAShapeCollision::Intersects(CandidateShapePoly, ShapeLine, 10)) {
													bValidCandidate = false;
													break;
												}
											}
										}

										if (bValidCandidate) {
											// Check if the shape collides with any of the stair occlusion bounds
											for (const auto& Entry : EmitMarkerContext.StairGenInfoByEdge) {
												const FStairGenInfo& StairGenInfo = Entry.Value;
												if (FDAShapeCollision::Intersects(CandidateShapePoly, StairGenInfo.OcclusionShape, 10)) {
													bValidCandidate = false;
													break;
												}
											}
										}
									}
								
									if (bValidCandidate) {
										CandidateIndices.Add(MarkerIdx);
										TotalWeight += MarkerDef.SelectionWeight;
									}

									const float Rand01 = EmitMarkerContext.Random ? EmitMarkerContext.Random->FRand() : 0;
									float RandomWeightPosition = Rand01 * TotalWeight;
									for (int32 CandidateIndex : CandidateIndices) {
										const FCellFlowSizedMarkerDef& CandidateMarkerDef = EdgeMarkers[CandidateIndex];
										if (RandomWeightPosition <= CandidateMarkerDef.SelectionWeight) {
											SelectedMarkerIndex = CandidateIndex;
											break;
										}
										RandomWeightPosition -= CandidateMarkerDef.SelectionWeight;
									}
								}

								if (SelectedMarkerIndex != INDEX_NONE) {
									const float MarkerSize = EdgeMarkers[SelectedMarkerIndex].Size;
									MarkerIndicesToAdd.Add({ SelectedMarkerIndex, MarkerSize });
									RemainingLength -= MarkerSize;
									StartOffset += MarkerSize;
								}
								else {
									if (!bFillUpOccludedEmptySpace) {
										MarkerIndicesToAdd.Add({ SelectedMarkerIndex, BestFallbackSkipDistance });
									}
									RemainingLength -= BestFallbackSkipDistance;
									StartOffset += BestFallbackSkipDistance;
								}
							}
						}
					
						// Scale all the scalable markers up or down to fit the entire edge length
						float ScalableMarkerGlobalScale = 1.0f;
						{
							float ScalableLength{};
							float NonScalableLength{};
							for (const FMarkerIndexToAdd& MarkerIdx : MarkerIndicesToAdd) {
								if (EdgeMarkers.IsValidIndex(MarkerIdx.MarkerIndex)) {
									const FCellFlowSizedMarkerDef& MarkerDef = EdgeMarkers[MarkerIdx.MarkerIndex];
									if (MarkerDef.bAllowAutoScaling) {
										ScalableLength += MarkerDef.Size;
									}
									else {
										NonScalableLength += MarkerDef.Size;
									}
								}
								else {
									ScalableLength += MarkerIdx.Size;
								}
							}

							if (ScalableLength > 0) {
								float TotalScalableLength = (EdgeLength * (EndT - StartT)) - NonScalableLength;
								ScalableMarkerGlobalScale = TotalScalableLength / ScalableLength;
							}
						}
					
						// Insert the markers into the scene
						float StartOffset = EdgeLength * StartT;
						for (const FMarkerIndexToAdd& MarkerIdx : MarkerIndicesToAdd) {
							float MarkerScale = 1.0f;
							if (EdgeMarkers.IsValidIndex(MarkerIdx.MarkerIndex)) {
								const FCellFlowSizedMarkerDef& MarkerDef = EdgeMarkers[MarkerIdx.MarkerIndex];
								if (MarkerDef.bAllowAutoScaling) {
									MarkerScale = ScalableMarkerGlobalScale;
								}
						
								FDAShapePolygon ShapePoly = CreateEdgeMarkerShapePoly(Edge, StartOffset, MarkerScale, MarkerDef);
								ExistingGroupShapes.Add(ShapePoly);
								
								const FTransform MarkerTransform = GetEdgeMarkerTransform(Edge, EmitMarkerContext.GridSize, LeafNode, StartOffset, MarkerScale, MarkerDef, false);
								EmitMarker(MarkerDef, MarkerTransform, ShapePoly.Transform);
								EmitVoxelMarker(VoxelMarkerName, MarkerTransform);
							}
							else {
								MarkerScale = ScalableMarkerGlobalScale;
							}
							
							StartOffset += MarkerIdx.Size * MarkerScale;
						}
					}
				}
			}
		}
	}
}

void FCellFlowLibVoronoi::EmitDoorEdgeMarkers(FEmitMarkersContext& EmitMarkerContext, const DA::DCEL::FEdge* Edge, const TFuncEmitMarker& EmitMarker, const TFuncEmitVoxelMarker& EmitVoxelMarker, const FString& VoxelMarkerName) {
	const double EdgeWidth = (Edge->Origin->Location - Edge->Twin->Origin->Location).Size();
	const double DoorWidth = FMath::Min(EdgeWidth, 1.0f);

	if (EdgeWidth < 1e-4f) {
		return;
	}
	
	constexpr float LocalEdgeWidth = 1.0f;
	const float LocalDoorWidth = DoorWidth / EdgeWidth;
	const float TotalLocalRemainingSpace = LocalEdgeWidth - LocalDoorWidth;
	const float LocalRemainingSpacePerSide = TotalLocalRemainingSpace * 0.5f;

	EmitEdgeMarkers(EmitMarkerContext, Edge, EmitMarker, EmitVoxelMarker, VoxelMarkerName, 0, LocalRemainingSpacePerSide);
	EmitEdgeMarkers(EmitMarkerContext, Edge, EmitMarker, EmitVoxelMarker, VoxelMarkerName, 1 - LocalRemainingSpacePerSide, 1);
}

void FCellFlowLibVoronoi::InitMarkerContext(const UDAFlowCellGraph* InCellGraph, const DA::DCELGraph& InDCELGraph, UCellFlowLayoutGraph* InLayoutGraph,
                                            const UCellFlowConfigMarkerSettings* InMarkerSettings, const FVector& InGridSize, FRandomStream& InRandom,
                                            FCellFlowLibVoronoi::FEmitMarkersContext& OutEmitMarkerContext)
{
	OutEmitMarkerContext = { InCellGraph, InGridSize, &InRandom };
		
	const FFlowAbstractGraphQuery GraphQuery(InLayoutGraph);
	if (InMarkerSettings) {
		FCellFlowUtils::GenerateGroupNodeMarkerSetup(InMarkerSettings->DefaultVoronoiPathMarkers,
				InMarkerSettings->VoronoiPathMarkers, InCellGraph->GroupNodes, GraphQuery, InRandom, OutEmitMarkerContext.GroupNodeChunkMarkers);
	}
	
	for (const UFlowAbstractNode* GraphNode : InLayoutGraph->GraphNodes) {
		if (GraphNode->bActive) {
			const UFlowAbstractNode*& NodeRef = OutEmitMarkerContext.LayoutNodes.FindOrAdd(GraphNode->NodeId);
			NodeRef = GraphNode;
		}
	}

	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			OutEmitMarkerContext.LeafToGroupMap.Add(LeafNodeId, GroupNode.GroupId);
		}
	}

	for (const DA::DCEL::FEdge* Edge : InDCELGraph.GetEdges()) {
		const int32 LeafFaceID = (Edge && Edge->LeftFace) ? Edge->LeftFace->FaceId : INDEX_NONE;
		const int32 TwinLeafFaceID = (Edge && Edge->Twin && Edge->Twin->LeftFace) ? Edge->Twin->LeftFace->FaceId : INDEX_NONE;

		const int32* GroupIDPtr = OutEmitMarkerContext.LeafToGroupMap.Find(LeafFaceID);
		if (!GroupIDPtr) {
			continue;
		}
		
		const int32* TwinGroupIDPtr = OutEmitMarkerContext.LeafToGroupMap.Find(TwinLeafFaceID);
		if (!TwinGroupIDPtr || *GroupIDPtr != *TwinGroupIDPtr) {
			TSet<const DA::DCEL::FEdge*>& GroupBoundaryEdgeSet = OutEmitMarkerContext.GroupBoundaryEdges.FindOrAdd(*GroupIDPtr);
			GroupBoundaryEdgeSet.Add(Edge);
		}
	}

	for (const auto& Entry : InCellGraph->DCELInfo.Stairs) {
		int EdgeIdx = Entry.Key;
		const FDAFlowCellGraphDCELEdgeInfo& StairInfo = Entry.Value;

		FCellFlowLibVoronoi::FStairGenInfo& StairGenInfo = OutEmitMarkerContext.StairGenInfoByEdge.FindOrAdd(EdgeIdx);
		StairGenInfo.StairInfo = StairInfo;

		// Generate the marker transform
		FRotator StairRotation = StairInfo.Direction.Rotation();
		FVector StairLocation = (StairInfo.LogicalLocation + StairInfo.Direction * 0.5f) * InGridSize;
		FVector StairScale = FVector(1, StairInfo.LogicalWidth, 1);
		StairGenInfo.MarkerTransform = FTransform(StairRotation, StairLocation, StairScale);

		// Generate the occlusion shape
		{
			constexpr float StairEntryExitOcclusion = 0.25f;
			constexpr float ExpandBounds = 1.25f;
			StairGenInfo.OcclusionBoxExtents = FVector(1 + StairEntryExitOcclusion * 2, 1, 1) * (InGridSize * ExpandBounds) * 0.5;
			StairGenInfo.OcclusionTransform = FTransform(
				StairRotation,
				StairLocation + FVector(0, 0, InGridSize.Z * 0.5f),
				StairScale);
			FDAShapeCollision::ConvertBoxToConvexPoly(StairGenInfo.OcclusionTransform, StairGenInfo.OcclusionBoxExtents, StairGenInfo.OcclusionShape);
		}
	}
}

