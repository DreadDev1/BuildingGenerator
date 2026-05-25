//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasViewport.h"


void FDungeonCanvasViewportTransform::SetLocalToWorld(const FTransform& InTransform) {
	LocalToWorld = InTransform;
	UpdateWorldAspectRatio();
	RecalculateTransforms();
	bFocusInitialized = true;		// TODO: get rid of this
}

void FDungeonCanvasViewportTransform::SetLocalToCanvas(const FTransform& InTransform) {
	LocalToCanvas = InTransform;
	UpdateWorldAspectRatio();
	RecalculateTransforms();
}

void FDungeonCanvasViewportTransform::FocusOnCanvas(float Width, float Height) {
	const FVector Size(Width, Height, 0);
	SetLocalToCanvas(FTransform(GetLocalToCanvas().GetRotation(), Size * 0.5, Size));
}

void FDungeonCanvasViewportTransform::UpdateWorldAspectRatio() {
	const FVector Size = LocalToCanvas.GetScale3D();
	FVector WorldSize = LocalToWorld.GetScale3D();
	const float CanvasAspectRatio = Size.X / Size.Y;
	const float WorldAspectRatio = WorldSize.X / WorldSize.Y;

	if (CanvasAspectRatio != WorldAspectRatio) {
		if (CanvasAspectRatio > WorldAspectRatio) {
			// Adjust width to match aspect ratio
			WorldSize.X = WorldSize.Y * CanvasAspectRatio;
		} else {
			// Adjust height to match aspect ratio
			WorldSize.Y = WorldSize.X / CanvasAspectRatio;
		}
	}
	LocalToWorld.SetScale3D(WorldSize);
}

FVector2D FDungeonCanvasViewportTransform::WorldToCanvasLocation(const FVector& InWorldLocation) const {
	return FVector2D(WorldToCanvas.TransformPosition(InWorldLocation));
}

FVector FDungeonCanvasViewportTransform::CanvasToWorldLocation(const FVector2D& InCanvasLocation) const {
	return CanvasToWorld.TransformPosition(FVector(InCanvasLocation, 0));
}

FVector2D FDungeonCanvasViewportTransform::CanvasToUV(const FVector2D& InCanvasLocation) const {
	const FVector2D CanvasSize = FVector2D(LocalToCanvas.GetScale3D());
	return InCanvasLocation / CanvasSize;
}

FVector2D FDungeonCanvasViewportTransform::UVToCanvas(const FVector2D& InUV) const {
	const FVector2D CanvasSize = FVector2D(LocalToCanvas.GetScale3D());
	return InUV * CanvasSize;
}

void FDungeonCanvasViewportTransform::RecalculateTransforms() {
	const FTransform WorldToLocal = LocalToWorld.Inverse();
	WorldToCanvas = WorldToLocal * LocalToCanvas;
	CanvasToWorld = WorldToCanvas.Inverse();
}

