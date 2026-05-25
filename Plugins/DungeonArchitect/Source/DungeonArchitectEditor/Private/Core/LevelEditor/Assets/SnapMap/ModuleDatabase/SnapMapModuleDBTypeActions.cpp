//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/SnapMap/ModuleDatabase/SnapMapModuleDBTypeActions.h"

#include "Core/Common/ContentBrowserMenuExtensions.h"
#include "DungeonArchitectEditorModule.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FSnapMapModuleDBTypeActions

FText FSnapMapModuleDBTypeActions::GetName() const {
    return LOCTEXT("SnapMapModuleDBTypeActionsName", "Snap Map - Module Database");
}

FColor FSnapMapModuleDBTypeActions::GetTypeColor() const {
    return FColor(64, 128, 255);
}

UClass* FSnapMapModuleDBTypeActions::GetSupportedClass() const {
    return USnapMapModuleDatabase::StaticClass();
}

uint32 FSnapMapModuleDBTypeActions::GetCategories() {
    return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

const TArray<FText>& FSnapMapModuleDBTypeActions::GetSubMenus() const {
    static const TArray<FText> SubMenus = {
        FDAContentBrowserSubMenuNames::SnapMap
    };
    return SubMenus;
}

#undef LOCTEXT_NAMESPACE

