//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/CanvasEditor/DungeonCanvasEditorCommands.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#define LOCTEXT_NAMESPACE "DungeonCanvasEditorCommands"

FDungeonCanvasEditorCommands::FDungeonCanvasEditorCommands()
	: TCommands<FDungeonCanvasEditorCommands>(
		TEXT("DungeonCanvasEditor"),
		NSLOCTEXT("Contexts", "DungeonCanvasEditor", "Dungeon Canvas Editor"),
		NAME_None,
		FDungeonArchitectStyle::GetStyleSetName()) {
	
}

void FDungeonCanvasEditorCommands::RegisterCommands() {
	UI_COMMAND(EditCanvasSettings, "Canvas Theme Settings", "Edit Canvas Theme Settings", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(EditDungeonSettings, "Preview Dungeon Settings", "Edit the Preview Dungeon Settings", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(Compile, "Compile", "Compile the theme material", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(RandomizeDungeon, "Randomize Dungeon", "Randomize the preview dungeon", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE

