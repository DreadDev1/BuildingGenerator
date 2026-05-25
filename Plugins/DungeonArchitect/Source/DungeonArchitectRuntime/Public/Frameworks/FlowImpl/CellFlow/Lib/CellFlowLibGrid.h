//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarker.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"

struct FDAFlowCellGraphGridInfo;
class UMarkerGenLayer;
struct FCellFlowLayoutTaskScatterPropSettings;
struct FCellFlowGridEdgeInfo;

class FCellFlowLibGrid {
public:
	struct FCellFlowGridMarkerContext {
		FVector GridSize;
		TSet<FIntVector> InsertedPillarCoords;
		TMap<int32, FCellFlowGridMarkerSetup> GroupNodeChunkMarkers;

		float VoxelWallThickness = 100;
		float VoxelTotalCeilingHeight = 800;
	};
	
	typedef TFunction<FDungeonMarkerInstance&(const FString& /* MarkerName */, const FTransform& /* MarkerTransform */)> FuncEmitGridCellEdgeMarker;
	static void TransformPatternLayer(UMarkerGenLayer* Layer, const FCellFlowLayoutTaskScatterPropSettings& Settings);
};

