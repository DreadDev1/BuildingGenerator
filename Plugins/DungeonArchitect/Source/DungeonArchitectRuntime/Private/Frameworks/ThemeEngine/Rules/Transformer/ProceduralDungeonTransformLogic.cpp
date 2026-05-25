//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Rules/Transformer/ProceduralDungeonTransformLogic.h"

#include "Core/Dungeon.h"
#include "Core/DungeonConfig.h"
#include "Core/Markers/DungeonMarker.h"
#include "Core/Markers/DungeonMarkerTags.h"

FTransform URandomTranslateProcTransformLogic::Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) {
	const FVector Translation {
		(InRandom.FRand() * 2 - 1) * MoveAlongX,
		(InRandom.FRand() * 2 - 1) * MoveAlongY,
		(InRandom.FRand() * 2 - 1) * MoveAlongZ
	};

	return FTransform(Translation);
}

FTransform URandomJitterProcTransformLogic::Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) {
	const float DistanceXY = JitterAlongXY ? JitterDistance : 0;
	const float DistanceZ = JitterAlongZ ? JitterDistance : 0;
	const FVector Translation {
		(InRandom.FRand() * 2 - 1) * DistanceXY,
		(InRandom.FRand() * 2 - 1) * DistanceXY,
		(InRandom.FRand() * 2 - 1) * DistanceZ
	};

	return FTransform(Translation);
}

FTransform URandomRotateZProcTransformLogic::Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) {
	const float Angle = InRandom.FRand() * PI * 2;
	return FTransform(FQuat(FVector::UpVector, Angle));
}

FTransform URandomRotateZ90ProcTransformLogic::Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) {
	const float Angle = (PI * 0.5f) * InRandom.RandRange(0, 3);
	return FTransform(FQuat(FVector::UpVector, Angle));
}

FTransform URandomRotateProcTransformLogic::Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) {
	FVector RotationAngles = FMath::Lerp(MinRotation, MaxRotation, InRandom.FRand());
	return FTransform(FRotator::MakeFromEuler(RotationAngles));
}

FTransform UClampToCeilingProcTransformLogic::Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) {
	if (InDungeon) {
		FVector WorldLocation = InDungeon->GetTransform().InverseTransformPositionNoScale(InMarker.Transform.GetLocation());
		if (UDungeonConfig* DungeonConfig = InDungeon->GetConfig()) {
			FDungeonConfigGenericSettings ConfigSettings = DungeonConfig->GatherGenericSettings();
			if (ConfigSettings.bIsGridBased) {
				FVector LocalSize = WorldLocation / ConfigSettings.GridSize;
				int32 CurrentElevationLevel{};
				if (FDungeonMarkerInstanceTagManagement::GetElevationFromGround(InMarker.Tags, CurrentElevationLevel)) {
					int32 MaxElevationLevel{};
					if (FDungeonMarkerInstanceTagManagement::GetCeilingElevation(InMarker.Tags, MaxElevationLevel)) {
						float OffsetZ = (MaxElevationLevel - CurrentElevationLevel) * ConfigSettings.GridSize.Z * 2;
						return FTransform(FVector(0, 0, OffsetZ));
					}
				}
			}
		}
	}
	
	return Super::Execute(InDungeon, InRandom, InMarker);
}

