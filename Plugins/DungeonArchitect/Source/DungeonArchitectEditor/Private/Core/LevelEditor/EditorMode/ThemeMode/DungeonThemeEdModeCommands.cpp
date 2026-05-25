//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeCommands.h"

#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "DungeonThemeEdModeCommands"

FDungeonThemeEdModeCommands::FDungeonThemeEdModeCommands()
	: TCommands<FDungeonThemeEdModeCommands>("DungeonThemeEdMode",
		LOCTEXT("DungeonThemeEdModeCommands", "Dungeon Theme Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FDungeonThemeEdModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(SelectTool, "Select", "The default Select tool", EUserInterfaceActionType::Button, FInputChord());
	ToolCommands.Add(SelectTool);
	
	UI_COMMAND(VisualNodeTool, "Visual Node", "Modify the actors associated with the visual theme node", EUserInterfaceActionType::Button, FInputChord());
	ToolCommands.Add(VisualNodeTool);

	UI_COMMAND(MarkerNodeTool, "Marker Node", "Drop drop assets in the view to attach them to the selected marker nodes", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(MarkerNodeTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FDungeonThemeEdModeCommands::GetCommands()
{
	return FDungeonThemeEdModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE

