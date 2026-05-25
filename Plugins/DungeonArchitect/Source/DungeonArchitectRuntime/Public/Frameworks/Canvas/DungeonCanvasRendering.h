//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DungeonCanvasRendering.generated.h"

class UDungeonCanvasActorIconFilter;
class FDungeonLayoutHeightRange;
class UDungeonCanvasThemeRenderResources;
class UCanvasRenderTarget2D;
class UTextureRenderTarget2D;
class UDungeonCanvasMaterialTheme;
class UMaterial;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UWorld;
class UCanvas;

struct FDungeonCanvasDrawContext;
struct FDungeonCanvasOverlayIcon;
struct FDungeonCanvasOverlayInternalIcon;
struct FDungeonCanvasTrackedActorRegistryItem;
struct FDungeonCanvasViewportTransform;
struct FDungeonCanvasLayoutRenderResources;
struct FDungeonLayoutData;


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasLayoutFloorTextures {
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UCanvasRenderTarget2D> SDF = {};;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UCanvasRenderTarget2D> LayoutFill = {};;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UCanvasRenderTarget2D> LayoutBorder = {};;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UCanvasRenderTarget2D> DynamicOcclusion = {};;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UCanvasRenderTarget2D> FogOfWarExplored = {};;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UCanvasRenderTarget2D> FogOfWarVisibility = {};;

	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	float GroundHeightMin = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	float GroundHeightMax = 0;

	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	float GroundHeightBias = 0.1f;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasMaterialPool {
	GENERATED_BODY()

	UPROPERTY()
	TSet<TObjectPtr<UMaterialInstanceDynamic>> FreeMaterials;
	
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LockedMaterials;

	UMaterialInstanceDynamic* LockMaterial(UMaterialInterface* InParent);
	void UnlockAll();
	void Clear();
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasLayoutRenderResources {
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TMap<int32, FDungeonCanvasLayoutFloorTextures> FloorTextures;

	UPROPERTY()
	FDungeonCanvasMaterialPool MaterialPool;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> TextureMaterialTemplate = nullptr;

	void Reset();
};

UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasThemeRenderResources : public UObject {
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UMaterialInstanceDynamic> ThemeMaterialInstance = nullptr;;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UMaterialInstanceDynamic> FogOfWarMaterialInstance = nullptr;;
};

UENUM(BlueprintType)
enum class EDungeonCanvasIconFogOfWarRenderMode : uint8 {
	/** Always show, regardless of the fog of war state */
	Always = 0,

	/** Show when the icon is explored */
	ExploredAndVisibleOnly = 1,
	
	/** Show when the icon is on the player's line of sight */
	VisibleOnly = 2,
};

UENUM(BlueprintType)
enum class EDungeonCanvasDrawMaterialBlendMode : uint8 {
	Opaque,
	Translucent,
	AlphaWriteOnly,
	AlphaHoldout
};


class DUNGEONARCHITECTRUNTIME_API FDungeonCanvasRenderingLibrary {
public:
	struct FFloorTexturesGenerationOptions {
		FFloorTexturesGenerationOptions()
			: bGenerateSDF(true)
			, bGenerateLayoutFill(true)
			, bGenerateLayoutBorder(true)
			, bGenerateDynamicOcclusion(true)
			, bGenerateFogOfWarExplored(true)
			, bGenerateFogOfWarVisibility(true)
		{
		}
		
		bool bGenerateSDF;
		bool bGenerateLayoutFill;
		bool bGenerateLayoutBorder;
		bool bGenerateDynamicOcclusion;
		bool bGenerateFogOfWarExplored;
		bool bGenerateFogOfWarVisibility;
	};
	
	static void CreateDungeonCanvasLayoutFloorTextures(UObject* OuterOwner, int32 TextureWidth, int32 TextureHeight, FDungeonCanvasLayoutFloorTextures& FloorTextures, const FFloorTexturesGenerationOptions& InOptions = FFloorTexturesGenerationOptions());
	static void ReleaseDungeonCanvasLayoutResources(FDungeonCanvasLayoutRenderResources& LayoutResources);
	static void DrawDungeonLayout(UWorld* InWorld, const FDungeonLayoutData& InDungeonLayout, float PaddingWorldUnits, bool bDrawAllFloors, int32 FloorIndex,
		const float InHeightNormalizeBias, UTextureRenderTarget2D* OutLayoutFill, UTextureRenderTarget2D* OutLayoutBorder, float& OutRenderedMinHeight, float& OutRenderedMaxHeight);
	static void DrawDungeonLayout(UWorld* InWorld, const FDungeonLayoutData& InDungeonLayout, const FDungeonCanvasViewportTransform& FullDungeonTransform, const FDungeonLayoutHeightRange& HeightRange,
			const float InHeightNormalizeBias, UTextureRenderTarget2D* OutLayoutFill, UTextureRenderTarget2D* OutLayoutBorder, float& OutRenderedMinHeight, float& OutRenderedMaxHeight);
	
	
	static void UpdateDynamicOcclusions(UWorld* InWorld, const TArray<FDungeonCanvasTrackedActorRegistryItem>& InTrackedItems, FDungeonCanvasLayoutRenderResources& RenderResources, const FDungeonLayoutHeightRange& HeightRange,
			UCanvasRenderTarget2D* DynamicOcclusionTexture, const FDungeonCanvasViewportTransform& FullDungeonTransform, const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons);
	static void BlurTexture3x(UCanvasRenderTarget2D* SourceTexture, UCanvasRenderTarget2D* DestinationTexture);
	static void BlurTexture5x(UCanvasRenderTarget2D* SourceTexture, UCanvasRenderTarget2D* DestinationTexture);
	static void SetDungeonCanvasMaterialWorldBounds(UMaterialInstanceDynamic* Material, const FTransform& WorldBoundsTransform);
	static void DungeonCanvasDrawResource(UCanvas* Canvas, const FDungeonCanvasViewportTransform& ViewTransform, const FTransform& WorldBoundsTransform, EDungeonCanvasDrawMaterialBlendMode BlendMode, const TFunction<void(class FCanvasTriangleItem& Triangle)>& FuncSetResourceObject);
	static void DungeonCanvasDrawMaterial(UCanvas* Canvas, FDungeonCanvasViewportTransform ViewTransform, UMaterialInterface* Material, const FTransform& WorldBoundsTransform, EDungeonCanvasDrawMaterialBlendMode BlendMode = EDungeonCanvasDrawMaterialBlendMode::Translucent);
	static void DungeonCanvasDrawTexture(UCanvas* Canvas, FDungeonCanvasViewportTransform ViewTransform, UTexture* Texture, const FTransform& WorldBoundsTransform, EDungeonCanvasDrawMaterialBlendMode BlendMode = EDungeonCanvasDrawMaterialBlendMode::Translucent);
	static void DungeonCanvasDrawLayoutIcons(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial, FDungeonCanvasViewportTransform ViewTransform, const FDungeonLayoutHeightRange& HeightRange, const FDungeonLayoutData& DungeonLayout, const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons, float OpacityMultiplier = 1.0);
	static void DungeonCanvasDrawStairIcons(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial, FDungeonCanvasViewportTransform ViewTransform, const FDungeonLayoutHeightRange& HeightRange, const FDungeonLayoutData& DungeonLayout, const FDungeonCanvasOverlayInternalIcon& StairIcon, float OpacityMultiplier = 1.0);
	static void DungeonCanvasDrawTrackedActorIcons(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial, FDungeonCanvasViewportTransform ViewTransform, const TArray<FDungeonCanvasTrackedActorRegistryItem>& TrackedObjects, bool bFogOfWarEnabled, const FDungeonLayoutHeightRange& HeightRange, const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons, float OpacityMultiplier, const TArray<UDungeonCanvasActorIconFilter*>& OverlayActorIconFilters);
	static UDungeonCanvasThemeRenderResources* CreateDungeonCanvasRenderSettings(UObject* OuterOwner, UDungeonCanvasMaterialTheme* MaterialTheme);
	static void SetupMaterialFloorTextures(UMaterialInstanceDynamic* ThemeMaterial, UMaterialInstanceDynamic* FogOfWarMaterial, const FDungeonCanvasLayoutFloorTextures& FloorTextures);
	static void SetupMaterialFogOfWarState(UMaterialInstanceDynamic* ThemeMaterial, UMaterialInstanceDynamic* FogOfWarMaterial, bool bFogOfWarEnabled, bool bFullyExplored);
	static void BeginFogOfWarUpdate(UCanvasRenderTarget2D* FogOfWarExploredTexture, UCanvasRenderTarget2D* FogOfWarVisibilityTexture);
	static void UpdateFogOfWarExplorer(UCanvasRenderTarget2D* FogOfWarExploredTexture, UCanvasRenderTarget2D* FogOfWarVisibilityTexture,
			UTexture* SDFTexture, const FDungeonCanvasViewportTransform& FullDungeonTransform, const FVector2D& LightSourceLocation,
			float LightRadius, float NumShadowSamples, int ShadowJitterDistance);

	static void DrawDungeonLayoutHeightTexture(UWorld* InWorld, const FDungeonLayoutData& FilteredDungeonLayout, const FDungeonCanvasViewportTransform& InFullDungeonTransform,
			float InHeightNormalizeBias, UTextureRenderTarget2D* OutLayoutFill, float& OutRenderedMinHeight, float& OutRenderedMaxHeight);
	
	static float ConvertLocalSDFValueToWorldUnits(float NormalizedPixelSDFValue, const FVector& WorldBoundsScale);
	static void SetupMaterialParameters(UMaterialInstanceDynamic* MID, const FDungeonCanvasLayoutFloorTextures& FloorTextures, const FDungeonCanvasViewportTransform& ViewTransform, const FDungeonCanvasViewportTransform& FullDungeonTransform);

	static void GenerateFloorTextures(UWorld* World, UObject* Owner, const FDungeonLayoutData& InDungeonLayout, int32 LayoutTextureSize,
			float PaddingWorldUnits, bool bDrawAllFloors, int32 FloorIndex, FDungeonCanvasLayoutFloorTextures& OutFloorTextures, const FFloorTexturesGenerationOptions& InOptions = FFloorTexturesGenerationOptions());

	static void GenerateDungeonSDF(UTexture* FillTexture, UTexture* BorderTexture, UTexture* DynamicOcclusionTexture, UCanvasRenderTarget2D* SDFTexture);
};

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasBPFunctionLib : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void SetDungeonCanvasMaterialScalarParameter(UMaterialInstanceDynamic* Material, FName ParamName, float Value);
	
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void SetDungeonCanvasMaterialVectorParameter(UMaterialInstanceDynamic* Material, FName ParamName, FVector Value);
	
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void SetDungeonCanvasMaterialColorParameter(UMaterialInstanceDynamic* Material, FName ParamName, FLinearColor Value);
	
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void SetDungeonCanvasMaterialTextureParameter(UMaterialInstanceDynamic* Material, FName ParamName, UTexture* Value);

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void GenerateDungeonSDF(UTexture* FillTexture, UTexture* BorderTexture, UTexture* DynamicOcclusionTexture, UCanvasRenderTarget2D* SDFTexture);
	
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void GenerateDungeonVoronoiSdfEffect(UTexture* SDFTexture, UTexture* BorderTexture, UCanvasRenderTarget2D* TargetEffectTexture, float ScaleMin = 5, float ScaleMax = 20);
};

