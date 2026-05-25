//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport3D/FlowDomainEdViewport3D.h"

#include "ComponentVisualizer.h"

class DUNGEONARCHITECTEDITOR_API FFlowDomainEdViewportClient3D
	: public FEditorViewportClient
	, public TSharedFromThis<FFlowDomainEdViewportClient3D>
{
public:
	FFlowDomainEdViewportClient3D(FPreviewScene& InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewport);
	FFDViewportActorMouseEvent& GetActorSelectionChanged() { return ActorSelectionChanged; }
	FFDViewportActorMouseEvent& GetActorDoubleClicked() { return ActorDoubleClicked; }
    
	// FEditorViewportClient interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual void SetupViewForRendering(FSceneViewFamily& ViewFamily, FSceneView& View) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	// End of FEditorViewportClient interface

	void RegisterComponentForVisualization(const TSharedPtr<FComponentVisualizer>& InComponentVisualizer, TObjectPtr<UActorComponent> InComponent);
	void ClearComponentVisualizers();

private:
	FFDViewportActorMouseEvent ActorSelectionChanged;
	FFDViewportActorMouseEvent ActorDoubleClicked;

	struct FComponentVisualizeInfo {
		TSharedPtr<FComponentVisualizer> Visualizer;
		TWeakObjectPtr<UActorComponent> ComponentPtr;
	};
	TArray<FComponentVisualizeInfo> ComponentsToVisualize;
};


