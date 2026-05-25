//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Data/DungeonForgeSpatialData.h"

#include "Frameworks/Forge/Common/DungeonForgePoint.h"
#include "Frameworks/Forge/Elements/DungeonForgeProjectionParams.h"

UDungeonForgeSpatialData::UDungeonForgeSpatialData(const FObjectInitializer& ObjectInitializer)\
	: Super(ObjectInitializer)
{
	
}

UDungeonForgeSpatialData* UDungeonForgeSpatialData::DuplicateData() const {
	UDungeonForgeSpatialData* NewSpatialData = CopyInternal();
	check(NewSpatialData);
	return NewSpatialData;
}

void UDungeonForgeSpatialData::SamplePoints(const TArrayView<const TPair<FTransform, FBox>>& InSamples, const TArrayView<FDungeonForgePoint>& OutPoints) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgeSpatialData::SamplePoints);
	check(InSamples.Num() == OutPoints.Num());
	for(int Index = 0; Index < InSamples.Num(); ++Index)
	{
		const TPair<FTransform, FBox>& Sample = InSamples[Index];
		FDungeonForgePoint& OutPoint = OutPoints[Index];

		if (!SamplePoint(Sample.Key, Sample.Value, OutPoint))
		{
			OutPoint.Density = 0;
		}
	}
}

bool UDungeonForgeSpatialData::K2_SamplePoint(const FTransform& InTransform, const FBox& InBounds, FDungeonForgePoint& OutPoint) const {
	return SamplePoint(InTransform, InBounds, OutPoint);
}

bool UDungeonForgeSpatialData::ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint) const {
	// Fallback implementation - calls SamplePoint because SamplePoint was being used for projection previously.
	
	// TODO This is a crutch until we implement ProjectPoint everywhere
	const bool bResult = SamplePoint(InTransform, InBounds, OutPoint);

	// Respect the projection params that we can at this point given our available data (InTransform)

	if (!InParams.bProjectPositions)
	{
		OutPoint.Transform.SetLocation(InTransform.GetLocation());
	}

	if (!InParams.bProjectRotations)
	{
		OutPoint.Transform.SetRotation(InTransform.GetRotation());
	}

	if (!InParams.bProjectScales)
	{
		OutPoint.Transform.SetScale3D(InTransform.GetScale3D());
	}

	return bResult;
}

void UDungeonForgeSpatialData::ProjectPoints(const TArrayView<const TPair<FTransform, FBox>>& InSamples, const FDungeonForgeProjectionParams& InParams, const TArrayView<FDungeonForgePoint>& OutPoints) const {
	TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgeSpatialData::ProjectPoints);
	check(InSamples.Num() == OutPoints.Num());
	for (int Index = 0; Index < InSamples.Num(); ++Index)
	{
		const TPair<FTransform, FBox>& Sample = InSamples[Index];
		FDungeonForgePoint& OutPoint = OutPoints[Index];

		if (!ProjectPoint(Sample.Key, Sample.Value, InParams, OutPoint))
		{
			OutPoint.Density = 0;
		}
	}
}

bool UDungeonForgeSpatialData::K2_ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint) const {
	return ProjectPoint(InTransform, InBounds, InParams, OutPoint);
}

