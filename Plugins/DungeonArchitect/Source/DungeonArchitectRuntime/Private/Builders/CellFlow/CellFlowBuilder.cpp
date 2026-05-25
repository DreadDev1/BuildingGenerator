//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/CellFlow/CellFlowBuilder.h"

#include "Builders/CellFlow/CellFlowAsset.h"
#include "Builders/CellFlow/CellFlowConfig.h"
#include "Builders/CellFlow/CellFlowModel.h"
#include "Builders/CellFlow/CellFlowQuery.h"
#include "Builders/CellFlow/CellFlowSelectorLogic.h"
#include "Builders/CellFlow/CellFlowToolData.h"
#include "Builders/CellFlow/CellFlowTransformLogic.h"
#include "Core/Dungeon.h"
#include "Core/Markers/DungeonMarkerNames.h"
#include "Core/Markers/DungeonMarkerTags.h"
#include "Core/Markers/DungeonMarkerVisualization.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Utils/FlowLayoutGraphVisualization.h"
#include "Frameworks/Flow/Domains/Voxel/FlowVoxelState.h"
#include "Frameworks/Flow/ExecGraph/FlowExecGraphScript.h"
#include "Frameworks/Flow/FlowProcessor.h"
#include "Frameworks/Flow/Utils/DungeonFlowTagVolume.h"
#include "Frameworks/FlowImpl/CellFlow/CellFlowConstants.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraphDomain.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLibGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLibVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowUtils.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/CellFlowChunkMeshActor.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshGeneratorDCEL.h"
#include "Frameworks/MarkerGenerator/Impl/Grid/MarkerGenGridProcessor.h"
#include "Frameworks/MarkerGenerator/MarkerGenLayer.h"
#include "Frameworks/MarkerGenerator/MarkerGenModel.h"
#include "Frameworks/Meshing/Geometry/DungeonProceduralMesh.h"
#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerEmitter.h"

#include "Materials/MaterialInterface.h"
#include "Misc/Variant.h"

DEFINE_LOG_CATEGORY_STATIC(CellFlowBuilderLog, Log, All);

void UCellFlowBuilder::BuildDungeonImpl(UWorld* InWorld) {
	
	if (!CellModel.IsValid()) {
		UE_LOG(CellFlowBuilderLog, Error, TEXT("Invalid dungeon model provided to the cell flow builder"));
		return;
	}

	if (!CellConfig.IsValid()) {
		UE_LOG(CellFlowBuilderLog, Error, TEXT("Invalid dungeon config provided to the cell flow builder"));
		return;
	}

	if (ExecuteGraph(InWorld)) {
		const FGuid DungeonUid = Dungeon ? Dungeon->Uid : FGuid();
		FTransform DungeonTransform = GetDungeonTransform();

		DestroyBaseMesh(InWorld, DungeonUid);
		DestroyDebugVisualizations(InWorld, DungeonUid);
		
		if (!BuilderParameters.bLayoutPreviewBuildOnly) {
			GenerateBaseMesh(InWorld, DungeonUid, DungeonTransform);
	
			if (Dungeon && Dungeon->bDrawDebugData) {
				CreateDebugVisualizations(InWorld, DungeonUid, DungeonTransform);
			}
		}
	}
	else {
		UE_LOG(CellFlowBuilderLog, Error, TEXT("Failed to execute cell flow graph"));
	}
}

void UCellFlowBuilder::DestroyDungeonImpl(UWorld* InWorld) {
	CellModel = Cast<UCellFlowModel>(DungeonModel);
	CellConfig = Cast<UCellFlowConfig>(DungeonConfig);
	CellQuery = Cast<UCellFlowQuery>(DungeonQuery);

	if (Dungeon) {
		DestroyBaseMesh(InWorld, Dungeon->Uid);
		DestroyDebugVisualizations(InWorld, Dungeon->Uid);
	}
}

void UCellFlowBuilder::InitializeState() {
	Super::InitializeState();
	
	CellModel = Cast<UCellFlowModel>(DungeonModel);
	CellConfig = Cast<UCellFlowConfig>(DungeonConfig);
	CellQuery = Cast<UCellFlowQuery>(DungeonQuery);
}

void UCellFlowBuilder::EmitDungeonMarkers_Implementation() {
	Super::EmitDungeonMarkers_Implementation();

	CellModel = Cast<UCellFlowModel>(DungeonModel);
	CellConfig = Cast<UCellFlowConfig>(DungeonConfig);
	CellQuery = Cast<UCellFlowQuery>(DungeonQuery);

	EmitGridMarkers();
	EmitDcelMarkers();
}

