//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/DungeonArchitectCommands.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#define LOCTEXT_NAMESPACE "DungeonArchitectCommands"

FDungeonArchitectCommands::FDungeonArchitectCommands() : TCommands<FDungeonArchitectCommands>(
    TEXT("DungeonArchitect"),
    NSLOCTEXT("DungeonArchitect", "DungeonArchitect", "Dungeon Architect"),
    NAME_None,
    FDungeonArchitectStyle::GetStyleSetName()) {
}

void FDungeonArchitectCommands::RegisterCommands() {
	UI_COMMAND(OpenSupportForums, "Support Forum", "Go to the support forums and interact with the developers and the community", EUserInterfaceActionType::Button, FInputChord(EKeys::F1));
	UI_COMMAND(OpenSupportDiscord, "Discord", "Join our discord community and interact with the developers and the community", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenDocumentationWebsite, "Documentation", "Open the documentation website for Dungeon Architect", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(ModePaint, "Paint", "Paint", EUserInterfaceActionType::ToggleButton, FInputChord());
    UI_COMMAND(ModeRectangle, "Rectangle", "Rectangle", EUserInterfaceActionType::ToggleButton, FInputChord());
    UI_COMMAND(ModeBorder, "Border", "Border", EUserInterfaceActionType::ToggleButton, FInputChord());
    UI_COMMAND(ModeSelect, "Select", "Select", EUserInterfaceActionType::ToggleButton, FInputChord());

    UI_COMMAND(UpgradeThemeFile, "Upgrade Theme File", "Upgrade Theme File", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE

