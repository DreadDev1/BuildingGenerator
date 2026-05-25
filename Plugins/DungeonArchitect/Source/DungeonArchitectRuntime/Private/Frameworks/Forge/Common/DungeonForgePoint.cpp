//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Common/DungeonForgePoint.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DungeonForgePoint)

namespace DungeonForgePointCustomPropertyNames
{
	const FName ExtentsName = TEXT("Extents");
	const FName LocalCenterName = TEXT("LocalCenter");
	const FName PositionName = TEXT("Position");
	const FName RotationName = TEXT("Rotation");
	const FName ScaleName = TEXT("Scale");
	const FName LocalSizeName = TEXT("LocalSize");
	const FName ScaledLocalSizeName = TEXT("ScaledLocalSize");
}

/** Serialized fields of a FDungeonForgePoint, the values here can't change as they are being used to mask out serialization */
enum class EDungeonForgePointSerializeFields : uint8
{
	None = 0 ,
	Density = 1 << 0,
	BoundsMin = 1 << 1,
	BoundsMax = 1 << 2,
	Color = 1 << 3,
	Seed = 1 << 4,
};

ENUM_CLASS_FLAGS(EDungeonForgePointSerializeFields);

FDungeonForgePoint::FDungeonForgePoint(const FTransform& InTransform, float InDensity, int32 InSeed)
	: Transform(InTransform)
	, Density(InDensity)
	, Seed(InSeed)
{
}

bool FDungeonForgePoint::Serialize(FStructuredArchive::FSlot Slot)
{
	FArchive& UnderlyingArchive = Slot.GetUnderlyingArchive();
		
	const FDungeonForgePoint Default;
	EDungeonForgePointSerializeFields SerializeMask = EDungeonForgePointSerializeFields::None;
	if (!UnderlyingArchive.IsLoading())
	{
		if (Density != Default.Density)
		{
			EnumAddFlags(SerializeMask, EDungeonForgePointSerializeFields::Density);
		}

		if (BoundsMin != Default.BoundsMin)
		{
			EnumAddFlags(SerializeMask, EDungeonForgePointSerializeFields::BoundsMin);
		}

		if (BoundsMax != Default.BoundsMax)
		{
			EnumAddFlags(SerializeMask, EDungeonForgePointSerializeFields::BoundsMax);
		}

		if (Color != Default.Color)
		{
			EnumAddFlags(SerializeMask, EDungeonForgePointSerializeFields::Color);
		}

		if (Seed != Default.Seed)
		{
			EnumAddFlags(SerializeMask, EDungeonForgePointSerializeFields::Seed);
		}
	}
		
	FStructuredArchive::FRecord Record = Slot.EnterRecord();

	Record << SA_VALUE(TEXT("SerializeMask"), SerializeMask);
	Record << SA_VALUE(TEXT("Transform"), Transform);

	if (EnumHasAnyFlags(SerializeMask, EDungeonForgePointSerializeFields::Density))
	{
		Record << SA_VALUE(TEXT("Density"), Density);
	}
		
	if (EnumHasAnyFlags(SerializeMask, EDungeonForgePointSerializeFields::BoundsMin))
	{
		Record << SA_VALUE(TEXT("BoundsMin"), BoundsMin);
	}
	
	if (EnumHasAnyFlags(SerializeMask, EDungeonForgePointSerializeFields::BoundsMax))
	{
		Record << SA_VALUE(TEXT("BoundsMax"), BoundsMax);
	}
	
	if (EnumHasAnyFlags(SerializeMask, EDungeonForgePointSerializeFields::Color))
	{
		Record << SA_VALUE(TEXT("Color"), Color);
	}
	
	if (EnumHasAnyFlags(SerializeMask, EDungeonForgePointSerializeFields::Seed))
	{
		Record << SA_VALUE(TEXT("Seed"), Seed);
	}
	
	return true;
}

FBox FDungeonForgePoint::GetLocalBounds() const
{
	return FBox(BoundsMin, BoundsMax);
}

void FDungeonForgePoint::SetLocalBounds(const FBox& InBounds)
{
	BoundsMin = InBounds.Min;
	BoundsMax = InBounds.Max;
}

FBoxSphereBounds FDungeonForgePoint::GetWorldBounds() const {
	return FBoxSphereBounds(GetLocalBounds().TransformBy(Transform));
}

void FDungeonForgePoint::ApplyScaleToBounds()
{
	const FVector PointScale = Transform.GetScale3D();
	Transform.SetScale3D(PointScale.GetSignVector());
	BoundsMin *= PointScale.GetAbs();
	BoundsMax *= PointScale.GetAbs();
}

void FDungeonForgePoint::ResetPointCenter(const FVector& BoundsRatio)
{
	const FVector NewCenterLocal = FMath::Lerp(BoundsMin, BoundsMax, BoundsRatio);

	BoundsMin -= NewCenterLocal;
	BoundsMax -= NewCenterLocal;

	Transform.SetLocation(Transform.GetLocation() + Transform.TransformVector(NewCenterLocal));
}

