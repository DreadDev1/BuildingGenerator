//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommands.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorCommands"

FDungeonForgeEditorCommands::FDungeonForgeEditorCommands()
	: ::TCommands<FDungeonForgeEditorCommands>(
		"DungeonForgeEditor",
		NSLOCTEXT("Contexts", "DungeonForgeEditor", "Dungeon Forge Editor"),
		NAME_None,
		FDungeonArchitectStyle::GetStyleSetName())
{
}

void FDungeonForgeEditorCommands::RegisterCommands() {
	UI_COMMAND(ToggleEnabled, "Toggle Enabled", "Toggle node enabled state for selected nodes.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::E));
	UI_COMMAND(ToggleDebug, "Toggle Debug", "Toggle node debug state for selected nodes", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::D));
}


#undef LOCTEXT_NAMESPACE

