//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeGraphNodeData.h"
#include "DungeonThemeCompiledGraph.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonThemeCompiledGraph {
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FDungeonThemeMarkerNodeData> MarkerNodes;
	
	UPROPERTY()
	TArray<FDungeonThemeVisualNodeData> VisualNodes;
	
	UPROPERTY()
	TArray<FDungeonThemeMarkerEmitterNodeData> MarkerEmitterNodes;
};

