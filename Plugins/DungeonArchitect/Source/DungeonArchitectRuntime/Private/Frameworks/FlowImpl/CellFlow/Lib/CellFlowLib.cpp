//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/Lib/FlowAbstractGraphPathUtils.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStats.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"

const FName UDAFlowCellGraph::StateTypeID = TEXT("CellGraphStateObject");

void UDAFlowCellGraph::CloneFromStateObject(const UObject* SourceObject) {
	const UDAFlowCellGraph* OtherGraph = Cast<UDAFlowCellGraph>(SourceObject);
	if (!OtherGraph) return;

	// Clone the group nodes. Since they are plain data objects, just copy them over
	GroupNodes = OtherGraph->GroupNodes;

	// Clone the leaf nodes
	LeafNodes.Reset();
	for (UDAFlowCellLeafNode* OtherLeafNode : OtherGraph->LeafNodes) {
		UDAFlowCellLeafNode* ClonedLeafNode = NewObject<UDAFlowCellLeafNode>(this, OtherLeafNode->GetClass(), NAME_None, RF_NoFlags, OtherLeafNode);
		LeafNodes.Add(ClonedLeafNode);
	}

	ScatterSettings = OtherGraph->ScatterSettings;
	
	GridInfo = OtherGraph->GridInfo;
	DCELInfo = OtherGraph->DCELInfo;
	SceneDebugData = OtherGraph->SceneDebugData;
	GlobalSettings = OtherGraph->GlobalSettings;
	
#if WITH_EDITORONLY_DATA
	bRenderInactiveGroups = OtherGraph->bRenderInactiveGroups;
#endif // WITH_EDITORONLY_DATA
}

namespace DA {
	void FCellGraphBuilder::CollapseEdges(UDAFlowCellGraph* InGraph, float MinGroupArea, float InClusterBlobbiness, const FRandomStream& Random, FCellAreaLookup& AreaLookup) {
		SCOPE_CYCLE_COUNTER(STAT_CFBuild_CollapseEdges);
		while (CollapsedBestGroupEdge(InGraph, MinGroupArea, InClusterBlobbiness, Random, AreaLookup)) {}
	}
	
	void FCellGraphBuilder::AssignGroupColors(UDAFlowCellGraph* InGraph) {
		TArray<int> ActiveGroupIds;
		for (const FDAFlowCellGroupNode &GroupNode: InGraph->GroupNodes) {
			if (GroupNode.IsActive()) {
				ActiveGroupIds.Add(GroupNode.GroupId);
			}
		}

		const int NumActiveGroups = ActiveGroupIds.Num();
		if (NumActiveGroups > 0) {
			const float AngleDelta = 360.0f / static_cast<float>(NumActiveGroups);
			for (int i = 0; i < NumActiveGroups; i++) {
				const float Hue = AngleDelta * i;
				const int GroupId = ActiveGroupIds[i];
				FLinearColor HSV(Hue, 0.3f, 1.0f);
					
				InGraph->GroupNodes[GroupId].GroupColor = HSV.HSVToLinearRGB();
			}
		}
	}

