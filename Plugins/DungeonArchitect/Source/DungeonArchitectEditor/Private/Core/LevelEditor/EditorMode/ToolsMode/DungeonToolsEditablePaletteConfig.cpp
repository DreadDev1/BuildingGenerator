//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditablePaletteConfig.h"


TObjectPtr<UDungeonToolsEditablePaletteConfig> UDungeonToolsEditablePaletteConfig::Instance = nullptr;

void UDungeonToolsEditablePaletteConfig::Initialize()
{
	if(!Instance)
	{
		Instance = NewObject<UDungeonToolsEditablePaletteConfig>(); 
		Instance->AddToRoot();
	}
}

FEditableToolPaletteSettings* UDungeonToolsEditablePaletteConfig::GetMutablePaletteConfig(const FName& InstanceName)
{
	if (InstanceName.IsNone())
	{
		return nullptr;
	}

	return &EditableToolPalettes.FindOrAdd(InstanceName);
}

const FEditableToolPaletteSettings* UDungeonToolsEditablePaletteConfig::GetConstPaletteConfig(const FName& InstanceName)
{
	if (InstanceName.IsNone())
	{
		return nullptr;
	}

	return EditableToolPalettes.Find(InstanceName);
}

void UDungeonToolsEditablePaletteConfig::SavePaletteConfig(const FName&)
{
	SaveEditorConfig();
}

