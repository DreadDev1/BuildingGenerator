//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "ComponentVisualizer.h"
#include "UObject/GCObject.h"

class UTexture;
class UMaterialInstanceDynamic;

class FDungeonCanvasOverlayTextureVisualizer : public FComponentVisualizer, public FGCObject {
public:
	FDungeonCanvasOverlayTextureVisualizer();
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	virtual bool ShouldShowForSelectedSubcomponents(const UActorComponent* Component) override { return bShowWhenSelected; }
	
private:
	TObjectPtr<UMaterialInstanceDynamic> OverlayMaterial{};
	TWeakObjectPtr<UTexture> WhiteTexture;
	bool bShowWhenSelected = true;
};

