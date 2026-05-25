//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewport2DBehaviorTargets.h"

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewportClient2D.h"

// FUVEditor2DScrollBehaviorTarget

DEFINE_LOG_CATEGORY_STATIC(LogFlowInput, Log, All);

FFlowDomainEd2DScrollBehaviorTargets::FFlowDomainEd2DScrollBehaviorTargets(FFlowDomainEdViewportClient2D* ViewportClientIn)
	: ViewportClient(ViewportClientIn)
{
}

FInputRayHit FFlowDomainEd2DScrollBehaviorTargets::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	// Verify that ray is facing the proper direction
	if (PressPos.WorldRay.Direction.Z * PressPos.WorldRay.Origin.Z < 0)
	{
		return FInputRayHit(-PressPos.WorldRay.Origin.Z / PressPos.WorldRay.Direction.Z);
	}
	return FInputRayHit();
}

void FFlowDomainEd2DScrollBehaviorTargets::OnClickPress(const FInputDeviceRay& PressPos)
{
	if (ensure(PressPos.WorldRay.Direction.Z * PressPos.WorldRay.Origin.Z < 0))
	{
		// Intersect with XY plane
		double T = -PressPos.WorldRay.Origin.Z / PressPos.WorldRay.Direction.Z;
		DragStart = FVector3d(
			PressPos.WorldRay.Origin.X + T * PressPos.WorldRay.Direction.X,
			PressPos.WorldRay.Origin.Y + T * PressPos.WorldRay.Direction.Y,
			0);
	}
}

void FFlowDomainEd2DScrollBehaviorTargets::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (ensure(DragPos.WorldRay.Direction.Z * DragPos.WorldRay.Origin.Z < 0))
	{
		// Intersect a ray starting from the original position and using the new
		// ray direction. I.e., pretend the camera is not moving.
		const FVector3d OriginalCameraLocation = (FVector3d)ViewportClient->GetViewLocation();
		const double T = -OriginalCameraLocation.Z / DragPos.WorldRay.Direction.Z;
		const FVector3d DragEnd = FVector3d(
			DragPos.WorldRay.Origin.X + T * DragPos.WorldRay.Direction.X,
			DragPos.WorldRay.Origin.Y + T * DragPos.WorldRay.Direction.Y,
			0);

		// We want to make it look like we are sliding the plane such that DragStart
		// ends up on DragEnd. For that, our camera will be moving the opposite direction.
		const FVector3d CameraDisplacement = DragStart - DragEnd;
		check(CameraDisplacement.Z == 0);
		ViewportClient->SetViewLocation((FVector)(OriginalCameraLocation + CameraDisplacement));
	}
}

void FFlowDomainEd2DScrollBehaviorTargets::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
}

void FFlowDomainEd2DScrollBehaviorTargets::OnTerminateDragSequence()
{
}



// FUVEditor2DMouseWheelZoomBehaviorTarget

FFlowDomainEd2DMouseWheelZoomBehaviorTargets::FFlowDomainEd2DMouseWheelZoomBehaviorTargets(FFlowDomainEdViewportClient2D* ViewportClientIn)
	: ViewportClient(ViewportClientIn)
{
}

FInputRayHit FFlowDomainEd2DMouseWheelZoomBehaviorTargets::ShouldRespondToMouseWheel(const FInputDeviceRay& CurrentPos)
{
	// Always allowed to zoom with mouse wheel
	FInputRayHit ToReturn;
	ToReturn.bHit = true;
	return ToReturn;
}

void FFlowDomainEd2DMouseWheelZoomBehaviorTargets::OnMouseWheelScrollUp(const FInputDeviceRay& CurrentPos)
{
	PerformZoom(CurrentPos, -1);
}

void FFlowDomainEd2DMouseWheelZoomBehaviorTargets::OnMouseWheelScrollDown(const FInputDeviceRay& CurrentPos)
{
	PerformZoom(CurrentPos, 1);
}

void FFlowDomainEd2DMouseWheelZoomBehaviorTargets::PerformZoom(const FInputDeviceRay& CurrentPos, float Direction) const {
	const float Delta = 25 * Direction;
	const float OldOrthoZoom = ViewportClient->GetOrthoZoom();
	float NewOrthoZoom = OldOrthoZoom + (OldOrthoZoom / CAMERA_ZOOM_DAMPEN) * Delta;
	NewOrthoZoom = FMath::Clamp<float>(NewOrthoZoom, 1500, MAX_FLT);
	
	ViewportClient->SetOrthoZoom(NewOrthoZoom);

	UE_LOG(LogFlowInput, Log, TEXT("Ortho Zoom: %f"), NewOrthoZoom);

	const FVector PreZoomMousePos = CurrentPos.WorldRay.Origin - ViewportClient->GetViewLocation();
	const FVector PostZoomMousePos = PreZoomMousePos * (NewOrthoZoom / OldOrthoZoom);
	FVector Offset = PostZoomMousePos - PreZoomMousePos;
	Offset.Z = 0;
	ViewportClient->SetViewLocation(ViewportClient->GetViewLocation() - Offset);
}

