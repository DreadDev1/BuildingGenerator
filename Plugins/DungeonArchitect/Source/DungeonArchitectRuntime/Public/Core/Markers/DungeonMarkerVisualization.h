//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "DungeonMarkerVisualization.generated.h"

UENUM(Blueprintable)
enum class EDungeonMarkerVisualizationShape : uint8 {
	None,
	Tile,
	Wall,
	Pillar,
	Point,
};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDungeonMarkerVisualizationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
	EDungeonMarkerVisualizationShape VisualizationShape = EDungeonMarkerVisualizationShape::None;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
	FTransform Offset = FTransform::Identity;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonMarkerVisualizationRules {
public:
	FDungeonMarkerVisualizationData Get(const FString& MarkerName) const;
	void Set(const FString& InMarkerName, const FDungeonMarkerVisualizationData& InData);

	FVector GetGridSize() const { return GridSize; }
    void SetGridSize(const FVector& InGridSize) { GridSize = InGridSize; }
	
private:
	TMap<FString, FDungeonMarkerVisualizationData> MarkerVisualizations;
	FVector GridSize = FVector::ZeroVector;
	
	static FDungeonMarkerVisualizationData Default;
};

class FPrimitiveDrawInterface;

class DUNGEONARCHITECTRUNTIME_API FDungeonMarkerVisualizer {
public:
	FDungeonMarkerVisualizer();
	void RenderMarker(FPrimitiveDrawInterface* PDI, const FString& InMarkerName, const FTransform& InMarkerTransform, const FDungeonMarkerVisualizationRules& InRules);

	void SetDepthPriorityGroup(const ESceneDepthPriorityGroup InDepthPriorityGroup);
	void SetDrawColor(const FLinearColor& InDrawColor);
	void SetLineThickness(float InLineThickness);
	void SetPointThickness(float InPointThickness);
	void SetDepthBias(float InDepthBias);
	void SetDrawScreenSpace(bool bInScreenSpace);

private:
	void RenderTile(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, const FVector& InGridSize);
	void RenderWall(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, const FVector& InGridSize, const FTransform& InOffset);
	void RenderPillar(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, const FVector& InGridSize, const FTransform& InOffset);
	void RenderPoint(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, bool bDrawCircle = false);
	void RenderPoint(FPrimitiveDrawInterface* PDI, const FVector& InLocation, bool bDrawCircle = false);

private:
	ESceneDepthPriorityGroup DepthPriorityGroup = SDPG_Foreground;
	FLinearColor DrawColor = FLinearColor(1, 0, 0, 0.5f);
	float LineThickness = 10.0f;
	float PointThickness = 10.0f;
	float DepthBias = 0.0f;
	bool bDrawScreenSpace = true;

private:
	static FVector2D LocalSquarePoints[4];
};

