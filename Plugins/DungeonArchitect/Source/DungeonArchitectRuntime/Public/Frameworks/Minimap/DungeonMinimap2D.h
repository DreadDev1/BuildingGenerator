//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "DungeonMinimap2D.generated.h"

class UTextureRenderTarget2D;
class UDungeonModel;
class UDungeonConfig;
struct FDungeonLayoutData;
class UDungeonCanvasTrackedObject;
class UTexture;
class UTexture2D;
typedef TSharedPtr<class FDungeonMinimapBuilder> FDungeonMinimapBuilderPtr;

//////////////////////// 2D Minimap ////////////////////////
UCLASS(Abstract, HideDropdown)
class DUNGEONARCHITECTRUNTIME_API ADungeonMinimapBase : public AActor {
    GENERATED_BODY()
public:
    ADungeonMinimapBase();
    
public:
    UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "MiniMap")
    TArray<TObjectPtr<UDungeonCanvasTrackedObject>> DynamicTracking;

    UPROPERTY(BlueprintReadOnly, Category = "MiniMap")
    FTransform WorldToScreen;
    
    UPROPERTY(EditAnywhere, Category = "Fog Of War")
    bool bEnableFogOfWar = false;

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    int32 FogOfWarTextureSize = 512;

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    FName FogOfWarTrackingItem = "Player";

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    TObjectPtr<UTexture2D> FogOfWarExploreTexture;

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    float FogOfWarVisibilityDistance = 5000.0f;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> FogOfWarTexture = {};;
protected:
    FDungeonMinimapBuilderPtr MinimapBuilder;
    
    
protected:
    void UpdateFogOfWarTexture() const;
    void RecreateFogOfWarTexture();
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API ADungeonMinimap2D : public ADungeonMinimapBase {
    GENERATED_BODY()

public:
    ADungeonMinimap2D();

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    int32 TextureSize = 512;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    float OutlineThickness = 4.0f;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    float DoorThickness = 8.0f;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    TArray<FDungeonCanvasOverlayIcon> OverlayIcons;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    TObjectPtr<UMaterialInterface> MaterialTemplate;

    UPROPERTY(EditAnywhere, Category = "MiniMap Blur Layer")
    float BlurRadius = 5;

    UPROPERTY(EditAnywhere, Category = "MiniMap Blur Layer")
    int32 BlurIterations = 3;

    
    UPROPERTY(Transient)
    TObjectPtr<UTexture> MaskTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> StaticOverlayTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> DynamicOverlayTexture;

    UPROPERTY(Transient, BlueprintReadOnly, Category="MiniMap")
    TObjectPtr<UMaterialInterface> MaterialInstance;
    
public:
    UFUNCTION(BlueprintCallable, Category = Dungeon)
    UMaterialInterface* CreateMaterialInstance();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    UMaterialInterface* CreateMaterialInstanceFromTemplate(UMaterialInterface* InMaterialTemplate);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void UpdateMaterial(UMaterialInterface* InMaterial);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    virtual void BuildLayout(ADungeon* Dungeon);

    //// Begin AActor Interface ////
    virtual void BeginDestroy() override;
    virtual void Tick(float DeltaSeconds) override;
    //// End AActor Interface ////

protected:
    float GetAttributeScaleMultiplier() const;
    float GetIconPixelSize(const FDungeonCanvasOverlayIcon& OverlayData, const FVector2D& CanvasSize) const;

};

