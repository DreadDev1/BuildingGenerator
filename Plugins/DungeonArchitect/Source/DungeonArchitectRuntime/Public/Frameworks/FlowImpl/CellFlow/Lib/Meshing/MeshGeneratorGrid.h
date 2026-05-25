//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Utils/FlowVisLib.h"

struct FCellFlowLevelMeshGenSettings;
class UDAFlowCellGraph;
class UCellFlowLayoutGraph;

class FCellFlowMeshGenGrid {
public:
	static void Generate(UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph, const FCellFlowLevelMeshGenSettings& InSettings,
												FFlowVisLib::FGeometry& OutSurfaceGeometry, FFlowVisLib::FGeometry& OutLineGeometry);

};

