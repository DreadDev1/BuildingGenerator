//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"

#include "Engine/DataAsset.h"
#include "DungeonCanvasEditorSettings.generated.h"

/**
 * Enumerates background for the texture editor view port.
 */
UENUM()
enum class EDungeonCanvasEditorBackgrounds : int8
{
	SolidColor UMETA(DisplayName="Solid Color"),
	Checkered UMETA(DisplayName="Checkered"),
	CheckeredFill UMETA(DisplayName="Checkered (Fill)")
};

UENUM()
enum class EDungeonCanvasEditorSampling : int8
{
	Default UMETA(DisplayName = "Default Sampling"),
	Point UMETA(DisplayName = "Nearest-Point Sampling"),
};

class UMaterialFunctionMaterialLayerBlend;
class UMaterialFunctionMaterialLayer;
enum class EDungeonCanvasViewportZoomMode : uint8;

USTRUCT()
struct DUNGEONARCHITECTEDITOR_API FDungeonCanvasEditorMaterialLayerPreset {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	FText Title;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	FText Tooltip;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon Canvas")
	TSoftObjectPtr<UMaterialFunctionMaterialLayer> MaterialLayer;
	
	UPROPERTY(EditDefaultsOnly, Category="Dungeon Canvas")
	TSoftObjectPtr<UMaterialFunctionMaterialLayerBlend> MaterialBlend;
};

USTRUCT()
struct DUNGEONARCHITECTEDITOR_API FDungeonCanvasEditorBuilderDefaults {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Snap Grid Flow")
	TSoftObjectPtr<class USnapGridFlowAsset> SgfGraph;
	
	UPROPERTY(EditAnywhere, Category="Snap Grid Flow")
	TSoftObjectPtr<class USnapGridFlowModuleDatabase> SgfModDB;
	
	UPROPERTY(EditAnywhere, Category="Snap Map")
	TSoftObjectPtr<class USnapMapAsset> SnapMapGraph;

	UPROPERTY(EditAnywhere, Category="Snap Map")
	TSoftObjectPtr<class USnapMapModuleDatabase> SnapMapModDB;
	
	UPROPERTY(EditAnywhere, Category="Grid Flow")
	TSoftObjectPtr<class UGridFlowAsset> GridFlowGraph;

	UPROPERTY(EditAnywhere, Category="Cell Flow")
	TSoftObjectPtr<class UCellFlowAsset> CellFlowGraph;
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonCanvasEditorDefaults : public UDataAsset {
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	TSoftObjectPtr<UMaterialFunctionMaterialLayerBlend> DefaultLayerBlend;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	TArray<FDungeonCanvasEditorMaterialLayerPreset> MaterialLayerPresets;

	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	FDungeonCanvasEditorBuilderDefaults BuilderDefaultAssets;
};


UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonCanvasMaterialThemePresets : public UDataAsset {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	TArray<FDungeonCanvasOverlayIcon> OverlayIcons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	FDungeonCanvasOverlayInternalIcon StairIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Settings")
	TSoftObjectPtr<UMaterialInterface> MaterialTemplateCanvas;
	
	UPROPERTY(EditAnywhere, Category="Settings")
	TSoftObjectPtr<UMaterialInterface> FogOfWarMaterialTemplate;
};


UCLASS(config=EditorPerProjectUserSettings)
class DUNGEONARCHITECTEDITOR_API UDungeonCanvasEditorSettings : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	
	/** The type of background to draw in the texture editor view port. */
	UPROPERTY(config)
	EDungeonCanvasEditorBackgrounds Background;

	/** The texture sampling mode used to render textures in the texture editor view port. */
	UPROPERTY(config)
	EDungeonCanvasEditorSampling Sampling;

	/** Background and foreground color used by Texture preview view ports. */
	UPROPERTY(config, EditAnywhere, Category=Background)
	FColor BackgroundColor;

	/** The first color of the checkered background. */
	UPROPERTY(config, EditAnywhere, Category=Background)
	FColor CheckerColorOne;

	/** The second color of the checkered background. */
	UPROPERTY(config, EditAnywhere, Category=Background)
	FColor CheckerColorTwo;

	/** The size of the checkered background tiles. */
	UPROPERTY(config, EditAnywhere, Category=Background, meta=(ClampMin="2", ClampMax="4096"))
	int32 CheckerSize;
	
	/** Whether the texture should scale to fit or fill the view port, or use a custom zoom level. */
	UPROPERTY(config)
	EDungeonCanvasViewportZoomMode ZoomMode;

	/** Color to use for the texture border, if enabled. */
	UPROPERTY(config, EditAnywhere, Category=TextureBorder)
	FColor TextureBorderColor;

	/** If true, displays a border around the texture. */
	UPROPERTY(config)
	bool TextureBorderEnabled;
};

