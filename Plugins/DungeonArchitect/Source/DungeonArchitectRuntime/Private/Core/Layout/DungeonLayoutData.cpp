//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Layout/DungeonLayoutData.h"


FBox FDungeonLayoutUtils::CalculateBounds(const FDungeonLayoutDataChunkInfo& LayoutShapes, const FCalcBoundsSettings& Settings) {
	FBox Bounds(ForceInit);
	for (const FDAShapeCircle& CircleItem : LayoutShapes.Circles) {
		const FVector ShapeCenter = CircleItem.Transform.GetLocation();
		const float Radius = CircleItem.Radius * FMath::Max(CircleItem.Transform.GetScale3D().X, CircleItem.Transform.GetScale3D().Y);
		const float Height = CircleItem.Height * CircleItem.Transform.GetScale3D().Z;

		const FVector BoxMin = ShapeCenter - FVector(Radius, Radius, 0);
		const FVector BoxMax = ShapeCenter + FVector(Radius, Radius, Height);
		const FBox ShapeBox(BoxMin, BoxMax);
		Bounds += ShapeBox;
	}
	for (const FDAShapePolygon& PolyItem : LayoutShapes.ConvexPolys) {
		for (const FVector2D PolyPoint : PolyItem.Points) {
			Bounds += PolyItem.Transform.TransformPosition(FVector(PolyPoint, 0));
			Bounds += PolyItem.Transform.TransformPosition(FVector(PolyPoint, PolyItem.Height));
		}
	}
	for (const FDAShapeLine& LineItem : LayoutShapes.Outlines) {
		Bounds += LineItem.Transform.TransformPosition(FVector(LineItem.LineStart, 0));
		Bounds += LineItem.Transform.TransformPosition(FVector(LineItem.LineEnd, 0));
		Bounds += LineItem.Transform.TransformPosition(FVector(LineItem.LineStart, LineItem.Height));
		Bounds += LineItem.Transform.TransformPosition(FVector(LineItem.LineEnd, LineItem.Height));
	}
	if (Settings.bIncludeTextureOverlays) {
		for (const FDAShapeTexture& TexItem : LayoutShapes.CanvasShapeTextures) {
			for (int i = 0; i < 4; i++) {
				Bounds += TexItem.Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[i]);
			}
		}	
	}
	
	return Bounds;
}

namespace DungeonLayoutUtils {
	struct FShapeRangePolicy {
		template<typename TShape>
		static void GetItemRange(const TShape& Item, float& OutHeightLower, float& OutHeightUpper) {
			OutHeightLower = Item.Transform.GetLocation().Z;
			OutHeightUpper = OutHeightLower + Item.Height;
		}
	};
	
	struct FPointRangePolicy {
		template<typename TShape>
		static void GetItemRange(const TShape& Item, float& OutHeightLower, float& OutHeightUpper) {
			OutHeightLower = Item.Transform.GetLocation().Z;
			OutHeightUpper = OutHeightLower;
		}
	};
	
	struct FDoorRangePolicy {
		template<typename TShape>
		static void GetItemRange(const TShape& Item, float& OutHeightLower, float& OutHeightUpper) {
			OutHeightLower = OutHeightUpper = Item.WorldTransform.GetLocation().Z;
		}
	};
	
	struct FPointOnInterestRangePolicy {
		template<typename TShape>
		static void GetItemRange(const TShape& Item, float& OutHeightLower, float& OutHeightUpper) {
			OutHeightLower = Item.Transform.GetLocation().Z;
			OutHeightUpper = OutHeightLower;
		}
	};
	
	class FShapeFilter {
	public:
		FShapeFilter(float MinHeight, float MaxHeight)
			: MinHeight(MinHeight)
			, MaxHeight(MaxHeight)
		{
		}

		template<typename TRangePolicy, typename TShape>
		TArray<TShape> Filter(const TArray<TShape>& Items) const {
			return Items.FilterByPredicate([this](const TShape& Item) {
				float HeightLower{}, HeightUpper{};
				TRangePolicy::GetItemRange(Item, HeightLower, HeightUpper);
				return IsInHeightRange(HeightLower, HeightUpper);
			});
		}

	private:
		static float GetOverlapsDistance(float MinA, float MaxA, float MinB, float MaxB) {
			return FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
		};
	
		bool IsInHeightRange(float _H0, float _H1, float Tolerance = 1e-4f) const {
			const float H0 = FMath::Min(_H0, _H1);
			const float H1 = FMath::Max(_H0, _H1);
			if (H1 - H0 < Tolerance) {
				// This is a point.  Check if the point is inside the range
				return H0 + Tolerance >= MinHeight && H1 - Tolerance <= MaxHeight;
			}

			// Check if they overlap
			const float OverlapDist = GetOverlapsDistance(
				H0, H1,
				MinHeight,
				MaxHeight);

			return OverlapDist - Tolerance > 0;
		};

	private:
		float MinHeight{};
		float MaxHeight{};
	};
}

