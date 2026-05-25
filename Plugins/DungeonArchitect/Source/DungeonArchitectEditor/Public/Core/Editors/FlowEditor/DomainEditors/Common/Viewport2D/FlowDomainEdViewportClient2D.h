//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewport2DBehaviorTargets.h"

#include "EditorViewportClient.h"
#include "InputBehaviorSet.h"

class FFlowViewportCanvasRenderer;

/**
 * Client used to display a 2D view, implemented by using a perspective viewport with a locked
 * camera.
 */
class FFlowDomainEdViewportClient2D  : public FEditorViewportClient, public IInputBehaviorSource {
public:
	FFlowDomainEdViewportClient2D(FEditorModeTools* InModeTools, FPreviewScene* InPreviewScene = nullptr,
		const TWeakPtr<SEditorViewport>& InEditorViewportWidget = nullptr);

	virtual ~FFlowDomainEdViewportClient2D() override {}

	// FEditorViewportClient
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual bool ShouldOrbitCamera() const override;
	virtual bool CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	// Overriding base class visibility
	using FEditorViewportClient::OverrideNearClipPlane;

	// IInputBehaviorSource
	virtual const UInputBehaviorSet* GetInputBehaviors() const override;

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

protected:
	virtual void DrawImpl(const FFlowViewportCanvasRenderer& InRenderer);

private:
	void DrawGrid(const FFlowViewportCanvasRenderer& InRenderer) const;
	
protected:
	// These get added in AddReferencedObjects for memory management
	TObjectPtr<UInputBehaviorSet> BehaviorSet;

	// Note that it's generally less hassle if the unique ptr types are complete here,
	// not forward declared, else we get compile errors if their destruction shows up
	// anywhere in the header.
	TUniquePtr<FFlowDomainEd2DScrollBehaviorTargets> ScrollBehaviorTarget;
	TUniquePtr<FFlowDomainEd2DMouseWheelZoomBehaviorTargets> ZoomBehaviorTarget;
};

