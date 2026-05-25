//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/Canvas/Themes/UDungeonCanvasMaterialThemeAssetTypeAction.h"

#include "Core/Common/ContentBrowserMenuExtensions.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasMaterialThemeEditor.h"
#include "DungeonArchitectEditorModule.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

#include "Frameworks/Canvas/DungeonCanvasDeprecated.h"

#define LOCTEXT_NAMESPACE "DungeonCanvasMaterialThemeAssetTypeAction"

//////////////////////// FCellFlowAssetTypeActions ////////////////////////
FText FDungeonCanvasMaterialThemeAssetTypeAction::GetName() const {
	return LOCTEXT("DungeonCanvasMaterialThemeAssetTypeActionsName", "Dungeon Canvas Material Theme");
}

FColor FDungeonCanvasMaterialThemeAssetTypeAction::GetTypeColor() const {
	return FColor::Purple;
}

UClass* FDungeonCanvasMaterialThemeAssetTypeAction::GetSupportedClass() const {
	return UDungeonCanvasMaterialTheme::StaticClass();
}

void FDungeonCanvasMaterialThemeAssetTypeAction::OpenAssetEditor(const TArray<UObject*>& InObjects,
													TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {
 	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
										? EToolkitMode::WorldCentric
										: EToolkitMode::Standalone;
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt) {
		if (UDungeonCanvasMaterialTheme* ThemeAsset = Cast<UDungeonCanvasMaterialTheme>(*ObjIt)) {
			const TSharedRef<FDungeonCanvasMaterialThemeEditor> NewCanvasEditor(new FDungeonCanvasMaterialThemeEditor());
			NewCanvasEditor->InitEditor(Mode, EditWithinLevelEditor, ThemeAsset);
		}
	}
}

uint32 FDungeonCanvasMaterialThemeAssetTypeAction::GetCategories() {
	return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

const TArray<FText>& FDungeonCanvasMaterialThemeAssetTypeAction::GetSubMenus() const {
	static const TArray<FText> SubMenus = {
		FDAContentBrowserSubMenuNames::Canvas
	};
	return SubMenus;
}

void FDungeonCanvasMaterialThemeAssetTypeAction::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) {
}

/////////////////////////// FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction ///////////////////////////
FText FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction::GetName() const {
	static const FText DisplayName = LOCTEXT("DungeonCanvasDeprecatedBP_Text", "Dungeon Canvas Blueprint (DEPRECATED)");
	return DisplayName;
}

FColor FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction::GetTypeColor() const {
	return FColor(64, 64, 64);
}

UClass* FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction::GetSupportedClass() const {
	return UDungeonCanvasBlueprint::StaticClass();
}

uint32 FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction::GetCategories() {
	return EAssetTypeCategories::None;
}

void FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {
	FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);
}

const FSlateBrush* FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction::GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const {
	return FDungeonArchitectStyle::Get().GetBrush("DungeonArchitect.Icons.Error");
}


#undef LOCTEXT_NAMESPACE

