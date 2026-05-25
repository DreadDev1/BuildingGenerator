//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "DungeonForgeSpatialData.generated.h"

class UDungeonForgePointData;
struct FDungeonForgePoint;
struct FDungeonForgeProjectionParams;

/**
* "Concrete" data base class for Dungeon Forge generation
* This will be the base class for data classes that actually represent
* concrete evidence of spatial data - points, surfaces, splines, etc.
* In opposition to settings/control type of data.
*/
UCLASS()
class UDungeonForgeSpatialData : public UDungeonForgeData {
	GENERATED_BODY()
public:
	UDungeonForgeSpatialData(const FObjectInitializer& ObjectInitializer);
	
	// ~Begin UDungeonForgeData interface
	virtual EDungeonForgeDataType GetDataType() const override { return EDungeonForgeDataType::Spatial; }
	virtual UDungeonForgeSpatialData* DuplicateData() const override;
	// ~End UDungeonForgeData interface
	
	/** Returns the dimension of the data type, which has nothing to do with the dimension of its points */
	UFUNCTION(BlueprintCallable, Category = SpatialData)
	virtual int GetDimension() const PURE_VIRTUAL(UDungeonForgeSpatialData::GetDimension, return 0;);

	/** Returns the full bounds (including density fall-off) of the data */
	UFUNCTION(BlueprintCallable, Category = SpatialData)
	virtual FBox GetBounds() const PURE_VIRTUAL(UDungeonForgeSpatialData::GetBounds, return FBox(EForceInit::ForceInit););

	/** Returns whether a given spatial data is bounded as some data types do not require bounds by themselves */
	virtual bool IsBounded() const { return true; }

	/** Returns the expected data normal (for surfaces) or eventual projection axis (for volumes) */
	UFUNCTION(BlueprintCallable, Category = SpatialData)
	virtual FVector GetNormal() const { return FVector::UnitZ(); }

	virtual const UDungeonForgePointData* ToPointData(const FBox& InBounds = FBox(EForceInit::ForceInit)) const PURE_VIRTUAL(UDungeonForgeSpatialData::ToPointData, return nullptr;);

	/** Sample rotation, scale and other attributes from this data at the query position. Returns true if Transform location and Bounds overlaps this data. */
	virtual bool SamplePoint(const FTransform& Transform, const FBox& Bounds, FDungeonForgePoint& OutPoint) const PURE_VIRTUAL(UDungeonForgeSpatialData::SamplePoint, return false;);

	/** Performs multiple samples at the same time.
	* Contrary to the single SamplePoint call, this is expected to set the density to 0 for points that were not overlapping - but the other properties can be anything.
	* The OutPoints arrays is expected pre-allocated to the size of the Samples.
	*/
	virtual void SamplePoints(const TArrayView<const TPair<FTransform, FBox>>& Samples, const TArrayView<FDungeonForgePoint>& OutPoints) const;

	/** Sample rotation, scale and other attributes from this data at the query position. Returns true if Transform location and Bounds overlaps this data. */
	UFUNCTION(BlueprintCallable, Category = SpatialData, meta = (DisplayName = "Sample Point"))
	bool K2_SamplePoint(const FTransform& Transform, const FBox& Bounds, FDungeonForgePoint& OutPoint) const;

	/** Project the query point onto this data, and sample point and metadata information at the projected position. Returns true if successful. */
	virtual bool ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint) const;
	
	/** Performs multiple projections of samples at the same time.
	* Contrary to the single ProjectPoint call, this is expected to set the density to 0 for points that were not overlapping - but the other properties can be anything.
	* The OutPoints arrays is expected pre-allocated to the size of the Samples.
	*/
	virtual void ProjectPoints(const TArrayView<const TPair<FTransform, FBox>>& Samples, const FDungeonForgeProjectionParams& InParams, const TArrayView<FDungeonForgePoint>& OutPoints) const;

	UFUNCTION(BlueprintCallable, Category = SpatialData, meta = (DisplayName = "Project Point"))
	bool K2_ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FDungeonForgeProjectionParams& InParams, FDungeonForgePoint& OutPoint) const;

	/** Returns true if the data has a non-trivial transform */
	UFUNCTION(BlueprintCallable, Category = SpatialData)
	virtual bool HasNonTrivialTransform() const { return false; }
	
protected:
	virtual UDungeonForgeSpatialData* CopyInternal() const PURE_VIRTUAL(UDungeonForgeSpatialData::CopyInternal, return nullptr;);
};