void UCellFlowBuilder::EmitGridMarkers() {
	if (!CellConfig.IsValid() || !CellModel.IsValid() || !CellModel->CellGraph || !CellModel->LayoutGraph) return;
	const FTransform DungeonTransform = GetDungeonTransform();
	UDAFlowCellGraph* CellGraph = CellModel->CellGraph;
	const FFlowAbstractGraphQuery LayoutGraphQuery(CellModel->LayoutGraph);
	const FVector GridSize = CellConfig->GridSize;
	const bool bSeparateAdjacentRooms = CellGraph->GlobalSettings.bSeparateAdjacentRooms;

	FDAVoxelMeshGenerationSettings VoxelMeshGenSettings{};
	ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter());
	if (OuterDungeon) {
		VoxelMeshGenSettings = OuterDungeon->VoxelMeshSettings;
	}
	
	FCellFlowLibGrid::FCellFlowGridMarkerContext Context;
	Context.GridSize = GridSize;
	Context.VoxelWallThickness = VoxelMeshGenSettings.WallThickness;
	// Get world-specific ceiling height for volume placement
	float BaseRoomHeight = GridSize.Z * 2;
	if (OuterDungeon && OuterDungeon->VoxelSDFModel) {
		Context.VoxelTotalCeilingHeight = OuterDungeon->VoxelSDFModel->GetTotalCeilingHeight(BaseRoomHeight);
	} else {
		Context.VoxelTotalCeilingHeight = BaseRoomHeight;
	}
	
	if (UCellFlowConfigMarkerSettings* MarkerSettings = CellConfig->MarkerConfig.LoadSynchronous()) {
		FCellFlowUtils::GenerateGroupNodeMarkerSetup(MarkerSettings->DefaultGridPathMarkers, MarkerSettings->GridPathMarkers, CellGraph->GroupNodes, LayoutGraphQuery, Random, Context.GroupNodeChunkMarkers);
	}

	auto GetLayoutNode = [&CellGraph, &LayoutGraphQuery](int32 InGroupId) -> UFlowAbstractNode* {
		if (!CellGraph->GroupNodes.IsValidIndex(InGroupId)) {
			return nullptr;
		}
		const FGuid& LayoutNodeID = CellGraph->GroupNodes[InGroupId].LayoutNodeID;
		return LayoutGraphQuery.GetNode(LayoutNodeID);
	};
	
	auto GetTileGroupHeight = [&](int32 TileGroupId) {
		if (CellGraph && CellGraph->GroupNodes.IsValidIndex(TileGroupId)) {
			return CellGraph->GroupNodes[TileGroupId].GroupHeight;
		}
		return 1;
	};

	FVector VolumeScale;
	float VoxelTargetZ;
	{
		constexpr float LocalVolumeSize = 200;
		const float WallThickness = Context.VoxelWallThickness;
		const float TotalCeilingHeight = Context.VoxelTotalCeilingHeight;
		const float TargetX = Context.GridSize.X + WallThickness;
		const float TargetY = WallThickness;
		VoxelTargetZ = TotalCeilingHeight + WallThickness * 4;
		VolumeScale = FVector(TargetX, TargetY, VoxelTargetZ) / LocalVolumeSize;
	}

	auto GetVoxelMarkerTransform = [&VoxelTargetZ, &VolumeScale](const FQuat& InRotation, const FVector& InLocation) {
		FVector VolumeLocation = InLocation + FVector(0, 0, VoxelTargetZ * 0.5f);
		return FTransform(InRotation, VolumeLocation, VolumeScale);
	};
	
	const TArray<FCellFlowGridEdgeInfo>& HalfEdges = CellGraph->GridInfo.HalfEdges;
	
	// Emit the ground marker specs
	for (int GroupIdx = 0; GroupIdx < CellGraph->GroupNodes.Num(); GroupIdx++) {
		const FDAFlowCellGroupNode& GroupNode = CellGraph->GroupNodes[GroupIdx];
		if (!GroupNode.IsActive()) continue;

		const UFlowAbstractNode* LayoutNode = LayoutGraphQuery.GetNode(GroupNode.LayoutNodeID);
		if (!LayoutNode || !LayoutNode->bActive) {
			continue;
		}

		FCellFlowGridMarkerSetup ChunkMarkers{};
		if (const FCellFlowGridMarkerSetup* SearchPtr = Context.GroupNodeChunkMarkers.Find(GroupNode.GroupId)){
			ChunkMarkers = *SearchPtr;
		}
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeId])) {
				for (const FIntPoint& Coord2D : GridLeafNode->LeafTileCoords) {
					FIntVector TileCoord(Coord2D.X, Coord2D.Y, GridLeafNode->LogicalZ);
					
					int32 LocalHeight = GroupNode.GroupHeight;
					FVector WorldLoc = FVector(TileCoord.X + 0.5f, TileCoord.Y + 0.5f, TileCoord.Z) * GridSize;
					
					// Emit the ground marker
					FDungeonMarkerInstance& GroundMarker = EmitCellMarker(ChunkMarkers.GroundMarker, DungeonTransform, FTransform(WorldLoc));
					FDungeonMarkerInstanceTagManagement::SetElevationFromGround(GroundMarker.Tags, 0);
					FDungeonMarkerInstanceTagManagement::SetCeilingElevation(GroundMarker.Tags, LocalHeight);
					
					// Emit the ceiling marker
					FString CeilingMarkerName = FCellFlowBuilderMarkers::CEILING;
					if (const FCellFlowGridMarkerSetup* GroupMarkerSetup = Context.GroupNodeChunkMarkers.Find(GroupNode.GroupId)) {
						CeilingMarkerName = GroupMarkerSetup->CeilingMarker;
					}
			
					const FVector CeilingWorldLoc = FVector(TileCoord.X + 0.5f, TileCoord.Y + 0.5f, TileCoord.Z + LocalHeight * 2) * GridSize;
					FDungeonMarkerInstance& CeilingMarker = EmitCellMarker(CeilingMarkerName, DungeonTransform, FTransform(CeilingWorldLoc));
					FDungeonMarkerInstanceTagManagement::SetElevationFromGround(CeilingMarker.Tags, LocalHeight);
					FDungeonMarkerInstanceTagManagement::SetCeilingElevation(CeilingMarker.Tags, LocalHeight);
				}
			}
		}
	}
	
	// Emit the edge markers
	for (int i = 0; i < HalfEdges.Num(); i++) {
		const FCellFlowGridEdgeInfo& Edge = HalfEdges[i];

		// Check if the edge is invalid
		if (!Edge.bEnabled || Edge.TileGroup == INDEX_NONE) {
			continue;
		}

		const FCellFlowGridEdgeInfo& EdgeTwin = HalfEdges[Edge.TwinIndex];
		const FIntPoint TileCoord { Edge.Coord };
		const FIntPoint TileCoordTwin { EdgeTwin.Coord };

		FCellFlowGridMarkerSetup ChunkMarkers{};
		if (const FCellFlowGridMarkerSetup* GroupMarkerSetupPtr = Context.GroupNodeChunkMarkers.Find(Edge.TileGroup)) {
			ChunkMarkers = *GroupMarkerSetupPtr;
		}
	
		FIntPoint EdgeCoordSrc, EdgeCoordDst;
		FCellFlowUtils::GetEdgeEndPoints(Edge, EdgeTwin, EdgeCoordSrc, EdgeCoordDst);
		
		const double AngleRad = FDAMathUtils::FindAngle(FVector2d(EdgeCoordDst.X - EdgeCoordSrc.X, EdgeCoordDst.Y - EdgeCoordSrc.Y));
		const FQuat EdgeWorldRotation(FVector::UpVector, AngleRad);
		int EdgeHeightZ = Edge.HeightZ;
		if (Edge.bContainsStair) {
			EdgeHeightZ = FMath::Max(Edge.HeightZ, EdgeTwin.HeightZ);
		}

		bool bEmitMarker{true};
		if (Edge.bConnection) {
			if (Edge.HeightZ < EdgeTwin.HeightZ) {
				// Use the higher door instead
				bEmitMarker = false;
			}
			else if (Edge.HeightZ == EdgeTwin.HeightZ) {
				// Choose any one of the doors
				bEmitMarker = Edge.TileGroup < EdgeTwin.TileGroup;
			}
		}

		
		// Check if the edge is shared by the door tile group and doesn't contain a connection in this current edge
		int32 GroupHeight = GetTileGroupHeight(Edge.TileGroup);
		auto AssignWallMarkerTags = [&](FDungeonMarkerInstance& Marker, int32 DZ) {
			FDungeonMarkerInstanceTagManagement::SetElevationFromGround(Marker.Tags, DZ);
			FDungeonMarkerInstanceTagManagement::SetCeilingElevation(Marker.Tags, GroupHeight);
		};
		
		if (bEmitMarker) {
			int32 StartDZ = 0;
			if (bSeparateAdjacentRooms && !Edge.bConnection) {
				if (Edge.HeightZ == EdgeTwin.HeightZ) {
					if (CellGraph->GroupNodes.IsValidIndex(Edge.TileGroup) && CellGraph->GroupNodes.IsValidIndex(EdgeTwin.TileGroup)) {
						const FDAFlowCellGroupNode& EdgeGroup = CellGraph->GroupNodes[Edge.TileGroup];
						const FDAFlowCellGroupNode& EdgeTwinGroup = CellGraph->GroupNodes[EdgeTwin.TileGroup];
						if (EdgeGroup.IsActive() && EdgeTwinGroup.IsActive()) {
							StartDZ = FMath::Min(EdgeGroup.GroupHeight, EdgeTwinGroup.GroupHeight);
						}
					}
				}
			}
			
			for (int DZ = StartDZ; DZ < GroupHeight; DZ++) {
				const FVector EdgeWorldLocation = FVector(
					(TileCoord.X + TileCoordTwin.X) * 0.5 + 0.5,
					(TileCoord.Y + TileCoordTwin.Y) * 0.5 + 0.5,
					EdgeHeightZ + DZ * 2
				) * Context.GridSize;

				
				auto InsertWallSeparator = [&](const FIntVector& InCoord) {
					if (!Context.InsertedPillarCoords.Contains(InCoord)) {
						Context.InsertedPillarCoords.Add(InCoord);
						const FVector SeparatorWorldLocation = FVector(InCoord) * Context.GridSize;
						const FTransform SeparatorTransform(EdgeWorldRotation, SeparatorWorldLocation);
						FDungeonMarkerInstance& WallSepMarker = EmitCellMarker(ChunkMarkers.EdgeWallSeparatorMarker, DungeonTransform, SeparatorTransform);
						AssignWallMarkerTags(WallSepMarker, DZ);
					}
				};

				if (!Edge.bConnection && StartDZ != 0 && StartDZ == DZ) {
					// Emit out a ceiling trim here
					FDungeonMarkerInstance& Marker = EmitCellMarker(ChunkMarkers.CeilingEdgeTrimMarker, DungeonTransform, FTransform(EdgeWorldRotation, EdgeWorldLocation));
					AssignWallMarkerTags(Marker, DZ);	
				}
				
				const FString MarkerName = Edge.bConnection && DZ == 0 ? ChunkMarkers.DoorMarker : ChunkMarkers.EdgeWallMarker;
				FDungeonMarkerInstance& Marker = EmitCellMarker(MarkerName, DungeonTransform, FTransform(EdgeWorldRotation, EdgeWorldLocation));
				AssignWallMarkerTags(Marker, DZ);
				
				// Emit the voxel edge marker
				if (!Edge.bConnection) {
					// TODO: Multiply with the grid size so the voxel volumes can have the scale of 1 and adapt to any grid size form the config
					FDungeonMarkerInstance& VoxelMarker = EmitCellMarker(FVoxelBuilderMarkers::INTERNAL_VOXEL_WALL, DungeonTransform, GetVoxelMarkerTransform(EdgeWorldRotation, EdgeWorldLocation));
					AssignWallMarkerTags(VoxelMarker, DZ);
				}

				bool bEmitWallSeparators = true;
				UFlowAbstractNode* EdgeGroupLayoutNode = GetLayoutNode(Edge.TileGroup);
				if (EdgeGroupLayoutNode && EdgeGroupLayoutNode->IsA<UFlowAbstractDoorNode>()) {
					bEmitWallSeparators = false;
				}
				
				// Insert the Wall Separators
				if (bEmitWallSeparators) {
					InsertWallSeparator({ EdgeCoordSrc.X, EdgeCoordSrc.Y, EdgeHeightZ + DZ * 2 });
					InsertWallSeparator({ EdgeCoordDst.X, EdgeCoordDst.Y, EdgeHeightZ + DZ * 2 });
				}
			}
		}
		
		if (Edge.bContainsStair) {
			if (const FDAFlowCellGraphGridStairInfo* StairInfo = CellGraph->GridInfo.Stairs.Find(Edge.EdgeIndex)) {
				const FQuat StairRotation({0, 0, 1}, StairInfo->AngleRadians - PI * 0.5);	// TODO: Fix the 90 degree hack
				const FVector BaseLoc = FVector(StairInfo->LocalLocation) * Context.GridSize;
				EmitCellMarker(ChunkMarkers.StairMarker, DungeonTransform,  FTransform(StairRotation, BaseLoc));

				// TODO: Multiply with the grid size so the voxel volumes can have the scale of 1 and adapt to any grid size form the config
				EmitCellMarker(FVoxelBuilderMarkers::INTERNAL_VOXEL_STAIR, DungeonTransform, GetVoxelMarkerTransform(StairRotation, BaseLoc));
			}
			
			// Add a half wall on top of the door
			{
				const FVector EdgeWorldLocation = FVector(
					(TileCoord.X + TileCoordTwin.X) * 0.5 + 0.5,
					(TileCoord.Y + TileCoordTwin.Y) * 0.5 + 0.5,
					EdgeHeightZ + 2
				) * Context.GridSize;

				const FString MarkerName = ChunkMarkers.EdgeDoorWallHalfMarker;
				FDungeonMarkerInstance& Marker = EmitCellMarker(MarkerName, DungeonTransform, FTransform(EdgeWorldRotation, EdgeWorldLocation));
				AssignWallMarkerTags(Marker, 1);
			}
			
			// Add a half wall behind the stair
			{
				const FVector EdgeWorldLocation = FVector(
					(TileCoord.X + TileCoordTwin.X) * 0.5 + 0.5,
					(TileCoord.Y + TileCoordTwin.Y) * 0.5 + 0.5,
					EdgeHeightZ - 1
				) * Context.GridSize;

				const FString MarkerName = ChunkMarkers.EdgeWallHalfMarker;
				FDungeonMarkerInstance& Marker = EmitCellMarker(MarkerName, DungeonTransform, FTransform(EdgeWorldRotation, EdgeWorldLocation));
				AssignWallMarkerTags(Marker, 0);
			}
		}
	}

	// Emit the item marker
	for (const FDAFlowCellGraphSpawnInfo& SpawnInfo : CellGraph->GridInfo.SpawnInfo) {
		if (!SpawnInfo.Item.IsValid()) {
			continue;
		}
		const UFlowGraphItem* Item = SpawnInfo.Item.Get();
		FIntVector TileCoord(
			FMath::RoundToInt(SpawnInfo.Coord.X),
			FMath::RoundToInt(SpawnInfo.Coord.Y),
			FMath::RoundToInt(SpawnInfo.Coord.Z));

		FVector WorldLoc = FVector(TileCoord.X + 0.5f, TileCoord.Y + 0.5f, TileCoord.Z) * GridSize;
		EmitCellMarker(Item->MarkerName, DungeonTransform, FTransform(WorldLoc));
	}

	// Run the Scatter Prop pattern generator
	if (CellGraph->ScatterSettings.Num() > 0) {
		const UDungeonThemeAsset* PatternThemeAsset = Cast<UDungeonThemeAsset>(StaticLoadObject(
			UDungeonThemeAsset::StaticClass(), nullptr, TEXT("/DungeonArchitect/Core/Runtime/Builders/CellFlowContent/Patterns/_Internal_Theme_CellFlowPatterns")));

		auto GetLayerProbability = [](const UMarkerGenLayer* Layer, const FCellFlowLayoutTaskScatterPropSettings& Settings) {
			static const FName LayerTagBase = TEXT("Base");
			static const FName LayerTag1x1 = TEXT("1x1");
			static const FName LayerTag1x2 = TEXT("1x2");
			static const FName LayerTag1x3 = TEXT("1x3");
			if (Layer->Tags.Contains(LayerTagBase)) return 1.0f;
			if (Layer->Tags.Contains(LayerTag1x1)) return Settings.Prop1x1.bEnabled ? Settings.Prop1x1.Probability : 0.0f;
			if (Layer->Tags.Contains(LayerTag1x2)) return Settings.Prop1x2.bEnabled ? Settings.Prop1x2.Probability : 0.0f;
			if (Layer->Tags.Contains(LayerTag1x3)) return Settings.Prop1x3.bEnabled ? Settings.Prop1x3.Probability : 0.0f;
			return 0.0f;
		};

		TArray<FDungeonMarkerInstance>& WorldMarkers = CellModel->WorldMarkers;
		TSharedPtr<IMarkerGenProcessor> MarkerGenerator = CreateMarkerGenProcessor(DungeonTransform);
		if (PatternThemeAsset && PatternThemeAsset->MarkerGenerationModel) {
			for (const FCellFlowLayoutTaskScatterPropSettings& ScatterSetting : CellGraph->ScatterSettings) {
				for (UMarkerGenLayer* MarkerGenLayer : PatternThemeAsset->MarkerGenerationModel->Layers) {
					if (MarkerGenLayer && MarkerGenLayer->bEnabled) {
						const float LayerProbability = GetLayerProbability(MarkerGenLayer, ScatterSetting);
						MarkerGenLayer->Probability = LayerProbability;
						if (LayerProbability > 0) {
							TArray<FDungeonMarkerInstance> NewMarkers;
							FCellFlowLibGrid::TransformPatternLayer(MarkerGenLayer, ScatterSetting);
							if (MarkerGenerator->Process(MarkerGenLayer, WorldMarkers, Random, NewMarkers)) {
								WorldMarkers = NewMarkers;
							}
						}
					}
				}
			}
		}
	}
}