	bool FCellGraphBuilder::CollapsedBestGroupEdge(UDAFlowCellGraph* InGraph, float MinGroupArea, float InClusterBlobbiness, const FRandomStream& Random, FCellAreaLookup& AreaLookup) {
		int GroupAId;
		{
			SCOPE_CYCLE_COUNTER(STAT_CFColl_PrepNodeGroups);
			if (!AreaLookup.GetGroupWithLeastArea(Random, GroupAId)) {
				return false;
			}

			if (AreaLookup.GetGroupArea(GroupAId) > MinGroupArea) {
				// No need to merge any further
				return false;
			}
		}

		auto FindSharedEdgeCountBetweenGroups = [InGraph](int32 GroupAId, int32 GroupBId) {
			int32 NumSharedEdges{};
			const FDAFlowCellGroupNode& GroupA = InGraph->GroupNodes[GroupAId];
			const FDAFlowCellGroupNode& GroupB = InGraph->GroupNodes[GroupBId];
			const TSet<int>& GroupALeafs = GroupA.LeafNodes;
			const TSet<int>& GroupBLeafs = GroupB.LeafNodes;

			for (int GroupALeafIdx : GroupALeafs) {
				if (InGraph->LeafNodes.IsValidIndex(GroupALeafIdx)) {
					if (UDAFlowCellLeafNode* GroupALeaf = InGraph->LeafNodes[GroupALeafIdx]) {
						for (int32 AdjacentLeafA : GroupALeaf->AdjacentLeafs) {
							if (GroupBLeafs.Contains(AdjacentLeafA)) {
								NumSharedEdges++;
							}
						}
					}
				}
			}
			return NumSharedEdges;
		};
		
		const FDAFlowCellGroupNode& GroupA = InGraph->GroupNodes[GroupAId];

		int GroupBId = -1;
		TArray<int> ConnectedGroups;
		{
			SCOPE_CYCLE_COUNTER(STAT_CFColl_PrepConnGroups);
			TArray<int> ConnectionsA = GroupA.Connections.Array();
			if (ConnectionsA.Num() == 0) {
				return false;
			}

			const float NumEdgeMultiplier = 1.0f;
			float BestWeight = MAX_flt;
			int BestConnectedGroup = -1;
			for (const int ConnectedGroupID: ConnectionsA) {
				const float ConnectedGroupArea = AreaLookup.GetGroupArea(ConnectedGroupID);
				/*
				// Find the shared no. of edges
				int32 NumSharedEdges = FindSharedEdgeCountBetweenGroups(GroupAId, ConnectedGroupID);
				if (NumSharedEdges > 1) {
					UE_LOG(LogTemp, Log, TEXT("DEBUG >> Found more than 1"));
				}
				float Weight = ConnectedGroupArea * (NumSharedEdges * NumEdgeMultiplier);
				*/
				
				float Weight = ConnectedGroupArea;
				if (Weight < BestWeight) {
					BestWeight = ConnectedGroupArea;
					BestConnectedGroup = ConnectedGroupID;
				}
			}
			GroupBId = BestConnectedGroup;
		}

		check(GroupBId != -1);
		
		{
			SCOPE_CYCLE_COUNTER(STAT_CFColl_Merge);
			check(GroupAId != GroupBId);
			MergeGroups(InGraph, GroupAId, GroupBId, AreaLookup);
		}
		return true;
	}

	void FCellGraphBuilder::AssignGroupPreviewLocations(UDAFlowCellGraph* InGraph, const FCellAreaLookup& InAreaLookup) {
		SCOPE_CYCLE_COUNTER(STAT_CFBuild_AssignPreviewLoc);
		TArray<int> GroupCenterNode;
		GroupCenterNode.SetNum(InGraph->GroupNodes.Num());
		for (int i = 0; i < GroupCenterNode.Num(); i++) {
			GroupCenterNode[i] = -1;
		}
			
		for (int GroupId = 0; GroupId < InGraph->GroupNodes.Num(); GroupId++) {
			FDAFlowCellGroupNode& GroupNode = InGraph->GroupNodes[GroupId];
			if (GroupNode.LeafNodes.Num() > 0) {
				int BestLeafId = -1;
				
				FVector2D Center = FVector2D::ZeroVector;
				{
					float Sum = 0;
					for (const int LeafNodeId : GroupNode.LeafNodes) {
						const float Area = InAreaLookup.GetLeafArea(LeafNodeId); 
						Center += InGraph->LeafNodes[LeafNodeId]->GetCenter() * Area;
						Sum += Area;
					}
					Center /= Sum;
				}

				float BestDistance = MAX_flt;;
				for (const int LeafNodeId : GroupNode.LeafNodes) {
					const UDAFlowCellLeafNode* LeafNode = InGraph->LeafNodes[LeafNodeId];
					const float DistanceToCenter = (Center - LeafNode->GetCenter()).Size();
					if (DistanceToCenter < BestDistance) {
						BestDistance = DistanceToCenter;
						BestLeafId = LeafNodeId;
					}
				}
				GroupNode.PreviewLocation = InGraph->LeafNodes[BestLeafId]->GetCenter();
				GroupCenterNode[GroupId] = BestLeafId;
			}
		}
	}

