//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonRenderUtils.h"

#include "SceneManagement.h"

void FDungeonRenderPDIUtils::RenderVisualPoint(FPrimitiveDrawInterface* PDI, const FVector& Location,
		float CircleRadius, FLinearColor DrawColor, int32 CircleSegments, float PointThickness,
		ESceneDepthPriorityGroup DepthPriorityGroup)
{
}

FVector2f FDungeonRenderUtils::CalculateTriplanarUV(const FVector& WorldPosition, const FVector3f& Normal, float Scale)
{
	const float TriplanarScale = 1.0f / Scale;
	const FVector AbsNormal = FVector(FMath::Abs(Normal.X), FMath::Abs(Normal.Y), FMath::Abs(Normal.Z));
	
	FVector2f UVX = FVector2f(WorldPosition.Y, WorldPosition.Z) * TriplanarScale;
	FVector2f UVY = FVector2f(WorldPosition.X, WorldPosition.Z) * TriplanarScale;
	FVector2f UVZ = FVector2f(WorldPosition.X, WorldPosition.Y) * TriplanarScale;
	
	if (AbsNormal.X >= AbsNormal.Y && AbsNormal.X >= AbsNormal.Z) {
		return UVX;
	} else if (AbsNormal.Y >= AbsNormal.Z) {
		return UVY;
	} else {
		return UVZ;
	}
}

