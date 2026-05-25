//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Utils/Debug/SceneDebugDataComponent.h"

#include "DynamicMeshActor.h"
#include "GameFramework/Actor.h"
#include "CellFlowLayoutVisualization.generated.h"

class UFlowAbstractNode;
class UCellFlowVoronoiGraph;
class UDAFlowCellGraph;
class UCellFlowLayoutGraph;
class UDynamicMeshComponent;


struct DUNGEONARCHITECTRUNTIME_API FCellFlowLevelMeshGenSettings {
	FVector VisualizationScale{};
	bool bRenderInactive{true};
	bool bGeneratedMergedMesh{};
	int NumSubDiv{0};
	FRandomStream* Random{};
	bool bSmoothNormals{false};
	bool bApplyNoise{true};
	FCellFlowMeshNoiseSettings NoiseSettings{};
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API ACellFlowLevelMeshGrid : public AActor {
	GENERATED_BODY()
	
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API ACellFlowLayoutVisualization : public AActor {
	GENERATED_UCLASS_BODY()
	
public:
	void Generate(UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph, const UCellFlowVoronoiGraph* VoronoiData, const FCellFlowLevelMeshGenSettings& InSettings) const;
	TObjectPtr<UDASceneDebugDataComponent> GetDebugComponent() const { return DebugData; }
	
public:
	UPROPERTY()
	FGuid DungeonID;
	
private:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UDASceneDebugDataComponent> DebugData;
	
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> QuadSurfaceComponent;
	
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> VoronoiSurfaceComponent;
	
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> LineComponent;
};