void UCellFlowBuilder::EmitDcelMarkers() {
	if (!CellConfig.IsValid() || !CellModel.IsValid() || !CellModel->CellGraph || !CellModel->LayoutGraph) return;
	const FTransform DungeonTransform = GetDungeonTransform();
	const UDAFlowCellGraph* CellGraph = CellModel->CellGraph;
	const UCellFlowLayoutGraph* LayoutGraph = CellModel->LayoutGraph;
	const UCellFlowVoronoiGraph* VoronoiData = CellModel->VoronoiData;
	const FVector GridSize = CellConfig->GridSize;
	const FFlowAbstractGraphQuery GraphQuery(CellModel->LayoutGraph);

	FCellFlowLibVoronoi::FEmitMarkersContext EmitMarkerContext;
	FCellFlowLibVoronoi::InitMarkerContext(CellModel->CellGraph, VoronoiData->DGraph, CellModel->LayoutGraph,
			CellConfig->MarkerConfig.LoadSynchronous(), GridSize, Random, EmitMarkerContext);

	// Draw the debug data
	UDASceneDebugDataComponent* SceneDebugData = Dungeon ? Dungeon->GetComponentByClass<UDASceneDebugDataComponent>() : nullptr;
	if (SceneDebugData) {
		SceneDebugData->RenderScale = CellConfig->GridSize;
		for (const auto& Entry : CellGraph->DCELInfo.Stairs) {
			int EdgeIdx = Entry.Key;
			const FCellFlowLibVoronoi::FStairGenInfo& StairGenInfo = EmitMarkerContext.StairGenInfoByEdge.FindOrAdd(EdgeIdx);
			SceneDebugData->Data.BoxEntries.Add({ StairGenInfo.OcclusionTransform, StairGenInfo.OcclusionBoxExtents, FColor::Cyan });
		}
	}

	
	FDAVoxelMeshGenerationSettings VoxelMeshGenSettings{};
	ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter());
	if (OuterDungeon) {
		VoxelMeshGenSettings = OuterDungeon->VoxelMeshSettings;
	}
	
	constexpr float LocalVolumeSize = 200;
	const float WallThickness = VoxelMeshGenSettings.WallThickness;
	// Get world-specific ceiling height for volume placement
	const float BaseRoomHeight = GridSize.Z * 2;
	float TotalCeilingHeight = BaseRoomHeight;
	if (OuterDungeon && OuterDungeon->VoxelSDFModel) {
		TotalCeilingHeight = OuterDungeon->VoxelSDFModel->GetTotalCeilingHeight(BaseRoomHeight);
	}
	const float TargetX = GridSize.X + WallThickness;
	const float TargetY = WallThickness;
	const float TargetZ = TotalCeilingHeight + WallThickness * 4;
	const FVector VolumeEdgeScale = FVector(TargetX, TargetY, TargetZ) / LocalVolumeSize;
	const FVector VolumeStairScale = GridSize / LocalVolumeSize;

	FCellFlowLibVoronoi::TFuncEmitVoxelMarker FnEmitVoxelEdgeMarker = [&](const FString& InMarkerName, const FTransform& InMarkerTransform) {
		FVector Scale = InMarkerTransform.GetScale3D();
		FVector Location = InMarkerTransform.GetLocation();
		FQuat Rotation = InMarkerTransform.GetRotation();
		Location += FVector(0, 0, TargetZ * 0.5f - WallThickness);
		Scale *= VolumeEdgeScale;
		FTransform VolumeMarkerTransform(Rotation, Location, Scale);
		AddMarker(InMarkerName, VolumeMarkerTransform * DungeonTransform);	
	};
	
	FCellFlowLibVoronoi::TFuncEmitVoxelMarker FnEmitVoxelStairMarker = [&](const FString& InMarkerName, const FTransform& InMarkerTransform) {
		FVector Scale = InMarkerTransform.GetScale3D();
		FVector Location = InMarkerTransform.GetLocation() + FVector(0, 0, GridSize.Z * 0.5f);
		FQuat Rotation = InMarkerTransform.GetRotation();
		Scale *= VolumeStairScale;
		FTransform VolumeMarkerTransform(Rotation, Location, Scale);
		AddMarker(InMarkerName, VolumeMarkerTransform * DungeonTransform);	
	};
	
	FCellFlowLibVoronoi::TFuncEmitMarker FnEmitMarker = [&](const FCellFlowSizedMarkerDef& MarkerDef, const FTransform& MarkerTransform, const FTransform& OcclusionTransform) {
		AddMarker(MarkerDef.MarkerName, MarkerTransform * DungeonTransform);
		if (SceneDebugData) {
			const float OcclusionDepth = MarkerDef.bOccludes ? MarkerDef.OcclusionDepth : 0;
			const FVector BoxExtent(MarkerDef.Size * 0.5f, OcclusionDepth * 0.5f, 0);
			SceneDebugData->Data.BoxEntries.Add({ OcclusionTransform, BoxExtent, FColor::Red });
		}
	};
	
	//const int GlobalCliffDepth = FCellFlowLevelMeshLib::GetGlobalCliffDepth(LayoutGraph, CellGraph, EmitMarkerContext.LayoutNodes);
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		const UFlowAbstractNode** LayoutNodePtr = EmitMarkerContext.LayoutNodes.Find(GroupNode.LayoutNodeID);
		const UFlowAbstractNode* LayoutNode = LayoutNodePtr ? *LayoutNodePtr : nullptr;
		if (!LayoutNode || !LayoutNode->bActive) {
			continue;
		}
		const TArray<FVector2d>& Sites = VoronoiData->Sites;
		const DA::DCELGraph& DGraph = VoronoiData->DGraph;
		
		const TArray<DA::DCEL::FFace*>& Faces = DGraph.GetFaces();
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (Faces.IsValidIndex(LeafNodeId)) {
				const UDAFlowCellLeafNode* LeafNode = CellGraph->LeafNodes[LeafNodeId];
				if (!LeafNode) continue;
				const int LogicalHeightZ = LeafNode->LogicalZ;
				
				const DA::DCEL::FFace* Face = Faces[LeafNodeId];
				if (!Face || !Face->bValid || !Face->Outer) continue;
				if (!Sites.IsValidIndex(LeafNodeId)) {
					continue;
				}

				TSet<int> DoorEdgeIndices(CellGraph->DCELInfo.DoorEdges);
				DA::DCEL::TraverseFaceEdges(Face->Outer,
				[&](const DA::DCEL::FEdge* InEdge) {
					if (const bool bGroupBorderEdge = FCellFlowLibVoronoi::IsGroupBorderEdge(GroupNode, InEdge)) {
						if (DoorEdgeIndices.Contains(InEdge->Index)) {
							FCellFlowLibVoronoi::EmitDoorEdgeMarkers(EmitMarkerContext, InEdge, FnEmitMarker, FnEmitVoxelEdgeMarker, FVoxelBuilderMarkers::INTERNAL_VOXEL_DOOR);
						}
						else {
							FCellFlowLibVoronoi::EmitEdgeMarkers(EmitMarkerContext, InEdge, FnEmitMarker, FnEmitVoxelEdgeMarker, FVoxelBuilderMarkers::INTERNAL_VOXEL_WALL);
						}
					}
					
					if (const FDAFlowCellGraphDCELEdgeInfo* StairInfo = CellGraph->DCELInfo.Stairs.Find(InEdge->Index)) {
						const FVector Scale = FVector(1, StairInfo->LogicalWidth, 1);
						const FQuat Rotation = FQuat::FindBetweenVectors({0, -1, 0}, StairInfo->Direction);
						const FVector BaseLocation = StairInfo->LogicalLocation;
						const FVector Location = (BaseLocation + StairInfo->Direction * 0.5f) * GridSize;

						FString StairMarkerName = FCellFlowBuilderMarkers::STAIR;
						if (const FCellFlowVoronoiMarkerSetup* MarkerSetup = EmitMarkerContext.GroupNodeChunkMarkers.Find(GroupNode.GroupId)) {
							FCellFlowSizedMarkerDef StairEdgeMarker;
							if (DA::FCellFlowLib::SelectRandomWeightedMarker(MarkerSetup->StairMarker, Random, StairEdgeMarker)) {
								StairMarkerName = StairEdgeMarker.MarkerName;
							}
						}
						
						FTransform WorldTransform = FTransform(FRotator(0, -90, 0)) * FTransform(Rotation, Location, Scale);
						EmitCellMarker(StairMarkerName, DungeonTransform, WorldTransform);

						FTransform StairTransform = WorldTransform * DungeonTransform;
						FnEmitVoxelStairMarker(FVoxelBuilderMarkers::INTERNAL_VOXEL_STAIR, StairTransform);
					}
				});

			}
		}
	}

	// Emit the spawned item markers
	for (const FDAFlowCellGraphSpawnInfo& SpawnInfo : CellGraph->DCELInfo.SpawnInfo) {
		if (!SpawnInfo.Item.IsValid()) {
			continue;
		}
		const UFlowGraphItem* Item = SpawnInfo.Item.Get();
		const FVector Location = SpawnInfo.Coord * GridSize;
		EmitCellMarker(Item->MarkerName, DungeonTransform, FTransform(Location));
	}

	// Emit Site points markers
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		if (UFlowAbstractNode* LayoutNode = GraphQuery.GetNode(GroupNode.LayoutNodeID)) {
			if (LayoutNode && LayoutNode->bActive) {
				FVector GroupSiteWorldLoc = LayoutNode->Coord * GridSize;
				EmitCellMarker(FCellFlowBuilderMarkers::SITE_GROUP, DungeonTransform, FTransform(GroupSiteWorldLoc));
				for (const int LeafNodeId : GroupNode.LeafNodes) {
					if (const UDAFlowCellLeafNode* LeafNode = CellGraph->LeafNodes[LeafNodeId]) {
						FVector SiteWorldLoc = FVector(LeafNode->GetCenter(), LayoutNode->Coord.Z) * GridSize;
						EmitCellMarker(FCellFlowBuilderMarkers::SITE, DungeonTransform, FTransform(SiteWorldLoc));
					}
				}
			}
		}
	}
}

