//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Common/DungeonForgePoint.h"
#include "Frameworks/Forge/Data/DungeonForgeSpatialData.h"

#include "Math/GenericOctree.h"
#include "DungeonForgePointData.generated.h"

struct DUNGEONARCHITECTRUNTIME_API FDungeonForgePointRef
{
	FDungeonForgePointRef(const FDungeonForgePoint& InPoint);
	FDungeonForgePointRef(const FDungeonForgePoint& InPoint, const FBox& InBoundsOverride);

	const FDungeonForgePoint* Point;
	FBoxSphereBounds Bounds;
};


namespace DungeonForgePointHelpers
{
	void Lerp(const FDungeonForgePoint& A, const FDungeonForgePoint& B, float Ratio, FDungeonForgePoint& OutPoint);
	void Bilerp(const FDungeonForgePoint& X0Y0, const FDungeonForgePoint& X1Y0, const FDungeonForgePoint& X0Y1, const FDungeonForgePoint& X1Y1, FDungeonForgePoint& OutPoint, float XFactor, float YFactor);
	void BilerpWithSnapping(const FDungeonForgePoint& X0Y0, const FDungeonForgePoint& X1Y0, const FDungeonForgePoint& X0Y1, const FDungeonForgePoint& X1Y1, FDungeonForgePoint& OutPoint, float XFactor, float YFactor);
}

struct DUNGEONARCHITECTRUNTIME_API FDungeonForgePointRefSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FDungeonForgePointRef& InPoint)
	{
		return InPoint.Bounds;
	}

	FORCEINLINE static const bool AreElementsEqual(const FDungeonForgePointRef& A, const FDungeonForgePointRef& B)
	{
		// TODO: verify if that's sufficient
		return A.Point == B.Point;
	}

	FORCEINLINE static void ApplyOffset(FDungeonForgePointRef& InPoint)
	{
		ensureMsgf(false, TEXT("Not implemented"));
	}

	FORCEINLINE static void SetElementId(const FDungeonForgePointRef& Element, FOctreeElementId2 OctreeElementID)
	{
	}
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgePointData : public UDungeonForgeSpatialData {
	GENERATED_BODY()
public:
	typedef TOctree2<FDungeonForgePointRef, FDungeonForgePointRefSemantics> PointOctree;
	
	//~Begin UObject Interface
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~End UObject Interface
	
	// ~Begin UDungeonForgeData interface
	virtual EDungeonForgeDataType GetDataType() const override { return EDungeonForgeDataType::Point; }
	// ~End UDungeonForgeData interface
	
	// ~Begin UDungeonForgeSpatialData interface
	virtual int GetDimension() const override { return 0; }
	virtual FBox GetBounds() const override;
	virtual const UDungeonForgePointData* ToPointData(const FBox& InBounds = FBox(EForceInit::ForceInit)) const override { return this; }
	virtual bool SamplePoint(const FTransform& Transform, const FBox& Bounds, FDungeonForgePoint& OutPoint) const override;
	virtual bool ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint) const override;

protected:
	virtual UDungeonForgeSpatialData* CopyInternal() const override;
	// ~End UDungeonForgeSpatialData interface

public:
	UFUNCTION(BlueprintCallable, Category = SpatialData)
	const TArray<FDungeonForgePoint>& GetPoints() const { return Points; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = SpatialData)
	TArray<FDungeonForgePoint> GetPointsCopy() const { return Points; }

	UFUNCTION(BlueprintCallable, Category = SpatialData)
	int32 GetNumPoints() const { return Points.Num(); }

	UFUNCTION(BlueprintCallable, Category = SpatialData)
	bool IsEmpty() const { return Points.IsEmpty(); }

	UFUNCTION(BlueprintCallable, Category = SpatialData)
	FDungeonForgePoint GetPoint(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = SpatialData)
	void SetPoints(const TArray<FDungeonForgePoint>& InPoints);

	UFUNCTION(BlueprintCallable, Category = SpatialData)
	void CopyPointsFrom(const UDungeonForgePointData* InData, const TArray<int>& InDataIndices);

	TArray<FDungeonForgePoint>& GetMutablePoints();

	const PointOctree& GetOctree() const;
	
	bool ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint, bool bUseBounds) const;

	/** Get the dirty status of the Octree. Note that the Octree can be rebuilt from another thread, so this info can be invalidated at anytime. */
	bool IsOctreeDirty() const { return bOctreeIsDirty; }
	
protected:
	void RebuildOctree() const;
	void RecomputeBounds() const;

	UPROPERTY()
	TArray<FDungeonForgePoint> Points;

	mutable FCriticalSection CachedDataLock;
	mutable PointOctree Octree;
	mutable FBox Bounds; // TODO: review if this needs to be threadsafe
	mutable bool bBoundsAreDirty = true;
	mutable bool bOctreeIsDirty = true;
};

