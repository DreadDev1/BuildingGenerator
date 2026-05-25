//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/CellFlow/CellFlowModel.h"

#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractNode.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowUtils.h"

void UCellFlowModel::Reset() {
	Super::Reset();
	LayoutGraph = {};
	CellGraph = {};
	VoronoiData = {};
}

FDungeonFloorSettings UCellFlowModel::CreateFloorSettings(const UDungeonConfig* InConfig) const {
	FDungeonFloorSettings Settings = {};
	if (const UCellFlowConfig* CellConfig = Cast<UCellFlowConfig>(InConfig)) {
		Settings.FloorHeight = CellConfig->GridSize.Z;
		Settings.FloorCaptureHeight = Settings.FloorHeight - 1;

		if (CellGraph && CellGraph->LeafNodes.Num() > 0) {
			int32 MinZ{};
			int32 MaxZ{};
			MinZ = MaxZ = CellGraph->LeafNodes[0]->LogicalZ;

			for (UDAFlowCellLeafNode* LeafNode : CellGraph->LeafNodes) {
				MinZ = FMath::Min(MinZ, LeafNode->LogicalZ);
				MaxZ = FMath::Max(MaxZ, LeafNode->LogicalZ);
			}

			Settings.FloorLowestIndex = MinZ;
			Settings.FloorHighestIndex = MaxZ;
		}
	}
	return Settings;
}