bool UCellFlowBuilder::EmitProceduralMarkers(const UProceduralMarkerEmitter* InProceduralMarkerEmitter) {
	if (!CellConfig.IsValid() || !CellModel.IsValid() || !CellModel->CellGraph || !CellModel->LayoutGraph) {
		return false;
	}
	
	const FTransform DungeonTransform = GetDungeonTransform();
	const FVector GridSize = CellConfig->GridSize;
	
	if (const UBoxBoundaryMarkerEmitter* BoxBoundaryEmitter = Cast<UBoxBoundaryMarkerEmitter>(InProceduralMarkerEmitter)) {
		FIntVector CoordMin, CoordMax;
		if (!GetDungeonBounds(CoordMin, CoordMax)) {
			return false;
		}

		BoxBoundaryEmitter->EmitMarkers(GridSize, CoordMin, CoordMax, DungeonTransform, [this](const FString& InMarkerName, const FTransform& InTransform) {
			AddMarker(InMarkerName, InTransform);
		});
	}

	return true;
}

void UCellFlowBuilder::GenerateBaseMesh(UWorld* InWorld, const FGuid& InDungeonId, const FTransform& InTransform) const {
	if (!InWorld || !CellConfig.IsValid()) {
		return;
	}
	if (!CellConfig->bGenerateVoronoiBaseMesh || CellModel->VoronoiData->Sites.Num() == 0) {
		return;
	}

	int LODIndex = 0;
	int SubDivs = CellConfig->DefaultVoronoiMeshProfile.NumRenderSubDiv;
	const int CollisionSubDivs = CellConfig->DefaultVoronoiMeshProfile.NumCollisionSubDiv;
	const bool bApplyNoise = CellConfig->DefaultVoronoiMeshProfile.bApplyNoise;
	const bool bSmoothNormals = CellConfig->DefaultVoronoiMeshProfile.bSmoothNormals;
	const FCellFlowMeshNoiseSettings& NoiseSettings = CellConfig->DefaultVoronoiMeshProfile.NoiseSettings;
		

	struct FLODData {
		TArray<FFlowVisLib::FGeometry> SurfaceGeometries;
		bool bGenerateCollision{};
	};
	TArray<FLODData> LODList;
	LODList.SetNum(SubDivs + 1);

	while (SubDivs >= 0) {
		FLODData& LODInfo = LODList[LODIndex];
		FRandomStream MeshGenRandom(CellConfig->Seed);
		FCellFlowLevelMeshGenSettings RenderSettings;
		RenderSettings.VisualizationScale = CellConfig->GridSize;
		RenderSettings.NumSubDiv = SubDivs;
		RenderSettings.bSmoothNormals = bSmoothNormals;
		RenderSettings.Random = &MeshGenRandom;
		RenderSettings.bApplyNoise = bApplyNoise;
		RenderSettings.NoiseSettings = NoiseSettings;
		RenderSettings.bGeneratedMergedMesh = false;	// We want one mesh actor for each chunk
#if WITH_EDITORONLY_DATA
		RenderSettings.bRenderInactive = CellModel->CellGraph ? CellModel->CellGraph->bRenderInactiveGroups : true;
#endif // WITH_EDITORONLY_DATA

		using namespace UE::Geometry;
		FCellFlowMeshGenDCEL::Generate(CellModel->LayoutGraph, CellModel->CellGraph, CellModel->VoronoiData, RenderSettings, LODInfo.SurfaceGeometries, nullptr);
		LODInfo.bGenerateCollision = (SubDivs == CollisionSubDivs);

		SubDivs--;
		LODIndex++;
	}

	// Spawn the chunk actors
	if (LODList.Num() > 0) {
		const int32 NumLODs = LODList.Num();
		const int32 NumChunks = LODList[0].SurfaceGeometries.Num();
		for (int SurfaceIdx = 0; SurfaceIdx < NumChunks; SurfaceIdx++) {
			// Bring the origin of the 
			ACellFlowChunkMesh* ChunkMesh = InWorld->SpawnActor<ACellFlowChunkMesh>();
			ChunkMesh->DungeonID = InDungeonId;

			// Calculate the offset
			FVector3d Offset;
			{
				FFlowVisLib::FGeometry& SurfaceGeometry = LODList[0].SurfaceGeometries[SurfaceIdx];
				FBox ChunkBounds = FFlowVisLib::GetBounds(SurfaceGeometry);
				Offset = ChunkBounds.GetCenter();
				Offset.Z = ChunkBounds.Max.Z;
				FTransform ChunkTransform = FTransform(Offset) * InTransform;
				ChunkMesh->SetActorTransform(ChunkTransform);
			}

			
			for (int LODIdx = 0; LODIdx < LODList.Num(); LODIdx++) {
				FLODData& LODInfo = LODList[LODIdx];
				FFlowVisLib::FGeometry& SurfaceGeometry = LODInfo.SurfaceGeometries[SurfaceIdx];
				FFlowVisLib::TranslateGeometry(-Offset, SurfaceGeometry);
				ChunkMesh->UploadGeometry(LODIdx, 0, SurfaceGeometry, LODInfo.bGenerateCollision);
			}
				
			// Set the material
			// TODO: Set the correct material, based on the chunk path id
			UMaterialInterface* Material = CellConfig->DefaultVoronoiMeshProfile.Material;
			ChunkMesh->GetMeshComponent()->SetMaterial(0, Material);
			ChunkMesh->GetMeshComponent()->LODFactorScale = CellConfig->DefaultVoronoiMeshProfile.LODFactorScale;

#if WITH_EDITOR
			if (ChunkMesh && Dungeon) {
				ChunkMesh->SetFolderPath(Dungeon->ItemFolderPath);
			}
#endif
		}
	}
}

