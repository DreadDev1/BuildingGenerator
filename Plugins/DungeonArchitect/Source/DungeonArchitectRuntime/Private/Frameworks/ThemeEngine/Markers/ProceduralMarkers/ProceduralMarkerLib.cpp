//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerLib.h"

#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerEmitter.h"

namespace DAProcMarkers {
	void ClampToSizeMult(int SizeMultiplier, const FIntVector& InMin, const FIntVector& InMax, FIntVector& OutMin, FIntVector& OutMax) {
		const int SizeX = FMath::CeilToInt((InMax.X - InMin.X) / static_cast<float>(SizeMultiplier)) * SizeMultiplier;
		const int SizeY = FMath::CeilToInt((InMax.Y - InMin.Y) / static_cast<float>(SizeMultiplier)) * SizeMultiplier;
		OutMin.X = InMin.X;
		OutMin.Y = InMin.Y;
		OutMax.X = OutMin.X + SizeX;
		OutMax.Y = OutMin.Y + SizeY;
	}
}

void DA::FProceduralMarkerLib::GetBoxBoundaryEdgeTransforms(const FVector& InGridSize, const FIntVector& InMin, const FIntVector& InMax, int HeightZ, int SizeMultiplier,
			TArray<FTransform>& OutTransforms)
{
	SizeMultiplier = FMath::Max(1, SizeMultiplier);
	FIntVector Min, Max;
	DAProcMarkers::ClampToSizeMult(SizeMultiplier, InMin, InMax, Min, Max);

	const float Offset = 0.5f * SizeMultiplier;
	for (int X = Min.X; X < Max.X; X += SizeMultiplier) {
		FVector LocLo = FVector(X + Offset, Min.Y, HeightZ) * InGridSize;
		FVector LocHi = FVector(X + Offset, Max.Y, HeightZ) * InGridSize;

		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI), LocLo));
		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, 0), LocHi));
	}

	for (int Y = Min.Y; Y < Max.Y; Y += SizeMultiplier) {
		FVector LocLo = FVector(Min.X, Y + Offset, HeightZ) * InGridSize;
		FVector LocHi = FVector(Max.X, Y + Offset, HeightZ) * InGridSize;

		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI * 0.5), LocLo));
		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, -PI * 0.5), LocHi));
	}
}

void DA::FProceduralMarkerLib::GetBoxBoundaryEdgeSeparatorTransforms(const FVector& InGridSize,
		const FIntVector& InMin, const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms)
{
	SizeMultiplier = FMath::Max(1, SizeMultiplier);
	FIntVector Min, Max;
	DAProcMarkers::ClampToSizeMult(SizeMultiplier, InMin, InMax, Min, Max);
	
	for (int X = Min.X; X < Max.X; X += SizeMultiplier) {
		FVector LocLo = FVector(X, Min.Y, HeightZ) * InGridSize;
		FVector LocHi = FVector(X + SizeMultiplier, Max.Y, HeightZ) * InGridSize;

		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI), LocLo));
		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, 0), LocHi));
	}

	for (int Y = Min.Y; Y < Max.Y; Y += SizeMultiplier) {
		FVector LocLo = FVector(Min.X, Y, HeightZ) * InGridSize;
		FVector LocHi = FVector(Max.X, Y + SizeMultiplier, HeightZ) * InGridSize;

		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI * 0.5), LocLo));
		OutTransforms.Add(FTransform(FQuat(FVector::UpVector, -PI * 0.5), LocHi));
	}
}

void DA::FProceduralMarkerLib::GetBoxBoundaryCornerTransforms(const FVector& InGridSize, const FIntVector& InMin,
			const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms) {

	SizeMultiplier = FMath::Max(1, SizeMultiplier);
	FIntVector Min, Max;
	DAProcMarkers::ClampToSizeMult(SizeMultiplier, InMin, InMax, Min, Max);
	
	const FVector Loc0 = FVector(Min.X, Min.Y, HeightZ) * InGridSize;
	const FVector Loc1 = FVector(Max.X, Min.Y, HeightZ) * InGridSize;
	const FVector Loc2 = FVector(Max.X, Max.Y, HeightZ) * InGridSize;
	const FVector Loc3 = FVector(Min.X, Max.Y, HeightZ) * InGridSize;

	OutTransforms.Add(FTransform(FQuat(FVector::UpVector, 0), Loc0));
	OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI * 0.5), Loc1));
	OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI), Loc2));
	OutTransforms.Add(FTransform(FQuat(FVector::UpVector, PI * 1.5), Loc3));
}

void DA::FProceduralMarkerLib::GetBoxBoundaryGroundTransforms(const FVector& InGridSize, const FIntVector& InMin,
		const FIntVector& InMax, int HeightZ, int SizeMultiplier, TArray<FTransform>& OutTransforms) {
	
	SizeMultiplier = FMath::Max(1, SizeMultiplier);
	FIntVector Min, Max;
	DAProcMarkers::ClampToSizeMult(SizeMultiplier, InMin, InMax, Min, Max);

	const float Offset = 0.5f * SizeMultiplier;
	for (int X = Min.X; X < Max.X; X += SizeMultiplier) {
		for (int Y = Min.Y; Y < Max.Y; Y += SizeMultiplier) {
			FVector Location = FVector(X + Offset, Y + Offset, HeightZ) * InGridSize;
			OutTransforms.Add(FTransform(FQuat(FVector::UpVector, 0), Location));
		}
	}
}

