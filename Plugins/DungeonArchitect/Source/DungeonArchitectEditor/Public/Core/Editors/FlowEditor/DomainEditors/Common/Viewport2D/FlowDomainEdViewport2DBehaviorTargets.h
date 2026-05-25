//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "BaseBehaviors/BehaviorTargetInterfaces.h"

class FFlowDomainEdViewportClient2D;

/**
 * Allows click-dragging to move the camera in the XY plane.
 */
class FFlowDomainEd2DScrollBehaviorTargets : public IClickDragBehaviorTarget {
public:

	FFlowDomainEd2DScrollBehaviorTargets(FFlowDomainEdViewportClient2D* ViewportClientIn);

	// IClickDragBehaviorTarget
	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override;

protected:
	FFlowDomainEdViewportClient2D* ViewportClient = nullptr;
	FVector3d DragStart;
};


/**
 * Allows the mouse wheel to move the camera forwards/backwards relative to the XY plane,
 * in the direction pointed to by the mouse.
 */
class FFlowDomainEd2DMouseWheelZoomBehaviorTargets : public IMouseWheelBehaviorTarget
{
public:

	FFlowDomainEd2DMouseWheelZoomBehaviorTargets(FFlowDomainEdViewportClient2D* ViewportClientIn);

	// IMouseWheelBehaviorTarget
	virtual FInputRayHit ShouldRespondToMouseWheel(const FInputDeviceRay& CurrentPos) override;
	virtual void OnMouseWheelScrollUp(const FInputDeviceRay& CurrentPos) override;
	virtual void OnMouseWheelScrollDown(const FInputDeviceRay& CurrentPos) override;

private:
	void PerformZoom(const FInputDeviceRay& CurrentPos, float Direction) const;
	
protected:
	FFlowDomainEdViewportClient2D* ViewportClient = nullptr;
	float ZoomLevel = 1.0f;
	const float ZoomIncrements = 0.1f;
};

