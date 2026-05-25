//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonShapes.generated.h"

class UTexture2D;

struct DUNGEONARCHITECTRUNTIME_API FDAShapeConstants {
	static const FName TagDoNotRenderOnCanvas;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapeBase {
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	FTransform Transform;
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	float Height{};

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FName> Tags;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapeCircle : public FDAShapeBase {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	float Radius{};
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapeLine : public FDAShapeBase {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	FVector2D LineStart = FVector2D::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	FVector2D LineEnd = FVector2D::ZeroVector;
};

namespace DAShapePolyLineTags {
	const FName Outline = TEXT("Outline");
}

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapePolyLine : public FDAShapeBase {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FVector2D> Points;
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	bool bClosed = false;
};


UENUM()
enum class EDAShapePolygonType : uint8 {
	Polygon,
	Box,
	Circle
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapePolygon : public FDAShapeBase {
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FVector2D> Points;
};

UENUM(BlueprintType)
enum class EDAShapeTextureBlendMode : uint8 {
	Add,
	Multiply,
	Opaque
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapeTexture : public FDAShapeBase {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TSoftObjectPtr<UTexture2D> TextureMask = nullptr;

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	EDAShapeTextureBlendMode BlendMode = EDAShapeTextureBlendMode::Add;

	static const FVector LocalQuadPoints[];
	static const FVector2D QuadUV[];
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAShapeList {
	GENERATED_BODY()

	/** The convex polys, used for rendering and collision detection */
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FDAShapePolygon> ConvexPolys;

	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FDAShapePolyLine> PolyLines;
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FDAShapeCircle> Circles;

	FDAShapeList TransformBy(const FTransform& InTransform) const;
	FORCEINLINE int32 GetTotalCustomShapes() const {
		// Texture shapes do not contribute to the bounds
		return ConvexPolys.Num() + Circles.Num();
	}

	FBox CalcBounds();
};

class FDAShapeUtils {
public:
	static FBox CalcBounds(const FDAShapeCircle& InCircle);
	static FBox CalcBounds(const FDAShapeLine& InLine);
	static FBox CalcBounds(const FDAShapePolygon& InPoly);
};

class DUNGEONARCHITECTRUNTIME_API FDAShapeCollision {
public:
	static void ConvertBoxToConvexPoly(const FBox& InBox, FDAShapePolygon& OutPoly);
	static void ConvertBoxToConvexPoly(const FTransform& InTransform, const FVector& InExtent, FDAShapePolygon& OutPoly);
	static bool Intersects(const FDAShapePolygon& A, const FDAShapePolygon& B, float Tolerance);
	static bool Intersects(const FDAShapePolygon& A, const FDAShapeCircle& B, float Tolerance);
	static bool Intersects(const FDAShapeCircle& A, const FDAShapeCircle& B, float Tolerance);
	static bool Intersects(const FDAShapeList& A, const FDAShapeList& B, float Tolerance);
	static bool Intersects(const FDAShapePolygon& Poly, const FDAShapeLine& Line, float Tolerance);
	
	static bool Intersects(const FBox& A, const FDAShapePolygon& B, float Tolerance);
	static bool Intersects(const FBox& A, const FDAShapeCircle& B, float Tolerance);
	static bool Intersects(const FBox& A, const FDAShapeList& B, float Tolerance);

	static void TransformPoints(const TArray<FVector2D>& InPoints, const FTransform& InTransform, TArray<FVector2D>& OutTransformedPoints);
	
private:
	static void PopulatePolyProjectionNormals(const TArray<FVector2D>& InTransformedPoints, TSet<FVector2D>& Visited, TArray<FVector2D>& OutProjections);
	static void PopulateCircleProjectionNormals(const FVector2D& InTransformedCenter, const TArray<FVector2D>& InTransformedPolyPoints, TSet<FVector2D>& Visited, TArray<FVector2D>& OutProjections);
	
	FORCEINLINE static float GetOverlapsDistance(float MinA, float MaxA, float MinB, float MaxB) {
		return FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
	}

	template<typename TA, typename TB>
	static float GetHeightOverlapDistance(const TA& A, const TB& B) {
		const float MinA = A.Transform.GetLocation().Z;
		const float MaxA = MinA + A.Height;
		const float MinB = B.Transform.GetLocation().Z;
		const float MaxB = MinB + B.Height;
		return GetOverlapsDistance(MinA, MaxA, MinB, MaxB);
	}
    
	template<typename TA, typename TB>
	static bool HeightOverlaps(const TA& A, const TB& B, float Tolerance) {
		return GetHeightOverlapDistance(A, B) - Tolerance > 0;
	}
	
	template<typename TB>
	static float GetBoxHeightOverlapDistance(const FBox& A, const TB& B) {
		const float MinA = A.Min.Z;
		const float MaxA = A.Max.Z;
		const float MinB = B.Transform.GetLocation().Z;
		const float MaxB = MinB + B.Height;
		return GetOverlapsDistance(MinA, MaxA, MinB, MaxB);
	}
	
	template<typename TB>
	static bool BoxHeightOverlaps(const FBox& A, const TB& B, float Tolerance) {
		return GetBoxHeightOverlapDistance(A, B) - Tolerance > 0;
	}
};