void UCellFlowBuilder::DestroyBaseMesh(UWorld* InWorld, const FGuid& InDungeonId) const {
	if (InWorld) {
		FDungeonUtils::DestroyManagedActor<ACellFlowChunkMesh>(InWorld, InDungeonId);
	}
}

bool UCellFlowBuilder::GetDungeonBounds(FIntVector& OutMin, FIntVector& OutMax) const {
	if (!CellConfig.IsValid() || !CellModel.IsValid() || !CellModel->CellGraph || !CellModel->LayoutGraph) {
		return false;
	}
	
	UDAFlowCellGraph* CellGraph = CellModel->CellGraph;
	const FFlowAbstractGraphQuery GraphQuery(CellModel->LayoutGraph);

	bool bBoundsInitialized{};
	
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		if (!GroupNode.IsActive()) continue;

		const UFlowAbstractNode* LayoutNode = GraphQuery.GetNode(GroupNode.LayoutNodeID);
		if (!LayoutNode || !LayoutNode->bActive) {
			continue;
		}
		
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeId])) {
				if (GridLeafNode->LeafTileCoords.Num() > 0) {
					if (!bBoundsInitialized) {
						OutMin = OutMax = FIntVector(GridLeafNode->LeafTileCoords[0].X, GridLeafNode->LeafTileCoords[0].Y, GridLeafNode->LogicalZ);
						bBoundsInitialized = true;
					}
					for (const FIntPoint& Coord : GridLeafNode->LeafTileCoords) {
						OutMin.X = FMath::Min(OutMin.X, Coord.X);
						OutMin.Y = FMath::Min(OutMin.Y, Coord.Y);
						OutMin.Z = FMath::Min(OutMin.Z, GridLeafNode->LogicalZ);

						OutMax.X = FMath::Max(OutMax.X, Coord.X + 1);
						OutMax.Y = FMath::Max(OutMax.Y, Coord.Y + 1);
						OutMax.Z = FMath::Max(OutMax.Z, GridLeafNode->LogicalZ);
					}
				}
			}
		}
	}
	return bBoundsInitialized;
}

