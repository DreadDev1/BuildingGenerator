//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonEventListener.h"

#include "Curves/CurveFloat.h"
#include "Landscape.h"
#include "DungeonLandscapeModifier.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLandscapeModifier, Log, All);

class FDungeonLandscapeDataRasterizer;
struct FLandscapeEditDataInterface;

class UGridDungeonModel;
class UGridDungeonConfig;
class USimpleCityModel;
class USimpleCityConfig;

/**
* Modifies the landscape.  Attach to the dungeon actor
*/
UCLASS(NotBlueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonLandscapeModifier : public UDungeonEventListener {
    GENERATED_BODY()

public:
    UDungeonLandscapeModifier(const FObjectInitializer& ObjectInitializer);

    virtual void OnDungeonLayoutBuilt_Implementation(ADungeon* Dungeon) override;

private:
#if WITH_EDITOR

    void BuildLandscape(ADungeon* Dungeon);

    void RasterizeLayout(ADungeon* Dungeon, ULandscapeInfo* LandscapeInfo);
    void RasterizeLayoutGridBuilder(UGridDungeonModel* GridModel, UGridDungeonConfig* GridConfig,
                                    FDungeonLandscapeDataRasterizer& Rasterizer, float RasterizeBlurWeight,
                                    bool bMaskLayout, bool bRasterizeRooms, bool bRasterizeCorridors);
    void RasterizeLayoutCityBuilder(USimpleCityModel* CityModel, USimpleCityConfig* CityConfig,
                                    FDungeonLandscapeDataRasterizer& Rasterizer, const FVector& DungeonLocation,
                                    float RasterizeBlurWeight);

    void RasterizeWeights(ADungeon* Dungeon, ULandscapeInfo* LandscapeInfo);

    void BuildCollision(ULandscapeInfo* LandscapeInfo, UWorld* InWorld);

#endif // WITH_EDITOR

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<ALandscape> Landscape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float HeightBlurRadius = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    int32 HeightBlurIterations = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float HeightBlurWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float PaintBlurRadius = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    int32 PaintBlurIterations = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float PaintBlurWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<UCurveFloat> PaintBlurWeightCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float BaseHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<TObjectPtr<ULandscapeLayerInfoObject>> Layers;
};

