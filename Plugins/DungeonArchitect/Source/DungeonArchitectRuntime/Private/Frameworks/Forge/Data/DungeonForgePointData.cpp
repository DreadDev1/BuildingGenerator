//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Data/DungeonForgePointData.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Elements/DungeonForgeProjectionParams.h"

namespace DungeonForgePointHelpers
{
	bool GetDistanceRatios(const FDungeonForgePoint& InPoint, const FVector& InPosition, FVector& OutRatios)
	{
		FVector LocalPosition = InPoint.Transform.InverseTransformPosition(InPosition);
		LocalPosition -= (InPoint.BoundsMax + InPoint.BoundsMin) / 2;
		LocalPosition /= InPoint.GetExtents();

		constexpr float Steepness = 0;
		// ]-2+s, 2-s] is the valid range of values
		const FVector::FReal LowerBound = Steepness - 2;
		const FVector::FReal HigherBound = 2 - Steepness;

		if (LocalPosition.X <= LowerBound || LocalPosition.X > HigherBound ||
			LocalPosition.Y <= LowerBound || LocalPosition.Y > HigherBound ||
			LocalPosition.Z <= LowerBound || LocalPosition.Z > HigherBound)
		{
			return false;
		}

		// [-s, +s] is the range where the density is 1 on that axis
		const FVector::FReal XDist = FMath::Max(0, FMath::Abs(LocalPosition.X) - Steepness);
		const FVector::FReal YDist = FMath::Max(0, FMath::Abs(LocalPosition.Y) - Steepness);
		const FVector::FReal ZDist = FMath::Max(0, FMath::Abs(LocalPosition.Z) - Steepness);

		const FVector::FReal DistanceScale = FMath::Max(2 - 2 * Steepness, KINDA_SMALL_NUMBER);

		OutRatios.X = XDist / DistanceScale;
		OutRatios.Y = YDist / DistanceScale;
		OutRatios.Z = ZDist / DistanceScale;
		return true;
	}

	FVector::FReal ManhattanDensity(const FDungeonForgePoint& InPoint, const FVector& InPosition)
	{
		FVector Ratios;
		if (GetDistanceRatios(InPoint, InPosition, Ratios))
		{
			return InPoint.Density * (1 - Ratios.X) * (1 - Ratios.Y) * (1 - Ratios.Z);
		}
		else
		{
			return 0;
		}
	}

	FVector::FReal InverseEuclidianDistance(const FDungeonForgePoint& InPoint, const FVector& InPosition)
	{
		FVector Ratios;
		if (GetDistanceRatios(InPoint, InPosition, Ratios))
		{
			return 1.0 - Ratios.Length();
		}
		else
		{
			return 0;
		}
	}

	/** Computes reasonable overlap ratio for point, 1d, 2d and volume overlaps, to be used as weights.
	* Note that this assumes that either data set is homogeneous in its points dimension (either 0d, 1d, 2d, 3d)
	* Otherwise there will be some artifacts from our assumption here (namely using a 1.0 value for the additional coordinates).
	*/
	FVector::FReal ComputeOverlapRatio(const FBox& Numerator, const FBox& Denominator)
	{
		const FVector NumeratorExtent = Numerator.GetExtent();
		const FVector DenominatorExtent = Denominator.GetExtent();

		return (FVector::FReal)((DenominatorExtent.X > 0 ? NumeratorExtent.X / DenominatorExtent.X : 1.0) *
			(DenominatorExtent.Y > 0 ? NumeratorExtent.Y / DenominatorExtent.Y : 1.0) *
			(DenominatorExtent.Z > 0 ? NumeratorExtent.Z / DenominatorExtent.Z : 1.0));
	}

	FVector::FReal VolumeOverlap(const FDungeonForgePoint& InPoint, const FBox& InBounds, const FMatrix& InInverseTransform)
	{
		// This is similar in idea to SAT considering we have two boxes - since we will test all 6 axes.
		// However, there is some uncertainty due to rotation, and using the overlap value as-is is an overestimation, which might not be critical in this case
		// TODO: investigate if we should do a 8-pt test instead (would be more precise, but significantly more costly).
		// Implementation note: we are using FMatrix here because we want to support non-uniform scales
		const FBox PointBounds = InPoint.GetLocalBounds();

		FMatrix PointTransformToInTransform = InPoint.Transform.ToMatrixWithScale() * InInverseTransform;
		const FBox PointBoundsTransformed = PointBounds.TransformBy(PointTransformToInTransform);

		const FBox FirstOverlap = InBounds.Overlap(PointBoundsTransformed);
		if (!FirstOverlap.IsValid)
		{
			return 0;
		}

		FMatrix InTransformToPointTransform = PointTransformToInTransform.Inverse();
		const FBox InBoundsTransformed = InBounds.TransformBy(InTransformToPointTransform);

		const FBox SecondOverlap = InBoundsTransformed.Overlap(PointBounds);
		if (!SecondOverlap.IsValid)
		{
			return 0;
		}

		return FMath::Min(ComputeOverlapRatio(FirstOverlap, InBounds), ComputeOverlapRatio(SecondOverlap, InBoundsTransformed));
	}

	/** Helper function for additive blending of quaternions (copied from ControlRig) */
	FQuat AddQuatWithWeight(const FQuat& Q, const FQuat& V, float Weight)
	{
		FQuat BlendQuat = V * Weight;

		if ((Q | BlendQuat) >= 0.0f)
			return Q + BlendQuat;
		else
			return Q - BlendQuat;
	}

	void Lerp(const FDungeonForgePoint& A, const FDungeonForgePoint& B, float Ratio, FDungeonForgePoint& OutPoint)
	{
		check(Ratio >= 0 && Ratio <= 1.0f);
		// TODO: this might be incorrect. See UKismetMathLibrary::TLerp instead
		OutPoint.Transform = FTransform(
			FMath::Lerp(A.Transform.GetRotation(), B.Transform.GetRotation(), Ratio),
			FMath::Lerp(A.Transform.GetLocation(), B.Transform.GetLocation(), Ratio),
			FMath::Lerp(A.Transform.GetScale3D(), B.Transform.GetScale3D(), Ratio));
		OutPoint.Density = FMath::Lerp(A.Density, B.Density, Ratio);
		OutPoint.BoundsMin = FMath::Lerp(A.BoundsMin, B.BoundsMin, Ratio);
		OutPoint.BoundsMax = FMath::Lerp(A.BoundsMax, B.BoundsMax, Ratio);
		OutPoint.Color = FMath::Lerp(A.Color, B.Color, Ratio);
	}

	void BilerpWithSnapping(const FDungeonForgePoint& X0Y0, const FDungeonForgePoint& X1Y0, const FDungeonForgePoint& X0Y1, const FDungeonForgePoint& X1Y1, FDungeonForgePoint& OutPoint, float XFactor, float YFactor)
	{
		const bool bIsOnLeftEdge = (XFactor < KINDA_SMALL_NUMBER);
		const bool bIsOnRightEdge = (XFactor > 1.0f - KINDA_SMALL_NUMBER);
		const bool bIsOnTopEdge = (YFactor < KINDA_SMALL_NUMBER);
		const bool bIsOnBottomEdge = (YFactor > 1.0f - KINDA_SMALL_NUMBER);

		auto CopyPoint = [&OutPoint](const FDungeonForgePoint& PointToCopy)
		{
			OutPoint = PointToCopy;
		};

		if (bIsOnLeftEdge || bIsOnRightEdge || bIsOnTopEdge || bIsOnBottomEdge)
		{
			if (bIsOnLeftEdge)
			{
				if (bIsOnTopEdge)
				{
					CopyPoint(X0Y0);
				}
				else if (bIsOnBottomEdge)
				{
					CopyPoint(X0Y1);
				}
				else
				{
					Lerp(X0Y0, X0Y1, YFactor, OutPoint);
				}
			}
			else if (bIsOnRightEdge)
			{
				if (bIsOnTopEdge)
				{
					CopyPoint(X1Y0);
				}
				else if (bIsOnBottomEdge)
				{
					CopyPoint(X1Y1);
				}
				else
				{
					Lerp(X1Y0, X1Y1, YFactor, OutPoint);
				}
			}
			else if (bIsOnTopEdge)
			{
				Lerp(X0Y0, X1Y0, XFactor, OutPoint);
			}
			else // bIsOnBottomEdge
			{
				Lerp(X0Y1, X1Y1, XFactor, OutPoint);
			}
		}
		else
		{
			Bilerp(X0Y0, X1Y0, X0Y1, X1Y1, OutPoint, XFactor, YFactor);
		}
	}