void UCellFlowBuilder::CreateDebugVisualizations(UWorld* InWorld, const FGuid& InDungeonId, const FTransform& InTransform) const {
	DestroyDebugVisualizations(InWorld, InDungeonId);

	if (!CellModel.IsValid() || !CellConfig.IsValid()) {
		return;
	}
	if (CellModel->LayoutGraph && CellModel->CellGraph) {
		UWorld* World = GetWorld();
		
		const float NodeRadius = CellConfig->GridSize.Z >= 50
				? CellConfig->GridSize.Z * 0.5f
				: FMath::Max(CellConfig->GridSize.X, CellConfig->GridSize.Y) * 0.3f;
		
		FTransform BaseTransform = GetDungeonTransform();
		const FTransform LayoutGraphOffsetZ = FTransform(FVector(0, 0, NodeRadius * 3));
		
		AFlowLayoutGraphVisualizer* LayoutVisualizer = World->SpawnActor<AFlowLayoutGraphVisualizer>();
		LayoutVisualizer->DungeonID = InDungeonId;
		LayoutVisualizer->SetAutoAlignToLevelViewport(true);
		LayoutVisualizer->SetActorTransform(LayoutGraphOffsetZ * BaseTransform);

		FDAAbstractGraphVisualizerSettings VisualizerSettings;
		VisualizerSettings.NodeRadius = NodeRadius; // ModuleWidth * 0.05;
		VisualizerSettings.LinkThickness = VisualizerSettings.NodeRadius * 0.2f;
		VisualizerSettings.LinkRefThickness = VisualizerSettings.LinkThickness * 0.5f;
		VisualizerSettings.NodeSeparationDistance = CellConfig->GridSize;
		VisualizerSettings.DisabledNodeScale = 0.6f;
		VisualizerSettings.DisabledNodeOpacity = 0.75f;
		LayoutVisualizer->Generate(CellModel->LayoutGraph, VisualizerSettings);

		ACellFlowLayoutVisualization* CellVisualizer = World->SpawnActor<ACellFlowLayoutVisualization>();
		CellVisualizer->DungeonID = InDungeonId;
		CellVisualizer->SetActorTransform(BaseTransform);
		CellVisualizer->SetActorLocation(BaseTransform.GetLocation() + FVector(0, 0, 5));	// Raise it up a bit to avoid z fighting with the ground meshes and to show the debug colors on the ground

		FCellFlowLevelMeshGenSettings RenderSettings;
		RenderSettings.bGeneratedMergedMesh = true;
		RenderSettings.bApplyNoise = false;
		RenderSettings.VisualizationScale = CellConfig->GridSize;
		
#if WITH_EDITORONLY_DATA
		RenderSettings.bRenderInactive = CellModel->CellGraph ? CellModel->CellGraph->bRenderInactiveGroups : true;
#endif // WITH_EDITORONLY_DATA
		
		CellVisualizer->Generate(CellModel->LayoutGraph, CellModel->CellGraph, CellModel->VoronoiData, RenderSettings);
	}
}

