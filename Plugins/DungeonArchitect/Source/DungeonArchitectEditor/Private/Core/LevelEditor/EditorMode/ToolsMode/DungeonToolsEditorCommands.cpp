//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorCommands.h"

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeSettings.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeStyles.h"

#include "Styling/ISlateStyle.h"

#define LOCTEXT_NAMESPACE "DungeonToolsEditorCommands"

FDungeonToolsEditorCommands::FDungeonToolsEditorCommands() :
	TCommands<FDungeonToolsEditorCommands>(
		"DungeonToolsManagerCommands", // Context name for fast lookup
		NSLOCTEXT("Contexts", "DungeonToolCommands", "Dungeon Architect Mode - Tools"), // Localized context name for displaying
		NAME_None, // Parent
		FDungeonToolsEditorModeStyles::Get()->GetStyleSetName() // Icon Style Set
		)
{
}

TSharedPtr<FUICommandInfo> FDungeonToolsEditorCommands::FindToolByName(FString Name, bool& bFound) const
{
	bFound = false;
	for (const FStartToolCommand& Command : RegisteredTools)
	{
		if (Command.ToolUIName.Equals(Name, ESearchCase::IgnoreCase)
		 || (Command.ToolCommand.IsValid() && Command.ToolCommand->GetLabel().ToString().Equals(Name, ESearchCase::IgnoreCase)))
		{
			bFound = true;
			return Command.ToolCommand;
		}
	}
	return TSharedPtr<FUICommandInfo>();
}


void FDungeonToolsEditorCommands::RegisterCommands()
{
	const UDungeonToolsEditorModeSettings* Settings = GetDefault<UDungeonToolsEditorModeSettings>();

	// this has to be done with a compile-time macro because UI_COMMAND expands to LOCTEXT macros
#define REGISTER_MODELING_TOOL_COMMAND(ToolCommandInfo, ToolName, ToolTip) \
		UI_COMMAND(ToolCommandInfo, ToolName, ToolTip, EUserInterfaceActionType::ToggleButton, FInputChord()); \
		RegisteredTools.Add(FStartToolCommand{ ToolName, ToolCommandInfo });

	// this has to be done with a compile-time macro because UI_COMMAND expands to LOCTEXT macros
#define REGISTER_MODELING_ACTION_COMMAND(ToolCommandInfo, ToolName, ToolTip) \
		UI_COMMAND(ToolCommandInfo, ToolName, ToolTip, EUserInterfaceActionType::Button, FInputChord()); \
		RegisteredTools.Add(FStartToolCommand{ ToolName, ToolCommandInfo });

	// Layout Segment Tools
	REGISTER_MODELING_TOOL_COMMAND(BeginLayoutSegmentManageTool, "Manage Layout Segments", "Create and manage layout segments");
	
	// Grid tools
	REGISTER_MODELING_TOOL_COMMAND(BeginGridPaintTool, "Grid Paint", "Paint dungeon cells");
	
	// Snap tools
	REGISTER_MODELING_TOOL_COMMAND(BeginSnapStitchModuleTool, "Module Stitch", "Snap and stitch together modules");
	REGISTER_MODELING_TOOL_COMMAND(BeginSnapConnectionTool, "Module Connection", "Change the connection between modules to create doors or walls");
	
	// Flow tools
	REGISTER_MODELING_TOOL_COMMAND(BeginFlowNodeTool, "Flow Graph Tool", "Modify the flow graph nodes and links to manually control the layout");


	
	UI_COMMAND(AddToFavorites, "Add to Favorites", "Add to Favorites", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemoveFromFavorites, "Remove from Favorites", "Remove from Favorites", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(LoadFavoritesTools, "Faves", "Favorites", EUserInterfaceActionType::RadioButton, FInputChord());
	
	UI_COMMAND(LoadLayoutTools, "Layout", "Layout Segment Tools", EUserInterfaceActionType::RadioButton, FInputChord());
	
	UI_COMMAND(LoadGridTools, "Grid", "Grid Builder tools", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(LoadSnapTools, "Snap", "Snap Builder tools", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(LoadFlowTools, "Flow", "Flow Graph tools", EUserInterfaceActionType::RadioButton, FInputChord());

	UI_COMMAND(AcceptActiveTool, "Accept", "Accept the active Tool", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CancelActiveTool, "Cancel", "Cancel the active Tool", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CompleteActiveTool, "Done", "Complete the active Tool", EUserInterfaceActionType::Button, FInputChord());

	// Note that passing a chord into one of these calls hooks the key press to the respective action. 
	UI_COMMAND(AcceptOrCompleteActiveTool, "Accept or Complete", "Accept or Complete the active Tool", EUserInterfaceActionType::Button, FInputChord(EKeys::Enter));
	UI_COMMAND(CancelOrCompleteActiveTool, "Cancel or Complete", "Cancel or Complete the active Tool", EUserInterfaceActionType::Button, FInputChord(EKeys::Escape));
	
#undef REGISTER_MODELING_TOOL_COMMAND
}


TSharedPtr<FUICommandInfo> FDungeonToolsEditorCommands::RegisterExtensionPaletteCommand(
	FName Name,
	const FText& Label,
	const FText& Tooltip,
	const FSlateIcon& Icon)
{
	if (IsRegistered())
	{
		TSharedPtr<FDungeonToolsEditorCommands> Commands = GetInstance().Pin();

		for (FDynamicExtensionCommand& ExtensionCommand : Commands->ExtensionPaletteCommands)
		{
			if (ExtensionCommand.RegistrationName == Name)
			{
				return ExtensionCommand.Command;
			}
		}

		TSharedPtr<FUICommandInfo> NewCommandInfo;

		FUICommandInfo::MakeCommandInfo(
			Commands->AsShared(),
			NewCommandInfo,
			Name, Label, Tooltip, Icon,
			EUserInterfaceActionType::RadioButton,
			FInputChord() );

		FDynamicExtensionCommand NewExtensionCommand;
		NewExtensionCommand.RegistrationName = Name;
		NewExtensionCommand.Command = NewCommandInfo;
		Commands->ExtensionPaletteCommands.Add(NewExtensionCommand);
		return NewCommandInfo;
	};

	return TSharedPtr<FUICommandInfo>();
}



#undef LOCTEXT_NAMESPACE

