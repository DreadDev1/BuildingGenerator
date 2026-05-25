//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/Canvas/Themes/DungeonCanvasMaterialThemeFactory.h"

#include "Core/Editors/CanvasEditor/DungeonCanvasEditorUtilities.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

UDungeonCanvasMaterialThemeFactory::UDungeonCanvasMaterialThemeFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	SupportedClass = UDungeonCanvasMaterialTheme::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonCanvasMaterialThemeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	UDungeonCanvasMaterialTheme* NewAsset = NewObject<UDungeonCanvasMaterialTheme>(InParent, Class, Name, Flags | RF_Transactional);
	FDungeonCanvasEditorUtilities::InitializeThemeAsset(NewAsset);
	NewAsset->Version = static_cast<int>(EDungeonCanvasMaterialThemeVersion::LatestVersion);
	return NewAsset;
}

bool UDungeonCanvasMaterialThemeFactory::CanCreateNew() const {
	return true;
}

