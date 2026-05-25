//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/GridFlow/GridFlowModel.h"

#include "Builders/GridFlow/GridFlowConfig.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraph.h"
#include "Frameworks/FlowImpl/GridFlow/Tilemap/GridFlowTilemap.h"

void UGridFlowModel::Reset() {
	Super::Reset();
	if (AbstractGraph) {
		AbstractGraph->Clear();
	}

	if (Tilemap) {
		Tilemap->Clear();
	}

	TilemapBuildSetup = {};
}

namespace GridFlowModelImpl {
	struct FChunkInfo {
		TArray<const UFlowAbstractNode*> LayoutNodes;		// May contain more than one layout node coord (e.g. connected cave chunks will be merged here)
		EGridFlowAbstractNodeRoomType RoomType = EGridFlowAbstractNodeRoomType::Unknown;
	};
	
	EGridFlowAbstractNodeRoomType GetLayoutNodeRoomType(const UFlowAbstractNode* InLayoutNode) {
		if (const UFANodeTilemapDomainData* TilemapDomainData = InLayoutNode->FindDomainData<UFANodeTilemapDomainData>()) {
			return TilemapDomainData->RoomType;
		}
		return EGridFlowAbstractNodeRoomType::Unknown;
	}

	void RecursiveVisitLayoutCoord(const UFlowAbstractNode* InLayoutNode, FChunkInfo& OutChunkInfo, const FFlowAbstractGraphQuery& Query, TSet<const UFlowAbstractNode*>& VisitedLayoutCoords) {
		check(!VisitedLayoutCoords.Contains(InLayoutNode));
		VisitedLayoutCoords.Add(InLayoutNode);

		OutChunkInfo.LayoutNodes.Add(InLayoutNode);
		if (OutChunkInfo.RoomType == EGridFlowAbstractNodeRoomType::Cave) {
			// Traverse the adjacent cave chunks
			TArray<FGuid> ConnectedLayoutNodeIds = Query.GetConnectedNodes(InLayoutNode->NodeId);
			for (const FGuid& ConnectedNodeId : ConnectedLayoutNodeIds) {
				if (const UFlowAbstractNode* ConnectedNode = Query.GetNode(ConnectedNodeId)) {
					const EGridFlowAbstractNodeRoomType ConnectedRoomType = GetLayoutNodeRoomType(ConnectedNode);
					if (!VisitedLayoutCoords.Contains(ConnectedNode) && ConnectedRoomType == OutChunkInfo.RoomType) {
						RecursiveVisitLayoutCoord(ConnectedNode, OutChunkInfo, Query, VisitedLayoutCoords);
					}
				}
			}
		}
	};

}