FDungeonLayoutData FDungeonLayoutUtils::FilterByHeightRange(const FDungeonLayoutData& InLayoutData, float MinHeight, float MaxHeight) {
	FDungeonLayoutData Result = InLayoutData;

	if (MaxHeight < MinHeight) {
		const float Temp = MinHeight;
		MinHeight = MaxHeight;
		MaxHeight = Temp;
	}

	using namespace DungeonLayoutUtils;
	const FShapeFilter ShapeFilter(MinHeight, MaxHeight);
	for (FDungeonLayoutDataChunkInfo& ChunkShape : Result.ChunkShapes) {
		ChunkShape.Circles = ShapeFilter.Filter<FShapeRangePolicy>(ChunkShape.Circles);
		ChunkShape.Outlines = ShapeFilter.Filter<FShapeRangePolicy>(ChunkShape.Outlines);
		ChunkShape.ConvexPolys = ShapeFilter.Filter<FShapeRangePolicy>(ChunkShape.ConvexPolys);
		ChunkShape.CanvasShapeTextures = ShapeFilter.Filter<FPointRangePolicy>(ChunkShape.CanvasShapeTextures);
		ChunkShape.PointsOfInterest = ShapeFilter.Filter<FPointOnInterestRangePolicy>(ChunkShape.PointsOfInterest);
	}
	Result.Doors = ShapeFilter.Filter<FDoorRangePolicy>(Result.Doors);
	Result.WallOpening = ShapeFilter.Filter<FDoorRangePolicy>(Result.WallOpening);
	Result.PointsOfInterest = ShapeFilter.Filter<FPointOnInterestRangePolicy>(Result.PointsOfInterest);
	Result.ChunkShapes = Result.ChunkShapes.FilterByPredicate([](const FDungeonLayoutDataChunkInfo& ChunkInfo) -> bool {
		const int32 TotalShapes = ChunkInfo.Circles.Num()
			+ ChunkInfo.ConvexPolys.Num()
			+ ChunkInfo.Outlines.Num()
			+ ChunkInfo.CanvasShapeTextures.Num();
		return TotalShapes > 0;
	});
	
	return Result;
}


//////////////////////////////// FDungeonLayoutHeightRange ////////////////////////////////
void FDungeonLayoutHeightRange::SetHeightRangeForSingleFloor(const FDungeonFloorSettings& InFloorSettings, int32 FloorIndex) {
	ActiveFloorIndices = { FloorIndex };
	RangeType = FDungeonLayoutHeightRangeType::Floor;
	UpdateFloorRange(InFloorSettings);
}

void FDungeonLayoutHeightRange::SetHeightRangeForMultipleFloors(const FDungeonFloorSettings& InFloorSettings, const TArray<int32>& InFloorIndices) {
	ActiveFloorIndices = InFloorIndices;
	RangeType = FDungeonLayoutHeightRangeType::Floor;
	UpdateFloorRange(InFloorSettings);
}

void FDungeonLayoutHeightRange::SetHeightRangeCustom(float InHeightRangeMin, float InHeightRangeMax) {
	HeightRangeMin = InHeightRangeMin;
	HeightRangeMax = InHeightRangeMax;
	RangeType = FDungeonLayoutHeightRangeType::CustomRange;
}

void FDungeonLayoutHeightRange::SetHeightRangeAcrossAllFloors() {
	HeightRangeMin = -100000000.0f;
	HeightRangeMax = 100000000.0f;
	ActiveFloorIndices = { 0 };
	RangeType = FDungeonLayoutHeightRangeType::EntireDungeon;
}

void FDungeonLayoutHeightRange::UpdateFloorRange(const FDungeonFloorSettings& InFloorSettings) {
	for (int i = 0; i < ActiveFloorIndices.Num(); i++) {
		const int32 FloorIdx = ActiveFloorIndices[i];
		const float Min = InFloorSettings.FloorHeight * FloorIdx + InFloorSettings.GlobalHeightOffset + BaseHeightOffset;
		const float Max = Min + InFloorSettings.FloorCaptureHeight;

		if (i == 0) {
			HeightRangeMin = Min;
			HeightRangeMax = Max;
		}
		else {
			HeightRangeMin = FMath::Min(Min, HeightRangeMin);
			HeightRangeMax = FMath::Max(Max, HeightRangeMax);
		}
	}
}

int32 FDungeonLayoutHeightRange::GetFloorIndexAtHeight(const FDungeonFloorSettings& InFloorSettings, float HeightZ) const {
	const float FloorIndexF = (HeightZ - InFloorSettings.GlobalHeightOffset - BaseHeightOffset) / InFloorSettings.FloorHeight;
	const int32 FloorIndex = FMath::FloorToInt(FloorIndexF);
	return FMath::Clamp(FloorIndex, InFloorSettings.FloorLowestIndex, InFloorSettings.FloorHighestIndex);
}

bool FDungeonLayoutHeightRange::InsideActiveFloorHeightRange(float HeightZ) const {
	if (RangeType == FDungeonLayoutHeightRangeType::EntireDungeon) {
		return true;
	}

	return HeightZ >= HeightRangeMin && HeightZ <= HeightRangeMax;
}

