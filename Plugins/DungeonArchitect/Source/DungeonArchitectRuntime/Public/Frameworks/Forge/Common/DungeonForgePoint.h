//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgePoint.generated.h"

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonForgePoint {
	GENERATED_BODY()
public:
	FDungeonForgePoint() = default;
	FDungeonForgePoint(const FTransform& InTransform, float InDensity, int32 InSeed);
	
	FBox GetLocalBounds() const;
	void SetLocalBounds(const FBox& InBounds);
	FBoxSphereBounds GetWorldBounds() const;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Properties)
	FTransform Transform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Properties)
	float Density = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Properties)
	FVector BoundsMin = -FVector::One();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Properties)
	FVector BoundsMax = FVector::One();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Properties)
	FVector4 Color = FVector4::One();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Properties)
	int32 Seed = 0;
	
	FVector GetExtents() const { return (BoundsMax - BoundsMin) / 2.0; }
	void SetExtents(const FVector& InExtents)
	{
		const FVector Center = GetLocalCenter();
		BoundsMin = Center - InExtents;
		BoundsMax = Center + InExtents;
	}

	FVector GetScaledExtents() const { return GetExtents() * Transform.GetScale3D(); }

	FVector GetLocalCenter() const { return (BoundsMax + BoundsMin) / 2.0; }
	void SetLocalCenter(const FVector& InCenter)
	{
		const FVector Delta = InCenter - GetLocalCenter();
		BoundsMin += Delta;
		BoundsMax += Delta;
	}
	
	FVector GetLocalSize() const { return BoundsMax - BoundsMin; }
	FVector GetScaledLocalSize() const { return GetLocalSize() * Transform.GetScale3D(); }

	void ApplyScaleToBounds();
	void ResetPointCenter(const FVector& BoundsRatio);
	bool Serialize(FStructuredArchive::FSlot Slot);
};

template<>
struct TStructOpsTypeTraits<FDungeonForgePoint> : public TStructOpsTypeTraitsBase2<FDungeonForgePoint> {
	enum {
		WithStructuredSerializer = true,
	};
};
