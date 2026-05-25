//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonShapes.h"

#include "Components/PrimitiveComponent.h"
#include "SnapModuleBoundShape.generated.h"

struct FDungeonPointOfInterest;

UCLASS() 
class USnapModuleBoundShapeRenderComponent : public UPrimitiveComponent {
	GENERATED_BODY()
public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool IsEditorOnly() const override;
	
	//~ Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface
};

USTRUCT()
struct FSnapModuleBoundShapeConvexPoly {
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> Points;
};

UCLASS(hidecategories=(Input))
class DUNGEONARCHITECTRUNTIME_API ASnapModuleBoundShape : public AActor {
	GENERATED_BODY()
public:
	ASnapModuleBoundShape();
	virtual void PostLoad() override;
	
	void RegenerateBoundsData();
	const TArray<FSnapModuleBoundShapeConvexPoly>& GetConvexPolys() const { return ConvexPolys; }
	void GatherShapes(FDAShapeList& OutShapes) const;
	void SetDrawColor(const FLinearColor& InDrawColor);


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	bool IsPolyTriangulationValid() const { return bPolyTriangulationValid; }

private:
	void GenerateConvexPolygons();

public:
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<USnapModuleBoundShapeRenderComponent> BoundsRenderComponent;
#endif

	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	bool bDrawBounds{true};

	/** Should we render this bounds in the Dungeon Canvas texture */
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	bool bRenderOnCanvas{true};
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	EDAShapePolygonType BoundsType = EDAShapePolygonType::Polygon;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	TArray<FVector> PolygonPoints = {
		{ -1500, 1000, 0 },
		{ -1500, -800, 0 },
		{ 700, -900, 0 },
		{ 2100, -3100, 0 },
		{ 3300, -1900, 0 },
		{ 1500, 1000, 0 }
	};

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FVector CircleRadius = FVector(2000, 0, 0);

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FVector BoxExtent = FVector(1200, 800, 0);

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FVector Height = FVector(0, 0, 1500);

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FLinearColor DrawColor = FLinearColor(1, 0, 0, 1);

	/** Should these bounds be used to draw the shape of the module in the dungeon canvas? */
	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	bool bDrawOnDungeonCanvas = true;
	
private:
	UPROPERTY()
	TArray<FSnapModuleBoundShapeConvexPoly> ConvexPolys;

	UPROPERTY()
	bool bPolyTriangulationValid{};
	
#if WITH_EDITORONLY_DATA
	bool bPostEditMoveGuard{};
#endif // WITH_EDITORONLY_DATA

};

struct FDungeonCanvasRoomShapeTextureList;

class DUNGEONARCHITECTRUNTIME_API FSnapModuleBoundShapeUtils {
public:
	static void InitializeModuleBoundShapes(const TArray<TObjectPtr<AActor>>& InActors, const FBox& InFallbackModuleBounds,
				FDAShapeList& OutModuleBoundShapes, FDungeonCanvasRoomShapeTextureList& OutCanvasRoomShapeTextures,
				TArray<FDungeonPointOfInterest>& OutPointsOfInterest);

	static void CreateHullPointsFromBoxExtents(const FVector2D& InExtents, TArray<FVector2D>& OutConvexHull);
	static FDAShapePolygon CreateFallbackModuleConvexHull(const FBox& ModuleBounds);
};

/*
UCLASS(hidecategories=(Input), HideDropdown, NotPlaceable)
class DUNGEONARCHITECTRUNTIME_API ASnapModuleBoundShapeCollisionDebug : public AActor {
	GENERATED_BODY()
public:
	ASnapModuleBoundShapeCollisionDebug();
	
	UPROPERTY(EditAnywhere, Category=Snap)
	TArray<TObjectPtr<ASnapModuleBoundShape>> TrackedActors;
	
	UPROPERTY(EditAnywhere, Category=Snap)
	float Tolerance = 0;

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual bool IsLevelBoundsRelevant() const override { return false; }
};
*/