void UCellFlowBuilder::DestroyDebugVisualizations(UWorld* InWorld, const FGuid& InDungeonId) const {
	FDungeonUtils::DestroyManagedActor<AFlowLayoutGraphVisualizer>(InWorld, InDungeonId);
	FDungeonUtils::DestroyManagedActor<ACellFlowLayoutVisualization>(InWorld, InDungeonId);
}

FDungeonMarkerInstance& UCellFlowBuilder::EmitCellMarker(const FString& InMarkerName, const FTransform& InDungeonTransform, const FTransform& InLocalTransform) {
	return AddMarker(InMarkerName, InLocalTransform * InDungeonTransform);
}

TSubclassOf<UDungeonModel> UCellFlowBuilder::GetModelClass() {
	return UCellFlowModel::StaticClass();
}

TSubclassOf<UDungeonConfig> UCellFlowBuilder::GetConfigClass() {
	return UCellFlowConfig::StaticClass();
}

TSubclassOf<UDungeonToolData> UCellFlowBuilder::GetToolDataClass() {
	return UCellFlowToolData::StaticClass();
}

TSubclassOf<UDungeonQuery> UCellFlowBuilder::GetQueryClass() {
	return UCellFlowQuery::StaticClass();
}

void UCellFlowBuilder::CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules) {
	Super::CreateMarkerVisualizationRules(OutRules);
	OutRules = {};

	if (!CellConfig.IsValid()) {
		CellConfig = Cast<UCellFlowConfig>(DungeonConfig);
	}
	
	
	const FTransform HalfWallOffset = FTransform(FQuat::Identity, FVector(0, 0, 0), FVector(1, 1, 0.5f));
	const FTransform EdgeRuleOffset = FTransform(FQuat::Identity, FVector(0, 0, 0), FVector(1, 1, -0.05f));
	
	OutRules.SetGridSize(CellConfig.IsValid() ? CellConfig->GridSize : FVector::ZeroVector);
    
	OutRules.Set(FCellFlowBuilderMarkers::GROUND, { EDungeonMarkerVisualizationShape::Tile });
	OutRules.Set(FCellFlowBuilderMarkers::CEILING, { EDungeonMarkerVisualizationShape::Tile });
	OutRules.Set(FCellFlowBuilderMarkers::CEILING_EDGE_TRIM, { EDungeonMarkerVisualizationShape::Wall, EdgeRuleOffset });
	OutRules.Set(FCellFlowBuilderMarkers::STAIR, { EDungeonMarkerVisualizationShape::Tile });
    
	OutRules.Set(FCellFlowBuilderMarkers::WALL, { EDungeonMarkerVisualizationShape::Wall });
    OutRules.Set(FCellFlowBuilderMarkers::WALL_HALF, { EDungeonMarkerVisualizationShape::Wall, HalfWallOffset });
    OutRules.Set(FCellFlowBuilderMarkers::DOOR_WALL_HALF, { EDungeonMarkerVisualizationShape::Wall, HalfWallOffset });
	OutRules.Set(FCellFlowBuilderMarkers::DOOR, { EDungeonMarkerVisualizationShape::Wall });
	OutRules.Set(FCellFlowBuilderMarkers::DOOR_ONEWAY, { EDungeonMarkerVisualizationShape::Wall });
    
	OutRules.Set(FCellFlowBuilderMarkers::WALL_SEPARATOR, { EDungeonMarkerVisualizationShape::Pillar });
}

void UCellFlowBuilder::GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) {
	OutMarkerNames.Reset();
	OutMarkerNames.Add(FCellFlowBuilderMarkers::GROUND);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::CEILING);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::CEILING_EDGE_TRIM);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::WALL);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::WALL_HALF);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::DOOR_WALL_HALF);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::WALL_SEPARATOR);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::STAIR);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::DOOR);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::DOOR_ONEWAY);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::SITE);
	OutMarkerNames.Add(FCellFlowBuilderMarkers::SITE_GROUP);
}

bool UCellFlowBuilder::PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
	if (CellModel.IsValid() && CellConfig.IsValid() && CellQuery.IsValid()) {
		for (UDungeonSelectorLogic* SelectionLogic : SelectionLogics) {
			UCellFlowSelectorLogic* CellFlowSelectionLogic = Cast<UCellFlowSelectorLogic>(SelectionLogic);
			if (!CellFlowSelectionLogic) {
				UE_LOG(CellFlowBuilderLog, Warning,
					   TEXT("Invalid selection logic specified.  CellFlowSelectorLogic expected"));
				return false;
			}

			const bool bSelected = CellFlowSelectionLogic->SelectNode(CellModel.Get(), CellConfig.Get(), this, CellQuery.Get(), Random, InMarker.Transform);
			if (!bSelected) {
				return false;
			}
		}
	}
    
	return true;
}

