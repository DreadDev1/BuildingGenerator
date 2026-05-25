//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class FDASceneDebugDataComponentVisualizer : public FComponentVisualizer {
public:
	virtual void DrawVisualizationHUD(const UActorComponent* InComponent, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};

