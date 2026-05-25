//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Extenders/EditorUIExtender.h"

#include "Core/Editors/LaunchPad/LaunchPad.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "Framework/Commands/UICommandList.h"
#include "LevelEditor.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "EditorUIExtender"

void FEditorUIExtender::Extend() {
    if (!IsRunningCommandlet()) {
        UToolMenu* AssetsToolBar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.AssetsToolBar");
        if (AssetsToolBar) {
            FToolMenuSection& Section = AssetsToolBar->AddSection("Content");
            FToolMenuEntry LaunchPadEntry = FToolMenuEntry::InitToolBarButton("DA", FUIAction(FExecuteAction::CreateStatic(&FLaunchPadSystem::Launch)),        //FDALevelToolbarCommands::Get().OpenLaunchPad,
                                                                                    LOCTEXT("DAToolbarButtonText_1", "Dungeon Architect"),
                                                                                    LOCTEXT("DAToolbarButtonTooltip", "Dungeon Architect Launch Pad"),
                                                                                    FSlateIcon(FDungeonArchitectStyle::GetStyleSetName(), TEXT("DungeonArchitect.Toolbar.IconMain")));
            LaunchPadEntry.StyleNameOverride = "CalloutToolbar";
            Section.AddEntry(LaunchPadEntry);
        }
    }
}

void FEditorUIExtender::Release() {
    if (!IsRunningCommandlet()) {
        FLevelEditorModule* LevelEditorModule = FModuleManager::Get().GetModulePtr<FLevelEditorModule>("LevelEditor");
        if (LevelEditorModule) {
            if (LevelEditorModule->GetToolBarExtensibilityManager().IsValid()) {
                LevelEditorModule->GetToolBarExtensibilityManager().Get()->RemoveExtender(LevelToolbarExtender);
            }
        }
    }
}

FDALevelToolbarCommands::FDALevelToolbarCommands()
    : TCommands<FDALevelToolbarCommands>(
        TEXT("DAToolbar"),
        NSLOCTEXT("DAToolbar", "DAToolbar", "Dungeon Architect"),
        NAME_None,
        FDungeonArchitectStyle::GetStyleSetName()) {
}

void FDALevelToolbarCommands::RegisterCommands() {
    UI_COMMAND(OpenLaunchPad, "Launch Pad", "Browse samples and templates", EUserInterfaceActionType::Button,
               FInputChord());
    UI_COMMAND(OpenHelpWindow, "Help / Support", "Documentation and help for the Dungeon Architect plugin",
               EUserInterfaceActionType::Button, FInputChord());

    LevelMenuActionList = MakeShareable(new FUICommandList);

    BindCommands();
}

void FDALevelToolbarCommands::BindCommands() {
    LevelMenuActionList->MapAction(
        OpenLaunchPad,
        FExecuteAction::CreateStatic(&FLaunchPadSystem::Launch)
    );
}

#undef LOCTEXT_NAMESPACE