	void Bilerp(const FDungeonForgePoint& X0Y0, const FDungeonForgePoint& X1Y0, const FDungeonForgePoint& X0Y1, const FDungeonForgePoint& X1Y1, FDungeonForgePoint& OutPoint, float XFactor, float YFactor)
	{
		// Interpolate X0Y0-X1Y0 and X0Y1-X1Y1 using XFactor
		FDungeonForgePoint Y0Lerp;
		FDungeonForgePoint Y1Lerp;

		Lerp(X0Y0, X1Y0, XFactor, Y0Lerp);
		Lerp(X0Y1, X1Y1, XFactor, Y1Lerp);
		// Interpolate between the two points using YFactor
		Lerp(Y0Lerp, Y1Lerp, YFactor, OutPoint);
	}
}

FDungeonForgePointRef::FDungeonForgePointRef(const FDungeonForgePoint& InPoint) {
	Point = &InPoint;
	Bounds = InPoint.GetWorldBounds();
}

FDungeonForgePointRef::FDungeonForgePointRef(const FDungeonForgePoint& InPoint, const FBox& InOverrideBounds) {
	Point = &InPoint;
	Bounds = FBoxSphereBounds(InOverrideBounds.TransformBy(InPoint.Transform));
}

void UDungeonForgePointData::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) {
	Super::GetResourceSizeEx(CumulativeResourceSize);
	
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Points.GetAllocatedSize() + Octree.GetSizeBytes() + sizeof(Bounds));
}

FBox UDungeonForgePointData::GetBounds() const {
	if (bBoundsAreDirty) {
		RecomputeBounds();
	}

	return Bounds;
}

bool UDungeonForgePointData::SamplePoint(const FTransform& InTransform, const FBox& InBounds, FDungeonForgePoint& OutPoint) const {
	// Run a projection but don't change the point transform. There is a large overlap in code/functionality so this shares one code path.
	FDungeonForgeProjectionParams Params{};
	Params.bProjectPositions = Params.bProjectRotations = Params.bProjectScales = false;
	Params.ColorBlendMode = EDungeonForgeProjectionColorBlendMode::SourceValue;

	// The ProjectPoint implementation in this class returns true if the query point is overlapping the point data, which is what SamplePoint should return, so forward the return value.
	return ProjectPoint(InTransform, InBounds, Params, OutPoint);
}

bool UDungeonForgePointData::ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint) const {
	return ProjectPoint(InTransform, InBounds, InParams, OutPoint, true);
}

UDungeonForgeSpatialData* UDungeonForgePointData::CopyInternal() const {
	UDungeonForgePointData* NewPointData = NewObject<UDungeonForgePointData>();
	NewPointData->GetMutablePoints() = GetPoints();

	return NewPointData;
}

FDungeonForgePoint UDungeonForgePointData::GetPoint(int32 Index) const {
	if (Points.IsValidIndex(Index))
	{
		return Points[Index];
	}
	else
	{
		UE_LOG(LogDungeonForge, Error, TEXT("Invalid index in GetPoint call"));
		return FDungeonForgePoint();
	}
}

void UDungeonForgePointData::SetPoints(const TArray<FDungeonForgePoint>& InPoints) {
	GetMutablePoints() = InPoints;
}

void UDungeonForgePointData::CopyPointsFrom(const UDungeonForgePointData* InData, const TArray<int>& InDataIndices) {
	TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgePointData::CopyPointsFrom);
	check(InData);
	Points.SetNum(InDataIndices.Num());

	// TODO: parallel-for this?
	for (int PointIndex = 0; PointIndex < InDataIndices.Num(); ++PointIndex)
	{
		Points[PointIndex] = InData->Points[InDataIndices[PointIndex]];
	}

	bBoundsAreDirty = true;
	bOctreeIsDirty = true;
}

TArray<FDungeonForgePoint>& UDungeonForgePointData::GetMutablePoints() {
	bOctreeIsDirty = true;
	bBoundsAreDirty = true;
	return Points;
}

const UDungeonForgePointData::PointOctree& UDungeonForgePointData::GetOctree() const {
	if (bOctreeIsDirty) {
		RebuildOctree();
	}

	return Octree;
}

bool UDungeonForgePointData::ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint, bool bUseBounds) const {
	//TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgePointData::SamplePoint);
	if (bOctreeIsDirty)
	{
		RebuildOctree();
	}

	TArray<TPair<const FDungeonForgePoint*, FVector::FReal>, TInlineAllocator<4>> Contributions;
	const bool bSampleInVolume = (InBounds.GetExtent() != FVector::ZeroVector);

	if (!bSampleInVolume)
	{
		const FVector InPosition = InTransform.GetLocation();
		Octree.FindElementsWithBoundsTest(FBoxCenterAndExtent(InPosition, FVector::Zero()), [&InPosition, &Contributions](const FDungeonForgePointRef& InPointRef) 
		{
			Contributions.Emplace(InPointRef.Point, DungeonForgePointHelpers::InverseEuclidianDistance(*InPointRef.Point, InPosition));
		});
	}
	else
	{
		FBox TransformedBounds = InBounds.TransformBy(InTransform);
		FMatrix InTransformInverseMatrix = InTransform.ToMatrixWithScale().Inverse();

		Octree.FindElementsWithBoundsTest(FBoxCenterAndExtent(TransformedBounds.GetCenter(), TransformedBounds.GetExtent()), [bUseBounds, &InBounds, &InTransformInverseMatrix, &Contributions](const FDungeonForgePointRef& InPointRef) 
		{
			const FVector::FReal Contribution = bUseBounds ? DungeonForgePointHelpers::VolumeOverlap(*InPointRef.Point, InBounds, InTransformInverseMatrix) : 1.0;
			if (Contribution > 0)
			{
				Contributions.Emplace(InPointRef.Point, Contribution);
			}
		});
	}

	FVector::FReal SumContributions = 0;
	FVector::FReal MaxContribution = 0;
	const FDungeonForgePoint* MaxContributor = nullptr;

	for (const TPair<const FDungeonForgePoint*, FVector::FReal>& Contribution : Contributions)
	{
		SumContributions += Contribution.Value;

		if (Contribution.Value > MaxContribution)
		{
			MaxContribution = Contribution.Value;
			MaxContributor = Contribution.Key;
		}
	}

	if (SumContributions <= 0)
	{
		return false;
	}

	// Rationale: 
	// When doing volume-to-volume intersection, we want the final density to reflect the amount of overlap
	// if any - hence the volume overlap computation before.
	// But, considering that some points may/will overlap (incl. due to steepness), we want to make sure we do not
	// sum up to more than the total volume. 
	// Note that this might create some artifacts on the edges in some instances, but we will revisit this once we have a
	// better and sufficiently efficient solution.
	const FVector::FReal DensityNormalizationFactor = ((SumContributions > 1.0) ? (1.0 / SumContributions) : 1.0);

	TArray<TPair<const FDungeonForgePoint*, float>, TInlineAllocator<4>> ContributionsForMetadata;

	// Computed weighted average of spatial properties
	FVector WeightedPosition = FVector::ZeroVector;
	FQuat WeightedQuat = FQuat(0.0, 0.0, 0.0, 0.0);
	FVector WeightedScale = FVector::ZeroVector;
	FVector::FReal WeightedDensity = 0;
	FVector WeightedBoundsMin = FVector::ZeroVector;
	FVector WeightedBoundsMax = FVector::ZeroVector;
	FVector4 WeightedColor = FVector4::Zero();

	for (const TPair<const FDungeonForgePoint*, FVector::FReal> Contribution : Contributions)
	{
		const FDungeonForgePoint& SourcePoint = *Contribution.Key;
		const FVector::FReal Weight = Contribution.Value / SumContributions;

		WeightedPosition += SourcePoint.Transform.GetLocation() * Weight;
		WeightedQuat = DungeonForgePointHelpers::AddQuatWithWeight(WeightedQuat, SourcePoint.Transform.GetRotation(), Weight);
		WeightedScale += SourcePoint.Transform.GetScale3D() * Weight;

		if (!bSampleInVolume)
		{
			WeightedDensity += DungeonForgePointHelpers::ManhattanDensity(SourcePoint, InTransform.GetLocation());
		}
		else
		{
			WeightedDensity += SourcePoint.Density * (bUseBounds ? (Contribution.Value * DensityNormalizationFactor) : Weight);
		}

		WeightedBoundsMin += SourcePoint.BoundsMin * Weight;
		WeightedBoundsMax += SourcePoint.BoundsMax * Weight;
		WeightedColor += SourcePoint.Color * Weight;

		ContributionsForMetadata.Emplace(Contribution.Key, static_cast<float>(Weight));
	}

	// Finally, apply changes to point, based on the projection settings
	if (InParams.bProjectPositions)
	{
		OutPoint.Transform.SetLocation(bSampleInVolume ? WeightedPosition : InTransform.GetLocation());
	}
	else
	{
		OutPoint.Transform.SetLocation(InTransform.GetLocation());
	}

	if (InParams.bProjectRotations)
	{
		WeightedQuat.Normalize();
		OutPoint.Transform.SetRotation(WeightedQuat);
	}
	else
	{
		OutPoint.Transform.SetRotation(InTransform.GetRotation());
	}

	if (InParams.bProjectScales)
	{
		OutPoint.Transform.SetScale3D(WeightedScale);
	}
	else
	{
		OutPoint.Transform.SetScale3D(InTransform.GetScale3D());
	}

	OutPoint.Density = static_cast<float>(WeightedDensity);
	OutPoint.BoundsMin = WeightedBoundsMin;
	OutPoint.BoundsMax = WeightedBoundsMax;
	OutPoint.Color = WeightedColor;

	return true;
}

void UDungeonForgePointData::RebuildOctree() const {
	FScopeLock Lock(&CachedDataLock);

	if (!bOctreeIsDirty)
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgePointData::RebuildOctree)
	check(bOctreeIsDirty);

	FBox PointBounds = GetBounds();
	TOctree2<FDungeonForgePointRef, FDungeonForgePointRefSemantics> NewOctree(PointBounds.GetCenter(), PointBounds.GetExtent().Length());

	for (const FDungeonForgePoint& Point : Points)
	{
		NewOctree.AddElement(FDungeonForgePointRef(Point));
	}

	Octree = MoveTemp(NewOctree);
	bOctreeIsDirty = false;
}

void UDungeonForgePointData::RecomputeBounds() const {
	FScopeLock Lock(&CachedDataLock);

	if (!bBoundsAreDirty)
	{
		return;
	}

	FBox NewBounds(EForceInit::ForceInit);
	for (const FDungeonForgePoint& Point : Points)
	{
		FBoxSphereBounds PointBounds = Point.GetWorldBounds();
		NewBounds += FBox::BuildAABB(PointBounds.Origin, PointBounds.BoxExtent);
	}

	Bounds = NewBounds;
	bBoundsAreDirty = false;
}

