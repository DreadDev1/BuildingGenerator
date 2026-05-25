//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Cameras/DungeonCanvasCamera.h"


void UDungeonCanvasCamera::Initialize_Implementation(UDungeonCanvasComponent* DungeonCanvas) {
	
}

void UDungeonCanvasCamera::Update_Implementation(UDungeonCanvasComponent* DungeonCanvas, float DeltaSeconds) {
	
}

void UDungeonCanvasCamera::SetCameraLocation(FVector WorldLocation) {
	FTransform LocalToWorld = ViewportTransform.GetLocalToWorld();
	LocalToWorld.SetLocation(WorldLocation);
	ViewportTransform.SetLocalToWorld(LocalToWorld);
}

void UDungeonCanvasCamera::SetCameraRotation(FRotator Rotation) {
	CameraRotation = Rotation;
}

void UDungeonCanvasCamera::SetCameraSize(FVector WorldSize) {
	FTransform LocalToWorld = ViewportTransform.GetLocalToWorld();
	LocalToWorld.SetScale3D(WorldSize);
	ViewportTransform.SetLocalToWorld(LocalToWorld);	
}

void UDungeonCanvasCamera::SetCameraWorldTransform(FTransform WorldTransform) {
	ViewportTransform.SetLocalToWorld(WorldTransform); 
}

void UDungeonCanvasCamera::SetCanvasTransform(const FDungeonCanvasViewportTransform& Transform) {
	ViewportTransform = Transform;
}

bool UDungeonCanvasCamera::OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	return false;
}

bool UDungeonCanvasCamera::OnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	return false;
}

bool UDungeonCanvasCamera::OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	return false;
}

bool UDungeonCanvasCamera::OnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	return false;
}

void UDungeonCanvasCamera::OnMouseLeave(const FPointerEvent& InMouseEvent) {
}

FDungeonCanvasViewportTransform UDungeonCanvasCamera::GetCameraTransform(const FVector2D& InCanvasSize) const {
	FDungeonCanvasViewportTransform FrameViewTransform = ViewportTransform;
	
	FrameViewTransform.FocusOnCanvas(InCanvasSize.X, InCanvasSize.Y);
	FTransform LocalToCanvas = FrameViewTransform.GetLocalToCanvas();
	FQuat Rotation = LocalToCanvas.GetRotation();
	Rotation = Rotation * FQuat(FVector::UpVector, FMath::DegreesToRadians(-BaseCanvasRotation));
	
	if (bRotateToView) {
		Rotation = CameraRotation.GetInverse().Quaternion() * Rotation;
	}
	
	LocalToCanvas.SetRotation(Rotation);
	FrameViewTransform.SetLocalToCanvas(LocalToCanvas);
	return FrameViewTransform;
}