void UCellFlowModel::GenerateLayoutDataImpl(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const {
	if (!LayoutGraph || !CellGraph) {
		return;
	}
	
	const UCellFlowConfig* CellFlowConfig = Cast<const UCellFlowConfig>(InConfig);
	if (!CellFlowConfig) {
		return;
	}

	const FTransform LocalTransform = FTransform::Identity;
	const FVector& GridSize = CellFlowConfig->GridSize;
	
	TMap<FGuid, const UFlowAbstractNode*> LayoutNodes;
	for (const UFlowAbstractNode* GraphNode : LayoutGraph->GraphNodes) {
		const UFlowAbstractNode*& NodeRef = LayoutNodes.FindOrAdd(GraphNode->NodeId);
		NodeRef = GraphNode;
	}

	
	auto InsertTileShape = [&](FDungeonLayoutDataChunkInfo& OutChunkShape, const FVector& Location, const FVector& Size) {
		const FTransform Transform = FTransform(Location + FVector(Size.X, Size.Y, 0) * 0.5f) * LocalTransform;

		FVector WorldLocation = Transform.TransformPosition(Location);
		FDAShapePolygon& OutCellPoly = OutChunkShape.ConvexPolys.AddDefaulted_GetRef();
		OutCellPoly.Height = Size.Z;  // Use the actual height of the cell, not its Z position
		// ConvertBoxToConvexPoly will set the transform, so we don't need to set it here
		FDAShapeCollision::ConvertBoxToConvexPoly(Transform, Size * 0.5f, OutCellPoly);
		
		FBox CellBox(Location, Location + Size);
		CellBox = CellBox.TransformBy(LocalTransform);
		OutLayout.Bounds += CellBox;
	};

	
	// Emit the cell polygons
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		if (!GroupNode.IsActive()) continue;
		if (GroupNode.LeafNodes.Num() == 0) continue;

		const UFlowAbstractNode** LayoutNodePtr = LayoutNodes.Find(GroupNode.LayoutNodeID);
		const UFlowAbstractNode* LayoutNode = LayoutNodePtr ? *LayoutNodePtr : nullptr;
		if (!LayoutNode || !LayoutNode->bActive) {
			continue;
		}

		const TArray<FVector2d>& Sites = VoronoiData->Sites;
		const DA::DCELGraph& DGraph = VoronoiData->DGraph;
		const TArray<DA::DCEL::FFace*>& Faces = DGraph.GetFaces();
		
		FDungeonLayoutDataChunkInfo& OutChunkShape = OutLayout.ChunkShapes.AddDefaulted_GetRef();
		
		// Emit grid cell quads 
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeId])) {
				for (const FIntPoint& LeafTileCoord : GridLeafNode->LeafTileCoords) {
					const FVector NodeSize(1, 1, 1);  // Add 1 for Z to represent one cell height
					FVector Location = FVector(LeafTileCoord.X, LeafTileCoord.Y, GridLeafNode->LogicalZ) * GridSize;
					FVector Size = NodeSize * GridSize;  // Use full GridSize including Z component
					//Size.Z *= 2;	// The grid size Z component is usually height the wall height
					Size.Z = GridSize.Z * 2 * FMath::Max(1, GroupNode.GroupHeight);
					
					InsertTileShape(OutChunkShape, Location, Size);	
				}
			}
		}
		
		// Emit the voronoi leaf quad
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (Faces.IsValidIndex(LeafNodeId)) {
				if (UDAFlowCellLeafNodeVoronoi* LeafNode = Cast<UDAFlowCellLeafNodeVoronoi>(CellGraph->LeafNodes[LeafNodeId])) {
					const DA::DCEL::FFace* Face = Faces[LeafNodeId];
					if (!Face || !Face->bValid || !Face->Outer) continue;
					if (!Sites.IsValidIndex(LeafNodeId)) {
						continue;
					}

					TArray<const DA::DCEL::FEdge*> FaceEdges;
					DA::DCEL::TraverseFaceEdges(Face->Outer, [&](const DA::DCEL::FEdge* InEdge) {
						FaceEdges.Add(InEdge);
					});

					FDAShapePolygon& SitePoly = OutChunkShape.ConvexPolys.AddDefaulted_GetRef();
					SitePoly.Height = GridSize.Z * FMath::Max(1, GroupNode.GroupHeight);
					
					// Set transform at the center of the extruded polygon for consistency with grid cells
					const float CenterZ = LeafNode->LogicalZ * GridSize.Z; // + GridSize.Z * 0.5f;
					SitePoly.Transform = FTransform(FVector(0, 0, CenterZ)) * LocalTransform;
					for (const DA::DCEL::FEdge* FaceEdge : FaceEdges) {
						if (FaceEdge && FaceEdge->Origin) {
							FVector2D EdgeStart = FVector2D(FaceEdge->Origin->Location) * FVector2D(GridSize);
							SitePoly.Points.Add(EdgeStart);
							OutLayout.Bounds += LocalTransform.TransformPosition(FVector(EdgeStart, SitePoly.Height));
						}
					}
				}
			}
		}
	}

	// Emit the stairs
	{
		// Emit out the grid stair info
		for (const auto& StairEntry : CellGraph->GridInfo.Stairs) {
			int32 EdgeIndex = StairEntry.Key;
			const FDAFlowCellGraphGridStairInfo& StairInfo = StairEntry.Value;
			FQuat StairRotation({0, 0, 1}, StairInfo.AngleRadians);
			FVector StairLocation = StairInfo.LocalLocation * GridSize;
			
			FDungeonLayoutDataStairItem& OutStair = OutLayout.Stairs.AddDefaulted_GetRef();
			OutStair.WorldTransform = FTransform(FRotator(0, -90, 0)) * FTransform(StairRotation, StairLocation) * LocalTransform;
			OutStair.Width = GridSize.X;
		}

		
		// Emit out the voronoi stair info
		for (const auto& StairEntry : CellGraph->DCELInfo.Stairs) {
			const FDAFlowCellGraphDCELEdgeInfo& StairInfo = StairEntry.Value;
			
			const FVector Scale = FVector(StairInfo.LogicalWidth, 1, 1);
			const FQuat Rotation = FQuat::FindBetweenVectors({0, -1, 0}, StairInfo.Direction);
			const FVector BaseLocation = StairInfo.LogicalLocation;
			const FVector Location = (BaseLocation + StairInfo.Direction * 0.5f) * GridSize;
			
			FDungeonLayoutDataStairItem& OutStair = OutLayout.Stairs.AddDefaulted_GetRef();
			OutStair.WorldTransform = FTransform(FRotator(0, -90, 0)) * FTransform(Rotation, Location, Scale) * LocalTransform;
			OutStair.Width = StairInfo.LogicalWidth * GridSize.X;
		}
	}

	// Emit the door info
	{
		TSet<FGuid> DoorLayoutNodeIds;
		for (TObjectPtr<UFlowAbstractNode> FlowAbstractNode : LayoutGraph->GraphNodes) {
			if (TObjectPtr<UFlowAbstractDoorNode> DoorLayoutNode = Cast<UFlowAbstractDoorNode>(FlowAbstractNode)) {
				DoorLayoutNodeIds.Add(DoorLayoutNode->NodeId);
			}
		}
		
		// Emit the grid doors
		for (const FCellFlowGridEdgeInfo& Edge : CellGraph->GridInfo.HalfEdges) {
			FCellFlowGridEdgeInfo& EdgeTwin = CellGraph->GridInfo.HalfEdges[Edge.TwinIndex];
			auto InitializeDoorItem = [&](FDungeonLayoutDataDoorItem& OutDoor) {
				OutDoor.Width = GridSize.X;
				OutDoor.DoorOcclusionThickness = GridSize.X;
			
				const FIntPoint TileCoord { Edge.Coord };
				const FIntPoint TileCoordTwin { EdgeTwin.Coord };
				FIntPoint EdgeCoordSrc, EdgeCoordDst;
				FCellFlowUtils::GetEdgeEndPoints(Edge, EdgeTwin, EdgeCoordSrc, EdgeCoordDst);
			
				const double AngleRad = FDAMathUtils::FindAngle(FVector2d(EdgeCoordDst.X - EdgeCoordSrc.X, EdgeCoordDst.Y - EdgeCoordSrc.Y));
				const FVector EdgeWorldLocation = FVector(
					(TileCoord.X + TileCoordTwin.X) * 0.5f + 0.5f,
					(TileCoord.Y + TileCoordTwin.Y) * 0.5f + 0.5f,
					Edge.HeightZ
				) * GridSize;

				const FQuat EdgeWorldRotation(FVector::UpVector, AngleRad);
				OutDoor.WorldTransform = FTransform(FRotator(0, -90, 0)) * FTransform(EdgeWorldRotation, EdgeWorldLocation) * LocalTransform;
			};

			
			if (!Edge.bConnection && !EdgeTwin.bConnection) {
				// Check if the edge group belongs to the door group. if so create a wall opening
				if (CellGraph->GroupNodes.IsValidIndex(Edge.TileGroup) && CellGraph->GroupNodes.IsValidIndex(EdgeTwin.TileGroup)) {
					const FDAFlowCellGroupNode& EdgeGroup = CellGraph->GroupNodes[Edge.TileGroup];
					if (DoorLayoutNodeIds.Contains(EdgeGroup.LayoutNodeID)) {
						// The adjacent node is an opening
						FDungeonLayoutDataDoorItem&	OutDoor = OutLayout.WallOpening.AddDefaulted_GetRef();
						InitializeDoorItem(OutDoor);
						continue;
					}
				}
			}
			
			if (!Edge.bConnection) {
				continue;
			}
			if (!CellGraph->GridInfo.HalfEdges.IsValidIndex(Edge.TwinIndex)) {
				continue;
			}
			
			if (EdgeTwin.bConnection) {
				if (EdgeTwin.HeightZ > Edge.HeightZ) {
					// the twin is at a higher height.  We'll process that instead when we get to it
					continue;
				}
				else if (EdgeTwin.HeightZ == Edge.HeightZ) {
					if (EdgeTwin.TileGroup < Edge.TileGroup) {
						// The two sides of the door are on the same height. we'll process this only once
						continue;
					}
				}
			}
			
			FDungeonLayoutDataDoorItem&	OutDoor = OutLayout.Doors.AddDefaulted_GetRef();
			InitializeDoorItem(OutDoor);
		}

		// Emit out the voronoi door info
		for (const auto& DoorEntry : CellGraph->DCELInfo.Doors) {
			const FDAFlowCellGraphDCELEdgeInfo& DoorInfo = DoorEntry.Value;
			
			const FVector Scale = FVector(DoorInfo.LogicalWidth, 1, 1);
			const FQuat Rotation = FQuat::FindBetweenVectors({0, -1, 0}, DoorInfo.Direction);
			const FVector Location = DoorInfo.LogicalLocation * GridSize;
			
			FDungeonLayoutDataDoorItem& OutDoor = OutLayout.Doors.AddDefaulted_GetRef();
			OutDoor.WorldTransform = FTransform(FRotator(0, -90, 0)) * FTransform(Rotation, Location, Scale) * LocalTransform;
			OutDoor.Width = DoorInfo.LogicalWidth * GridSize.X;
			OutDoor.DoorOcclusionThickness = GridSize.X * 0.5f;
		}
	}
}

