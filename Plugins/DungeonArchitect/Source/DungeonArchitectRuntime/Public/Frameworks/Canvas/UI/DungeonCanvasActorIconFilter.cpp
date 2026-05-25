//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/UI/DungeonCanvasActorIconFilter.h"

#include "Frameworks/Canvas/DungeonCanvasStructs.h"
#include "Frameworks/Canvas/DungeonCanvasViewport.h"

#include "Curves/CurveFloat.h"

void UDungeonCanvasCircularFrameActorIconFilter::ApplyFilter(const FDungeonCanvasViewportTransform& View, const TMap<FName,
	FDungeonCanvasOverlayIcon>& OverlayIconMap, const TArray<FName>& ItemTags, TObjectPtr<UObject>& ResourceObject,
	FVector2D& CanvasSize, FVector2D& CanvasLocation, float& ScreenSize, FLinearColor& Tint, float& Rotation)
{
	if (const FDungeonCanvasOverlayIcon* EdgeIconPtr = OverlayIconMap.Find(BoundaryEdgeIconName)) {
		bool bFoundTag = false;
		for (const FName& RequiredTag : TrackedIconTags) {
			if (ItemTags.Contains(RequiredTag)) {
				bFoundTag = true;
				break;
			}
		} 
		if (!bFoundTag) {
			return;
		}
		
		const FVector2D CanvasCenter = CanvasSize * 0.5f;
		const float TargetRadius = CanvasCenter.X * FMath::Clamp(BoundaryRadius, 0, 1);
		const float IconDistanceFromCenter = (CanvasLocation - CanvasCenter).Size();
		const float MaxEdgeIconDisplayDistance = CanvasSize.X * BoundaryEdgeIconMaxDisplayDistance;
		if (IconDistanceFromCenter > TargetRadius && IconDistanceFromCenter < TargetRadius + MaxEdgeIconDisplayDistance) {
			// Clamp to the circle and change the icon
			const FVector2D DirectionFromCenter = (CanvasLocation - CanvasCenter).GetSafeNormal();
			CanvasLocation = CanvasCenter + DirectionFromCenter * TargetRadius;

			ResourceObject = EdgeIconPtr->ResourceObject;
			const float SourceTintAlpha = Tint.A;
			Tint = EdgeIconPtr->Tint;
			Tint.A *= SourceTintAlpha;
			
			Rotation = EdgeIconPtr->RotationOffset;
			const float DirectionAngleRad = FMath::Atan2(DirectionFromCenter.Y, DirectionFromCenter.X);
			Rotation += FMath::RadiansToDegrees(DirectionAngleRad);

			if (EdgeIconPtr->ScreenSizeType == EDungeonCanvasIconCoordinateSystem::Pixels) {
				ScreenSize = EdgeIconPtr->ScreenSize;
			}
			else if (EdgeIconPtr->ScreenSizeType == EDungeonCanvasIconCoordinateSystem::WorldCoordinates) {
				const FVector WorldToCanvasScale = View.GetWorldToCanvas().GetScale3D() * EdgeIconPtr->ScreenSize;
				ScreenSize = FMath::Max(WorldToCanvasScale.X, WorldToCanvasScale.Y);
			}

			if (bEnableScaleBeyondBoundary) {
				const float Ratio = FMath::Clamp((IconDistanceFromCenter - TargetRadius) / MaxEdgeIconDisplayDistance, 0, 1);
				float Scale = 1 - Ratio;
				if (const UCurveFloat* Curve = ScaleCurve.LoadSynchronous()) {
					Scale = Curve->GetFloatValue(Ratio);
				}
				ScreenSize *= Scale;
			}
		}
	}
}


