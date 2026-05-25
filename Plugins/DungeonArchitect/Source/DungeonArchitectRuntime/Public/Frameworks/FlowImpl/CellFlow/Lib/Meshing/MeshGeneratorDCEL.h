//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Utils/FlowVisLib.h"

class UCellFlowVoronoiGraph;
struct FCellFlowLevelMeshGenSettings;
class UDAFlowCellGraph;
class UCellFlowLayoutGraph;

class FCellFlowMeshGenDCEL {
public:
	static void Generate(const UCellFlowLayoutGraph* InLayoutGraph, const UDAFlowCellGraph* InCellGraph, const UCellFlowVoronoiGraph* InVoronoiData,
							const FCellFlowLevelMeshGenSettings& InSettings, TArray<FFlowVisLib::FGeometry>& OutGeometries,
							FFlowVisLib::FGeometry* OutLineGeometry = nullptr);

private:
	
};

