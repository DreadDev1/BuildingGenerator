//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Cameras/DungeonCanvasCameraInteractive.h"

#include "Frameworks/Canvas/DungeonCanvas.h"

void UDungeonCanvasCameraInteractive::Initialize_Implementation(UDungeonCanvasComponent* DungeonCanvas) {
	Super::Initialize_Implementation(DungeonCanvas);

	if (DungeonCanvas) {
		SetCanvasTransform(DungeonCanvas->GetFullDungeonViewTransform());

		// Capture the base world size for zoom calculations
		BaseWorldSize = ViewportTransform.GetLocalToWorld().GetScale3D();
		CurrentZoom = 1.0f;
		TargetZoom = 1.0f;
	}
}

void UDungeonCanvasCameraInteractive::Update_Implementation(UDungeonCanvasComponent* DungeonCanvas, float DeltaSeconds) {
	Super::Update_Implementation(DungeonCanvas, DeltaSeconds);

	// Smooth zoom interpolation
	if (!FMath::IsNearlyEqual(CurrentZoom, TargetZoom, KINDA_SMALL_NUMBER)) {
		if (ZoomInterpSpeed <= 0.0f) {
			// Instant zoom
			CurrentZoom = TargetZoom;
		}
		else {
			// Smooth interpolation
			CurrentZoom = FMath::FInterpTo(CurrentZoom, TargetZoom, DeltaSeconds, ZoomInterpSpeed);
		}

		// Apply the interpolated zoom
		FTransform LocalToWorld = ViewportTransform.GetLocalToWorld();
		LocalToWorld.SetScale3D(BaseWorldSize * CurrentZoom);
		ViewportTransform.SetLocalToWorld(LocalToWorld);
	}
}

bool UDungeonCanvasCameraInteractive::OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) {
		bIsPanning = true;
		// Use local coordinates for proper 1:1 panning regardless of DPI/scaling
		LastMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		return true;
	}
	return Super::OnMouseButtonDown(InGeometry, InMouseEvent);
}

bool UDungeonCanvasCameraInteractive::OnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) {
		bIsPanning = false;
		return true;
	}
	return Super::OnMouseButtonUp(InGeometry, InMouseEvent);
}

bool UDungeonCanvasCameraInteractive::OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (bIsPanning) {
		// Use local coordinates for proper 1:1 panning regardless of DPI/scaling
		const FVector2D CurrentMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D Delta = CurrentMousePosition - LastMousePosition;
		LastMousePosition = CurrentMousePosition;

		// Convert local widget delta to world delta
		FTransform LocalToWorld = ViewportTransform.GetLocalToWorld();
		const FVector WorldSize = LocalToWorld.GetScale3D();
		const FVector2D WidgetSize = InGeometry.GetLocalSize();

		// Calculate world units per local pixel (1:1 correspondence)
		const float WorldUnitsPerPixelX = WorldSize.X / WidgetSize.X;
		const float WorldUnitsPerPixelY = WorldSize.Y / WidgetSize.Y;

		// Apply pan (invert delta so dragging moves the view in the expected direction)
		FVector Location = LocalToWorld.GetLocation();
		Location.X -= Delta.X * WorldUnitsPerPixelX * PanSpeed;
		Location.Y -= Delta.Y * WorldUnitsPerPixelY * PanSpeed;
		LocalToWorld.SetLocation(Location);
		ViewportTransform.SetLocalToWorld(LocalToWorld);

		return true;
	}
	return Super::OnMouseMove(InGeometry, InMouseEvent);
}

bool UDungeonCanvasCameraInteractive::OnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	const float WheelDelta = InMouseEvent.GetWheelDelta();
	if (FMath::Abs(WheelDelta) > KINDA_SMALL_NUMBER) {
		// Use logarithmic zoom for consistent feel across zoom levels
		// Scroll up (positive delta) = zoom in (smaller TargetZoom)
		// Scroll down (negative delta) = zoom out (larger TargetZoom, capped at 1.0)
		const float LogZoomDelta = -WheelDelta * ZoomSpeed * 0.1f;

		// Clamp between MaxZoomIn (e.g., 0.1) and 1.0 (full dungeon view, can't zoom out past initial)
		TargetZoom = FMath::Clamp(TargetZoom * FMath::Exp(LogZoomDelta), MaxZoomIn, 1.0f);

		return true;
	}
	return Super::OnMouseWheel(InGeometry, InMouseEvent);
}

void UDungeonCanvasCameraInteractive::OnMouseLeave(const FPointerEvent& InMouseEvent) {
	Super::OnMouseLeave(InMouseEvent);
	bIsPanning = false;
}


