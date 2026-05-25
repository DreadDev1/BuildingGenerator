//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

class UDungeonCanvasMaterialLayer;
class UMaterialInstanceConstant;

class FDungeonCanvasEditorUtilities {
public:
	static void CopyMaterialLayers(const UMaterialInstanceConstant* Source, UMaterialInstanceConstant* Destination);
	static void CompileDungeonCanvasMaterialTemplate(UDungeonCanvasMaterialTheme* CanvasThemeAsset);
	static void CompileDungeonCanvasMaterialTemplate(UMaterialInstanceConstant* MaterialInstance, const TArray<UDungeonCanvasMaterialLayer*>& MaterialLayers);

	static void SaveMaterialLayerParameters(const UDungeonCanvasMaterialTheme* CanvasThemeAsset);
	static void RestoreMaterialLayerParameters(const UDungeonCanvasMaterialTheme* CanvasThemeAsset);
	static void RestoreMaterialLayerParameters(UMaterialInstanceConstant* MaterialInstance, const TArray<UDungeonCanvasMaterialLayer*>& MaterialLayers);
	
	static void InitializeThemeAsset(UDungeonCanvasMaterialTheme* NewAsset);
};