void UGridFlowModel::GenerateLayoutDataImpl(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const {
    const UGridFlowConfig* GridFlowConfig = Cast<UGridFlowConfig>(InConfig);
	const AActor* DungeonActor = Cast<AActor>(GetOuter());
	if (!GridFlowConfig || !DungeonActor || !AbstractGraph || !Tilemap) {
		return;
	}

	const FTransform DungeonTransform = DungeonActor->GetActorTransform();
		
	OutLayout.FloorSettings = CreateFloorSettings(InConfig);
    OutLayout.Bounds.Init();

	const FVector& GridSize = GridFlowConfig->GridSize;
	const int32 Width = Tilemap->GetWidth();
	const int32 Height = Tilemap->GetHeight();
	int32 OffsetIdX = 0;
	int32 OffsetIdY = 0;
	if (GridFlowConfig->bAlignDungeonAtCenter) {
		OffsetIdX = Width / 2;
		OffsetIdY = Height / 2;
	}

	auto GetTileToWorldLocation = [&](const FVector2D& TileCoord) {
		const FVector FloorWorldCoord = FVector(TileCoord.X - OffsetIdX, TileCoord.Y - OffsetIdY, 0) * GridSize;
		return DungeonTransform.TransformPosition(FloorWorldCoord);
	};
	
	auto InsertTileShape = [&](const FIntPoint& TileCoord, FDungeonLayoutDataChunkInfo& OutChunkShape) {
		const FVector P00 = GetTileToWorldLocation(TileCoord);
		const FVector P01 = GetTileToWorldLocation(TileCoord + FIntPoint(0, 1));
		const FVector P11 = GetTileToWorldLocation(TileCoord + FIntPoint(1, 1));
		const FVector P10 = GetTileToWorldLocation(TileCoord + FIntPoint(1, 0));
			
		FDAShapePolygon& OutCellPoly = OutChunkShape.ConvexPolys.AddDefaulted_GetRef();
		OutCellPoly.Height = 0;
		OutCellPoly.Transform = FTransform::Identity;
		OutCellPoly.Points.Add(FVector2D(P00));
		OutCellPoly.Points.Add(FVector2D(P01));
		OutCellPoly.Points.Add(FVector2D(P11));
		OutCellPoly.Points.Add(FVector2D(P10));
		OutCellPoly.Height = 0;
		OutCellPoly.Transform = FTransform::Identity;
		
		OutLayout.Bounds += P00;
		OutLayout.Bounds += P01;
		OutLayout.Bounds += P11;
		OutLayout.Bounds += P10;
	};

	TMap<FGuid, const UFlowGraphItem*> Items;
	{
    	TArray<UFlowGraphItem*> ItemArray;
    	AbstractGraph->GetAllItems(ItemArray);
    	for (const UFlowGraphItem* Item : ItemArray) {
    		const UFlowGraphItem*& ItemRef = Items.FindOrAdd(Item->ItemId);
    		ItemRef = Item;
    	}
	}
	
	TArray<GridFlowModelImpl::FChunkInfo> ChunkInfoList;
	
	const FFlowAbstractGraphQuery Query(AbstractGraph);
	TSet<const UFlowAbstractNode*> VisitedLayoutCoords;
	for (const UFlowAbstractNode* LayoutNode : AbstractGraph->GraphNodes) {
		if (!LayoutNode->bActive || VisitedLayoutCoords.Contains(LayoutNode)) {
			continue;
		}

		EGridFlowAbstractNodeRoomType RoomType = GridFlowModelImpl::GetLayoutNodeRoomType(LayoutNode);
		if (RoomType != EGridFlowAbstractNodeRoomType::Unknown) {
			GridFlowModelImpl::FChunkInfo& ChunkInfo = ChunkInfoList.AddDefaulted_GetRef();
			ChunkInfo.RoomType = RoomType;
			GridFlowModelImpl::RecursiveVisitLayoutCoord(LayoutNode, ChunkInfo, Query, VisitedLayoutCoords);
		}
	}

	TMap<FVector, const UFlowAbstractNode*> LayoutCoordToNode;
	for (const UFlowAbstractNode* LayoutNode : AbstractGraph->GraphNodes) {
		if (LayoutNode) {
			const UFlowAbstractNode*& NodeRef = LayoutCoordToNode.FindOrAdd(LayoutNode->Coord);
			NodeRef = LayoutNode;
		}
	}
	
	const float DoorOpeningSize = GridSize.X * 1.0f;
	const float DoorOcclusionSize = GridSize.X * 1.5f;
	
	FDungeonLayoutDataChunkInfo& DoorChunkShape = OutLayout.ChunkShapes.AddDefaulted_GetRef();
	TMap<const UFlowAbstractNode*, TArray<FIntPoint>> LayoutNodeTileCoords;
	for (int32 y = 0; y < Height; y++) {
		for (int32 x = 0; x < Width; x++) {
			if (const FFlowTilemapCell* Cell = Tilemap->GetSafe(x, y)) {
				if (Cell->CellType == EFlowTilemapCellType::Floor) {
					if (const UFlowAbstractNode** NodePtr = LayoutCoordToNode.Find(Cell->ChunkCoord)) {
						if (const UFlowAbstractNode* LayoutNode = *NodePtr) {
							TArray<FIntPoint>& NodeTileCoords = LayoutNodeTileCoords.FindOrAdd(LayoutNode);
							NodeTileCoords.Add(Cell->TileCoord);
						}
					}
				}
				else if (Cell->CellType == EFlowTilemapCellType::Door) {
					// Emit the doors (placed on tiles)
					InsertTileShape(Cell->TileCoord, DoorChunkShape);

					{
						const FFlowTilemapCell* CellA = Tilemap->GetSafe(x - 1, y);
						const FFlowTilemapCell* CellB = Tilemap->GetSafe(x + 1, y);
						bool bHorizontalDoor = (CellA && CellB
							&& CellA->CellType == EFlowTilemapCellType::Floor
							&& CellB->CellType == EFlowTilemapCellType::Floor
							&& CellA->ChunkCoord != CellB->ChunkCoord);

						FDungeonLayoutDataDoorItem& OutDoorInfo = OutLayout.Doors.AddDefaulted_GetRef();
						OutDoorInfo.Width = DoorOpeningSize;
						OutDoorInfo.DoorOcclusionThickness = DoorOcclusionSize;

						FVector WorldPoint = GetTileToWorldLocation(FVector2D(Cell->TileCoord) + FVector2D(0.5f, 0.5f));
						FTransform WorldRotation = (bHorizontalDoor ? FTransform(FRotator(0, 0, 0)) : FTransform(FRotator(0, 90, 0)))
								* FTransform(DungeonTransform.GetRotation());
						
						OutDoorInfo.WorldTransform = FTransform(WorldRotation.GetRotation(), WorldPoint);
					}
				}
			}
		}
	}

	TArray<const FFlowTilemapEdge*> DoorEdges;
	for (int32 y = 0; y < Height; y++) {
		for (int32 x = 0; x < Width; x++) {
			if (const FFlowTilemapEdge* EdgeH = Tilemap->GetEdgeHSafe(x, y)) {
				if (EdgeH->EdgeType == EFlowTilemapEdgeType::Door) {
					DoorEdges.Add(EdgeH);
				}
			}
			if (const FFlowTilemapEdge* EdgeV = Tilemap->GetEdgeVSafe(x, y)) {
				if (EdgeV->EdgeType == EFlowTilemapEdgeType::Door) {
					DoorEdges.Add(EdgeV);
				}
			}
		}
	}
		
	for (const FFlowTilemapEdge* DoorEdge : DoorEdges) {
		FDungeonLayoutDataDoorItem& OutDoorInfo = OutLayout.Doors.AddDefaulted_GetRef();
		OutDoorInfo.Width = DoorOpeningSize;
		OutDoorInfo.DoorOcclusionThickness = DoorOpeningSize;

		const FVector2D EdgeOffset = DoorEdge->EdgeCoord.bHorizontalEdge ? FVector2D(0.5f, 0) : FVector2D(0, 0.5f);
		const FVector WorldPoint = GetTileToWorldLocation(FVector2D(DoorEdge->EdgeCoord.Coord) + EdgeOffset);
		OutDoorInfo.WorldTransform = FTransform(DoorEdge->EdgeCoord.bHorizontalEdge
			? FRotator(0, 0, 0)
			: FRotator(0, 90, 0)
			, WorldPoint);
	}

	for (const GridFlowModelImpl::FChunkInfo& ChunkInfo : ChunkInfoList) {
		if (ChunkInfo.LayoutNodes.Num() == 0) {
			continue;
		}
		
		FDungeonLayoutDataChunkInfo& OutChunkShape = OutLayout.ChunkShapes.AddDefaulted_GetRef();
		for (const UFlowAbstractNode* LayoutNode : ChunkInfo.LayoutNodes) {
			if (TArray<FIntPoint>* LayoutCoordsPtr = LayoutNodeTileCoords.Find(LayoutNode)) {
				TArray<FIntPoint>& TileCoords = *LayoutCoordsPtr;
				for (const FIntPoint& TileCoord : TileCoords) {
					InsertTileShape(TileCoord, OutChunkShape);
				} 
			}
		}
	}
}

FDungeonFloorSettings UGridFlowModel::CreateFloorSettings(const UDungeonConfig* InConfig) const {
	FDungeonFloorSettings Settings = {};
	if (const UGridFlowConfig* GridFlowConfig = Cast<UGridFlowConfig>(InConfig)) {
		Settings.FloorHeight = GridFlowConfig->GridSize.Z;
		Settings.FloorCaptureHeight = Settings.FloorHeight - 1;
	}
	return Settings;
}

