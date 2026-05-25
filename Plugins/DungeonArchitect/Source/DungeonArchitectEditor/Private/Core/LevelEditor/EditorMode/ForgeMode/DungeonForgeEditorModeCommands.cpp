//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeCommands.h"

#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorModeCommands"

namespace DA::DungeonForge::CategoryNames
{
	const FName CommonTools = FName(TEXT("CommonTools"));
	const FName GridTools = FName(TEXT("GridTools"));
	const FName TextureTools = FName(TEXT("TextureTools"));
}

FDungeonForgeEditorModeCommands::FDungeonForgeEditorModeCommands()
	: TCommands<FDungeonForgeEditorModeCommands>("DungeonForgeEditorMode",
		NSLOCTEXT("DungeonForgeEditorMode", "DungeonForgeEditorModeCommands", "Dungeon Forge Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FDungeonForgeEditorModeCommands::RegisterCommands()
{
	// Add the common tools
	{
		UI_COMMAND(CommonToolsCategory, "Common Tools", "Common Tools", EUserInterfaceActionType::ToggleButton, FInputChord())
		Categories.Add(DA::DungeonForge::CategoryNames::CommonTools, CommonToolsCategory);

		TArray <TSharedPtr<FUICommandInfo>>& CommonToolCommands = Commands.FindOrAdd(DA::DungeonForge::CategoryNames::CommonTools);
		UI_COMMAND(TransformTool, "Transform Tool", "Transform the object in the scene", EUserInterfaceActionType::ToggleButton, FInputChord());
		CommonToolCommands.Add(TransformTool);
	}
	
	// Add the grid tools
	{
		UI_COMMAND(GridToolsCategory, "Grid Tools", "Grid Drawing tools", EUserInterfaceActionType::ToggleButton, FInputChord())
		Categories.Add(DA::DungeonForge::CategoryNames::GridTools, GridToolsCategory);
	
	}

	// Add the texture tools
	{
		UI_COMMAND(TextureToolsCategory, "Texture Tools", "Texture Drawing tools", EUserInterfaceActionType::ToggleButton, FInputChord())
		Categories.Add(DA::DungeonForge::CategoryNames::TextureTools, TextureToolsCategory);
	
		TArray <TSharedPtr<FUICommandInfo>>& TextureToolCommands = Commands.FindOrAdd(DA::DungeonForge::CategoryNames::TextureTools);
		UI_COMMAND(TexturePaintTool, "Texture Paint Tool", "Paint the texture layout on a texture", EUserInterfaceActionType::ToggleButton, FInputChord());
		TextureToolCommands.Add(TexturePaintTool);
	}

	// Layer creation commands
	{
		UI_COMMAND(CreateLayerGrid2D, "Grid 2D Layer", "Add a new 2D Grid layer to paint the layout on", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(CreateLayerGrid3D, "Grid 3D Layer", "Add a new 3D Grid layer to paint the layout on", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(CreateLayerTexture, "Texture Layer", "Add a new Texture layer to paint the layout on", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(CreateLayerFolder, "Folder", "Create a folder to organize the hierarchy", EUserInterfaceActionType::Button, FInputChord());
	}
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FDungeonForgeEditorModeCommands::GetCommands()
{
	return FDungeonForgeEditorModeCommands::Get().Commands;
}

TMap<FName, TSharedPtr<FUICommandInfo>> FDungeonForgeEditorModeCommands::GetCategories() {
	return FDungeonForgeEditorModeCommands::Get().Categories;
}

#undef LOCTEXT_NAMESPACE

