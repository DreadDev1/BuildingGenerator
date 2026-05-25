//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"
#include "DungeonCanvasMaterialTheme.generated.h"

enum class EDungeonCanvasMaterialThemeVersion {
	InitialVersion = 0,

	//----------- Versions should be placed above this line -----------------
	LastVersionPlusOne,
	LatestVersion = LastVersionPlusOne - 1
};

class UMaterialInterface;
class UMaterialInstanceConstant;
class UDungeonCanvasEffectBase;
class UDungeonCanvasMaterialLayer;
class UDungeonCanvasMaterialThemeEditorProperties;


UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasMaterialTheme : public UObject {
	GENERATED_BODY()
public:
	UDungeonCanvasMaterialTheme();
	
	UPROPERTY()
	int32 Version = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	TArray<FDungeonCanvasOverlayIcon> OverlayIcons;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings", meta = (UIMin = "0.0", UIMax = "1.0"))
	float OverlayIconOpacity = 0.85f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	FDungeonCanvasOverlayInternalIcon StairIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Settings", meta=(EditInline))
	TArray<TObjectPtr<UDungeonCanvasEffectBase>> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Settings")
	TSoftObjectPtr<UMaterialInterface> MaterialTemplateCanvas;
	
	UPROPERTY(EditAnywhere, Category="Settings")
	TSoftObjectPtr<UMaterialInterface> FogOfWarMaterialTemplate;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceConstant> CompiledThemeMaterial = nullptr;;

	UPROPERTY()
	TArray<TObjectPtr<UDungeonCanvasMaterialLayer>> MaterialLayers;

	UPROPERTY()
	TObjectPtr<UDungeonCanvasMaterialThemeEditorProperties> PreviewDungeonProperties = nullptr;;
	
};