FTransform UCellFlowBuilder::PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) {
	FTransform Result = FTransform::Identity;

	if (CellModel.IsValid() && CellConfig.IsValid() && CellQuery.IsValid()) {
		for (UDungeonTransformLogic* TransformLogic : TransformLogics) {
			UCellFlowTransformLogic* CellFlowTransformLogic = Cast<UCellFlowTransformLogic>(TransformLogic);
			if (!CellFlowTransformLogic) {
				UE_LOG(CellFlowBuilderLog, Warning,
					   TEXT("Invalid transform logic specified.  CellFlowTransformLogic expected"));
				continue;
			}

			FTransform LogicOffset;
			if (TransformLogic) {
				CellFlowTransformLogic->GetNodeOffset(CellModel.Get(), CellConfig.Get(), CellQuery.Get(), Random, InMarker.Transform, LogicOffset);
			}
			else {
				LogicOffset = FTransform::Identity;
			}

			FTransform Out;
			FTransform::Multiply(&Out, &LogicOffset, &Result);
			Result = Out;
		}
	}
    
	return Result;
}

TSharedPtr<IMarkerGenProcessor> UCellFlowBuilder::CreateMarkerGenProcessor(const FTransform& InDungeonTransform) const {
	if (const UCellFlowConfig* Config = CellConfig.IsValid() ? CellConfig.Get() : Cast<UCellFlowConfig>(DungeonConfig)) {
		const FVector GridSize = Config->GridSize;
		return MakeShareable(new FMarkerGenGridProcessor(InDungeonTransform, GridSize));
	}
	return nullptr;
}

bool UCellFlowBuilder::ExecuteGraph(UWorld* InWorld) {
	CellModel->LayoutGraph = nullptr;

	const UCellFlowAsset* CellFlowAsset = CellConfig->CellFlow.LoadSynchronous();

    if (!CellFlowAsset) {
        UE_LOG(CellFlowBuilderLog, Error, TEXT("Missing Cell Flow graph"));
        return false;
    }

    if (!CellFlowAsset->ExecScript) {
        UE_LOG(CellFlowBuilderLog, Error, TEXT("Invalid Cell Flow graph state. Please resave in editor"));
        return false;
    }

    FFlowProcessor FlowProcessor;
    
    // Register the domains
    {
        FCellFlowProcessDomainExtender Extender;
        Extender.ExtendDomains(FlowProcessor);
    }

	TMap<FName, FVariant> BuilderAttributes;
	BuilderAttributes.Add(DA::CellFlow::FConstants::AttributeNameGridSize, CellConfig->GridSize);

	TArray<TWeakObjectPtr<AActor>> ContextActors;
	for (TActorIterator<ADungeonVolume> It(InWorld); It; ++It) {
		if (ADungeonVolume* Volume = *It) {
			if (IsValid(Volume->Dungeon) && Volume->Dungeon == Dungeon) {
				ContextActors.Add(Volume);
			}
		}
	}
	
    const int32 MAX_RETRIES = FMath::Max(1, CellConfig->MaxRetries);
    int32 NumTries = 0;
    FFlowProcessorResult Result;
    while (NumTries < MAX_RETRIES) {
        FFlowProcessorSettings FlowProcessorSettings;
        FlowProcessorSettings.AttributeList = AttributeList;
        FlowProcessorSettings.SerializedAttributeList = CellConfig->ParameterOverrides;
    	FlowProcessorSettings.Dungeon = Dungeon;
    	FlowProcessorSettings.CustomParameters = BuilderParameters.CustomParameters;
    	FlowProcessorSettings.BuilderAttributes = BuilderAttributes;
    	FlowProcessorSettings.ContextActors = ContextActors;
        Result = FlowProcessor.Process(CellFlowAsset->ExecScript, Random, FlowProcessorSettings);
        NumTries++;
        if (Result.ExecResult == EFlowTaskExecutionResult::Success) {
            break;
        }
        if (Result.ExecResult == EFlowTaskExecutionResult::FailHalt) {
            break;
        }
    }

    if (Result.ExecResult != EFlowTaskExecutionResult::Success) {
        UE_LOG(CellFlowBuilderLog, Error, TEXT("Failed to generate cell flow graph"));
        return false;
    }

    if (!CellFlowAsset->ExecScript->ResultNode) {
        UE_LOG(CellFlowBuilderLog, Error, TEXT("Cannot find result node in the grid flow exec graph. Please resave the grid flow asset in the editor"));
        return false;
    }

    const FGuid ResultNodeId = CellFlowAsset->ExecScript->ResultNode->NodeId;
    if (FlowProcessor.GetNodeExecStage(ResultNodeId) != EFlowTaskExecutionStage::Executed) {
        UE_LOG(CellFlowBuilderLog, Error, TEXT("Grid Flow Graph execution failed"));
        return false;
    }

    FFlowExecutionOutput ResultNodeState;
    FlowProcessor.GetNodeState(ResultNodeId, ResultNodeState);
    if (ResultNodeState.ExecutionResult != EFlowTaskExecutionResult::Success) {
        UE_LOG(CellFlowBuilderLog, Error, TEXT("Grid Flow Result node execution did not succeed"));
        return false;
    }

    // Save a copy in the model
    if (CellModel.IsValid()) {
        UCellFlowModel* CellModelPtr = CellModel.Get();

    	// Clone the layout graph and save it in the model
	    {
		    const UCellFlowLayoutGraph* ResultLayoutGraphState = ResultNodeState.State->GetState<UCellFlowLayoutGraph>(UFlowAbstractGraphBase::StateTypeID);
        	CellModel->LayoutGraph = NewObject<UCellFlowLayoutGraph>(CellModelPtr, "LayoutGraph");
        	if (ResultLayoutGraphState) {
        		CellModel->LayoutGraph->CloneFromStateObject(ResultLayoutGraphState);
        	}
	    }

    	// Clone the cell graph and save it in the model
        {
        	const UDAFlowCellGraph* ResultCellGraphState = ResultNodeState.State->GetState<UDAFlowCellGraph>(UDAFlowCellGraph::StateTypeID);
        	CellModel->CellGraph = NewObject<UDAFlowCellGraph>(CellModelPtr, "CellGraph");
        	if (ResultCellGraphState) {
        		CellModel->CellGraph->CloneFromStateObject(ResultCellGraphState);
        	}
        }

    	// Clone the Voronoi graph data
        {
        	const UCellFlowVoronoiGraph* ResultVoronoiData = ResultNodeState.State->GetState<UCellFlowVoronoiGraph>(UCellFlowVoronoiGraph::StateTypeID);
        	CellModel->VoronoiData = NewObject<UCellFlowVoronoiGraph>(CellModelPtr, "VoronoiData");
        	if (ResultVoronoiData) {
        		CellModel->VoronoiData->CloneFromStateObject(ResultVoronoiData);
        	}
        }
    }
    return true;
}

void FCellFlowProcessDomainExtender::ExtendDomains(FFlowProcessor& InProcessor) {
	// Register layout nodes
	InProcessor.RegisterDomain(MakeShared<FCellFlowLayoutGraphDomain>());
}

// My son's first line of code :) 02-Sep-23: 0.0t54f0-ok,;l5v,ṭxcccgbjk m  weffffffffzasssssssss3ce

