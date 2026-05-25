//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class FPrimitiveDrawInterface;

class DUNGEONARCHITECTRUNTIME_API FDungeonRenderPDIUtils {
public:
	static void RenderVisualPoint(FPrimitiveDrawInterface* PDI, const FVector& Location, float CircleRadius,
			FLinearColor DrawColor = FLinearColor::Red, int32 CircleSegments = 16, float PointThickness = 10.0f,
			ESceneDepthPriorityGroup DepthPriorityGroup = SDPG_Foreground);
};

class DUNGEONARCHITECTRUNTIME_API FDungeonRenderUtils {
public:
	static FVector2f CalculateTriplanarUV(const FVector& WorldPosition, const FVector3f& Normal, float Scale = 100.0f);
};

