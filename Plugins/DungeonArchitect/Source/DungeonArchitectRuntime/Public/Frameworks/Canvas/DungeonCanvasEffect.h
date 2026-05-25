//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonCanvasEffect.generated.h"

class UCanvasRenderTarget2D;
class UMaterialFunctionMaterialLayerInstance;

UCLASS(Abstract, HideDropdown, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasEffectBase : public UObject {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dungeon")
	void Initialize();
	virtual void Initialize_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dungeon")
	void Draw();
	virtual void Draw_Implementation();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dungeon")
	void Tick(float DeltaSeconds);
	virtual void Tick_Implementation(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dungeon")
	void InitCanvasMaterial(UMaterialInstanceDynamic* Material);
	virtual void InitCanvasMaterial_Implementation(UMaterialInstanceDynamic* Material);
	
	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	void SetTextures(UCanvasRenderTarget2D* TexLayoutFill, UCanvasRenderTarget2D* TexLayoutBorder, UCanvasRenderTarget2D* TexSDF, UCanvasRenderTarget2D* TexDynamicOcclusion);

	UPROPERTY(BlueprintReadOnly, Category="Dungeon")
	TWeakObjectPtr<UCanvasRenderTarget2D> LayoutFillTexture;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon")
	TWeakObjectPtr<UCanvasRenderTarget2D> LayoutBorderTexture;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon")
	TWeakObjectPtr<UCanvasRenderTarget2D> SDFTexture;
	
	UPROPERTY(BlueprintReadOnly, Category="Dungeon")
	TWeakObjectPtr<UCanvasRenderTarget2D> DynamicOcclusionTexture;
	
	UPROPERTY()
	TObjectPtr<UMaterialFunctionMaterialLayerInstance> MaterialLayerInstance;
};

