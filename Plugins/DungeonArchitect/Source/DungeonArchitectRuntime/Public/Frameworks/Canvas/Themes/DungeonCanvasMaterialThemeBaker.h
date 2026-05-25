//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/UI/DungeonCanvasWidgetBase.h"
#include "DungeonCanvasMaterialThemeBaker.generated.h"

class UDungeonCanvasThemeRenderResources;
class UCanvasRenderTarget2D;
class UDungeonCanvasMaterialTheme;
class UMaterialInstanceDynamic;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasMaterialThemeBaker : public UObject {
	GENERATED_BODY()
public:
	void ReleaseResources();
	void Tick(const FIntPoint& InDesiredTextureSize);
	FDungeonCanvasViewportTransform GetCachedViewTransform() const { return CachedViewTransform; }
	
	UCanvasRenderTarget2D* GetRenderedTexture() const;

public:
	TAttribute<UDungeonCanvasMaterialTheme*> ThemeOverride;
	TAttribute<FDungeonCanvasDrawSettings> DrawSettings;
	TAttribute<UDungeonCanvasComponent*> DungeonCanvas;
	TAttribute<UDungeonCanvasCamera*> DungeonCamera;
	
protected:
	UFUNCTION()
	void CanvasDrawCallback(UCanvas* Canvas, int32 Width, int32 Height);

	void CreateRenderResources(const FVector2D& InSize);
	void ResizeTextureIfNeeded(const FIntPoint& InDesiredTextureSize) const;
	FDungeonCanvasDrawContext CreateDrawContext(UCanvas* Canvas, int32 Width, int32 Height) const;
	UDungeonCanvasMaterialTheme* GetTheme() const;
	
protected:	
	UPROPERTY()
	TObjectPtr<UCanvasRenderTarget2D> CanvasRenderTexture;
		
	UPROPERTY(Transient)
	TObjectPtr<UDungeonCanvasThemeRenderResources> CanvasRenderResources;

	FDungeonCanvasViewportTransform CachedViewTransform;
};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UDungeonCanvasMaterialThemeBakerComponent : public UActorComponent {
	GENERATED_BODY()
public:
	UDungeonCanvasMaterialThemeBakerComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon Canvas")
	UCanvasRenderTarget2D* GetRenderedTexture() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon Canvas")
	UMaterialInstanceDynamic* GetRenderedMaterial() const;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	FIntPoint TextureSize = FIntPoint(1024, 1024);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	FDungeonCanvasDrawSettings DrawSettings;

	/** Display the rendered canvas theme texture is using this material. Expects a texture parameter "RenderTexture" inside the material */   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UMaterialInterface> DisplayMaterialTemplate;

	/** Override the theme of the canvas.  Leave blank to pick up the theme specified in the dungeon canvas actor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UDungeonCanvasMaterialTheme> CanvasThemeOverride;

	/** Control where we focus on the dungeon layout using different types of cameras */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category="Dungeon Canvas")
	TObjectPtr<UDungeonCanvasCamera> Camera = nullptr;;
	
	/**
	 * The dungeon actor to use for rendering the dungeon layout.
	 * Leave blank if you want to automatically pick up the first one on the scene
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	TObjectPtr<ADungeon> Dungeon = nullptr;;
	
private:
	UPROPERTY()
	TObjectPtr<UDungeonCanvasMaterialThemeBaker> ThemeBaker;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DisplayMaterialInstance;
};


UCLASS(hidecategories=(Input), ConversionRoot, ComponentWrapperClass)
class DUNGEONARCHITECTRUNTIME_API ADungeonCanvasMaterialThemeBakerActor : public AActor {
	GENERATED_BODY()

public:
	ADungeonCanvasMaterialThemeBakerActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDungeonCanvasMaterialThemeBakerComponent> ThemeBakerComponent;


#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif // WITH_EDITORONLY_DATA
};