	void FCellGraphBuilder::GenerateEdgeList(UDAFlowCellGraph* CellGraph, TArray<FCellFlowGridEdgeInfo>& HalfEdges,
			const FFlowAbstractGraphQuery& LayoutGraphQuery, bool bHandleInactiveGroups) {
		TMap<FIntPoint, int32> CellHeights;
		TMap<FIntPoint, int32> CellGroups;

		for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
			if (!GroupNode.IsActive() || !GroupNode.LayoutNodeID.IsValid()) continue;
			const UFlowAbstractNode* LayoutNode = LayoutGraphQuery.GetNode(GroupNode.LayoutNodeID);
			if (!LayoutNode) {
				continue;
			}
			
			if (!bHandleInactiveGroups && !LayoutNode->bActive) {
				// Do not handle this inactive node
				continue;
			}
			
			for (const int LeafNodeId : GroupNode.LeafNodes) {
				if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeId])) {
					for (const FIntPoint& LeafTileCoord : GridLeafNode->LeafTileCoords) {
						int32& HeightRef = CellHeights.FindOrAdd(LeafTileCoord);
						HeightRef = GridLeafNode->LogicalZ;

						int32& GroupRef = CellGroups.FindOrAdd(LeafTileCoord);
						GroupRef = GroupNode.GroupId;
					}
				}
			}
		}
		
		TSet<FIntVector4> HandleEdgeVisited;
		auto HandleEdge = [&CellGroups, &CellHeights, &HalfEdges, &HandleEdgeVisited](int X, int Y, int Z, int dx, int dy) {
			const FIntPoint Coord(X, Y);
			const FIntPoint CoordNeighbor(X + dx, Y + dy);
			const int GroupBase = CellGroups[Coord];
			const int* GroupNextPtr = CellGroups.Find(CoordNeighbor);
			const int GroupNext = GroupNextPtr ? *GroupNextPtr : INDEX_NONE;
			if (HandleEdgeVisited.Contains(FIntVector4(Coord.X, Coord.Y, CoordNeighbor.X, CoordNeighbor.Y))) {
				return;
			}

			// Place the key in the other direction so the twin can skip it later
			HandleEdgeVisited.Add(FIntVector4(CoordNeighbor.X, CoordNeighbor.Y, Coord.X, Coord.Y));
		
			if (GroupBase != GroupNext) {
				const int EdgeIdx = HalfEdges.Num();
				const int TwinEdgeIdx = HalfEdges.Num() + 1;
			
				FCellFlowGridEdgeInfo Edge;
				Edge.Coord = Coord;
				Edge.TileGroup = GroupBase;
				Edge.HeightZ = CellHeights[Coord];
				Edge.EdgeIndex = EdgeIdx;
			
				FCellFlowGridEdgeInfo TwinEdge;
				TwinEdge.Coord = CoordNeighbor;
				TwinEdge.TileGroup = GroupNext;
				TwinEdge.HeightZ = (GroupNext != INDEX_NONE)
					? CellHeights[CoordNeighbor]
					: Edge.HeightZ;
				TwinEdge.EdgeIndex = TwinEdgeIdx;
			
				Edge.TwinIndex = TwinEdgeIdx;
				TwinEdge.TwinIndex = EdgeIdx;

				HalfEdges.Add(MoveTemp(Edge));
				HalfEdges.Add(MoveTemp(TwinEdge));
			}
		};
	
		for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
			if (!GroupNode.IsActive() || !GroupNode.LayoutNodeID.IsValid()) continue;
			const UFlowAbstractNode* LayoutNode = LayoutGraphQuery.GetNode(GroupNode.LayoutNodeID);
			if (!LayoutNode) {
				continue;
			}
			
			if (!bHandleInactiveGroups && !LayoutNode->bActive) {
				// Do not handle this inactive node
				continue;
			}

			static const FIntPoint NeighborOffsets[4] = {
				FIntPoint(-1, 0),
				FIntPoint(1, 0),
				FIntPoint(0, -1),
				FIntPoint(0, 1),
			};
			
			for (const int LeafNodeId : GroupNode.LeafNodes) {
				if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeId])) {
					const int Z = GridLeafNode->LogicalZ;
					TSet<FIntPoint> LeafCoordsMap(GridLeafNode->LeafTileCoords);
					for (const FIntPoint& TileCoord : GridLeafNode->LeafTileCoords) {
						for (int N = 0; N < 4; ++N) {
							const FIntPoint& Offset = NeighborOffsets[N];
							if (!LeafCoordsMap.Contains(TileCoord + Offset)) {
								HandleEdge(TileCoord.X, TileCoord.Y, Z, Offset.X, Offset.Y);
							}
						}
					}
				}
			}
		}
	}

	void FCellGraphBuilder::MergeGroups(UDAFlowCellGraph* InGraph, int GroupIdA, int GroupIdB, FCellAreaLookup& AreaLookup) {
		FDAFlowCellGroupNode& GroupA = InGraph->GroupNodes[GroupIdA];
		FDAFlowCellGroupNode& GroupB = InGraph->GroupNodes[GroupIdB];

		// Copy B to A and discard B
		GroupA.LeafNodes.Append(GroupB.LeafNodes);
		GroupB.LeafNodes.Reset();

		// Break all GroupB connections
		for (int ConnectedToB : GroupB.Connections) {
			InGraph->GroupNodes[ConnectedToB].Connections.Remove(GroupIdB);
			InGraph->GroupNodes[ConnectedToB].Connections.Add(GroupIdA);
			GroupA.Connections.Add(ConnectedToB);
		}
		GroupB.Connections.Reset();

		// Remove internal links (merged nodes do not connect to each other)
		TArray<int> ValidConnectionsA;
		for (int ConnectedCellID : GroupA.Connections) {
			if (!GroupA.LeafNodes.Contains(ConnectedCellID)) {
				ValidConnectionsA.Add(ConnectedCellID);
			}
		}
		GroupA.Connections = TSet(ValidConnectionsA);

		// Copy GroupB's area over ot A and then clear it on B
		const float MergedArea = AreaLookup.GetGroupArea(GroupIdA) + AreaLookup.GetGroupArea(GroupIdB); 
		AreaLookup.SetGroupArea(GroupIdA, MergedArea);
		AreaLookup.SetGroupArea(GroupIdB, 0);
	}

	bool FCellFlowLib::SelectRandomWeightedMarker(const TArray<FCellFlowSizedMarkerDef>& InMarkers, const FRandomStream& InRandomStream, FCellFlowSizedMarkerDef& OutRandomMarker) {
		if (InMarkers.Num() == 0)
		{
			OutRandomMarker = {};
			return false;
		}
	
		// Calculate total weight
		float TotalWeight = 0.0f;
		for (const FCellFlowSizedMarkerDef& Marker : InMarkers)
		{
			TotalWeight += Marker.SelectionWeight;
		}
	
		// If all weights are zero, return empty
		if (TotalWeight <= 0.0f)
		{
			OutRandomMarker = {};
			return false;
		}
	
		// Generate random value between 0 and TotalWeight
		float RandomValue = InRandomStream.FRand() * TotalWeight;
	
		// Find the selected marker based on cumulative weights
		float CumulativeWeight = 0.0f;
		for (const FCellFlowSizedMarkerDef& Marker : InMarkers)
		{
			CumulativeWeight += Marker.SelectionWeight;
			if (RandomValue <= CumulativeWeight)
			{
				OutRandomMarker = Marker;
				return true;
			}
		}
	
		// Fallback (should not reach here, but return last element just in case)
		OutRandomMarker = InMarkers.Last();
		return false;
	}

	void FCellFlowLib::AssignGroupHeights(UDAFlowCellGraph* CellGraph, const TSet<FGuid>& LayoutNodeIds, int32 CeilingHeightMin, int32 CeilingHeightMax, const FFlowAbstractGraphQuery& LayoutGraphQuery, const FRandomStream& Random) {
		for (FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
			if (!LayoutNodeIds.Contains(GroupNode.LayoutNodeID)) {
				continue;
			}
			
			UFlowAbstractNode* LayoutNode = LayoutGraphQuery.GetNode(GroupNode.LayoutNodeID);
			if (LayoutNode && !LayoutNode->IsA<UFlowAbstractDoorNode>()) {
				int32 ClampedCeilingHeightMin = FMath::Clamp(CeilingHeightMin, 1, 10);
				int32 ClampedCeilingHeightMax = FMath::Clamp(CeilingHeightMax, 1, 10);
				const int32 MinCHeight = FMath::Min(ClampedCeilingHeightMin, ClampedCeilingHeightMax);
				const int32 MaxCHeight = FMath::Max(ClampedCeilingHeightMin, ClampedCeilingHeightMax);
				int32 GroupHeight = Random.RandRange(MinCHeight, MaxCHeight);
				GroupNode.GroupHeight = GroupHeight;
			}
		}
	}


	////////////////////////////// FCellAreaLookup //////////////////////////////
	void FCellAreaLookup::Init(UDAFlowCellGraph* InGraph) {
		LeafAreas.SetNum(InGraph->LeafNodes.Num());
		for (int LeafId = 0; LeafId < InGraph->LeafNodes.Num(); LeafId++) {
			LeafAreas[LeafId] = InGraph->LeafNodes[LeafId]->GetArea();
		}

		GroupAreas.SetNum(InGraph->GroupNodes.Num());
		for (int GroupId = 0; GroupId < InGraph->GroupNodes.Num(); GroupId++) {
			FDAFlowCellGroupNode& GroupNode = InGraph->GroupNodes[GroupId];
			if (GroupNode.Connections.Num() == 0 || GroupNode.LeafNodes.Num() == 0) {
				continue;
			}
			
			float& GroupArea = GroupAreas[GroupId];
			GroupArea = 0;
			const TSet<int>& LeafNodeIds = GroupNode.LeafNodes;
			for (const int LeafId : LeafNodeIds) {
				GroupArea += LeafAreas[LeafId];
			}
			if (GroupArea > 0) {
				TArray<int>& Groups = ActiveGroupIdsByArea.FindOrAdd(GroupArea);
				Groups.Add(GroupId);
			}
		}
	}

	void FCellAreaLookup::SetGroupArea(int GroupId, float NewArea) {
		const float OldArea = GroupAreas[GroupId];
		GroupAreas[GroupId] = NewArea;

		TArray<int>& OldAreaGroups = ActiveGroupIdsByArea.FindOrAdd(OldArea);
		OldAreaGroups.Remove(GroupId);
		if (OldAreaGroups.Num() == 0) {
			ActiveGroupIdsByArea.Remove(OldArea);
		}

		if (NewArea > 0) {
			TArray<int>& NewAreaGroups = ActiveGroupIdsByArea.FindOrAdd(NewArea);
			check(!NewAreaGroups.Contains(GroupId));
			NewAreaGroups.Add(GroupId);
		}
	}

	bool FCellAreaLookup::GetGroupWithLeastArea(const FRandomStream& InRandom, int& OutGroupId) const {
		float BestArea = MAX_flt;
		bool bFound = false;
		for (const auto& Entry : ActiveGroupIdsByArea) {
			const float Area = Entry.Key;
			const TArray<int>& GroupIds = Entry.Value;
			if (GroupIds.Num() == 0) {
				continue;
			}

			if (Area < BestArea) {
				BestArea = Area;
				bFound = true;
			}
		}

		if (!bFound) {
			OutGroupId = -1;
			return false;
		}

		const TArray<int>& Candidates = ActiveGroupIdsByArea[BestArea];
		OutGroupId = Candidates[InRandom.RandRange(0, Candidates.Num() - 1)];
		return true;
	}
}

