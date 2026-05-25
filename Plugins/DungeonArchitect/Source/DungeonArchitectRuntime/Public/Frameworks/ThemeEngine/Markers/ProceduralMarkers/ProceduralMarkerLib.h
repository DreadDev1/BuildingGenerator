//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UBoxBoundaryMarkerEmitter;

namespace DA {
	class DUNGEONARCHITECTRUNTIME_API FProceduralMarkerLib {
	public:
		static void GetBoxBoundaryEdgeTransforms(const FVector& InGridSize, const FIntVector& InMin, const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms);
		static void GetBoxBoundaryEdgeSeparatorTransforms(const FVector& InGridSize, const FIntVector& InMin, const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms);
		static void GetBoxBoundaryCornerTransforms(const FVector& InGridSize, const FIntVector& InMin, const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms);
		static void GetBoxBoundaryGroundTransforms(const FVector& InGridSize, const FIntVector& InMin, const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms);
	};
}

