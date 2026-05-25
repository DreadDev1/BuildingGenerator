//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/CellFlowDomainEdViewportCommands.h"

#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "SCellFlowDomainEdViewportCommands"

void FCellFlowDomainEdViewportCommands::RegisterCommands() {
	UI_COMMAND(ToggleDebugData, "Toggle Debug Data", "Shows the debug data of the dungeon",
			   EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisplayDungeonProperties, "Dungeon Properties", "Shows the dungeon properties",
			   EUserInterfaceActionType::Button, FInputChord());
}


#undef LOCTEXT_NAMESPACE

