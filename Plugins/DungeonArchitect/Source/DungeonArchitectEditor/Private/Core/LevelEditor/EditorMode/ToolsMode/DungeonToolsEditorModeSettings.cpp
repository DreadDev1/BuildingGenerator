//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeSettings.h"


#define LOCTEXT_NAMESPACE "ModelingToolsEditorModeSettings"


FText UDungeonToolsEditorModeSettings::GetSectionText() const 
{ 
	return LOCTEXT("ModelingModeSettingsName", "Dungeon Mode"); 
}

FText UDungeonToolsEditorModeSettings::GetSectionDescription() const
{
	return LOCTEXT("ModelingModeSettingsDescription", "Configure the Dungeon Architect Tools Editor Mode plugin");
}


FText UDungeonToolsModeCustomizationSettings::GetSectionText() const 
{ 
	return LOCTEXT("ModelingModeSettingsName", "Dungeon Mode"); 
}

FText UDungeonToolsModeCustomizationSettings::GetSectionDescription() const
{
	return LOCTEXT("ModelingModeSettingsDescription", "Configure the Dungeon Architect Tools Editor Mode plugin");
}



#undef LOCTEXT_NAMESPACE

