//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBuilders.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBase.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeMarkerNodeTool.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeSelectTool.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeVisualNodeTool.h"

#include "InteractiveToolManager.h"

////////////////// UDungeonThemeEdModeToolBuilderBase //////////////////
bool UDungeonThemeEdModeToolBuilderBase::CanBuildTool(const FToolBuilderState& SceneState) const {
	return true;
}

UInteractiveTool* UDungeonThemeEdModeToolBuilderBase::BuildTool(const FToolBuilderState& SceneState) const {
	UDungeonThemeEdModeToolBase* NewTool = NewObject<UDungeonThemeEdModeToolBase>(SceneState.ToolManager, ToolClass);
	NewTool->SetWorld(SceneState.World);
	NewTool->SetEdMode(EdMode);
	return NewTool;
}

void UDungeonThemeEdModeToolBuilderBase::SetEdMode(UDungeonThemeEdMode* InEdMode) {
	EdMode = InEdMode;
}

////////////////// Tool Builder Implementations //////////////////

//////////////////// Visual Tool Builder ////////////////////
UDungeonThemeEdModeVisualNodeToolBuilder::UDungeonThemeEdModeVisualNodeToolBuilder() {
	ToolClass = UDungeonThemeEdModeVisualNodeTool::StaticClass();
}

//////////////////// Select Tool Builder ////////////////////
UDungeonThemeEdModeSelectToolBuilder::UDungeonThemeEdModeSelectToolBuilder() {
	ToolClass = UDungeonThemeEdModeSelectTool::StaticClass();
}

//////////////////// Marker Tool Builder ////////////////////
UDungeonThemeEdModeMarkerNodeToolBuilder::UDungeonThemeEdModeMarkerNodeToolBuilder() {
	ToolClass = UDungeonThemeEdModeMarkerNodeTool::StaticClass();
}

UInteractiveTool* UDungeonThemeEdModeMarkerNodeToolBuilder::BuildTool(const FToolBuilderState& SceneState) const {
	UInteractiveTool* Tool = Super::BuildTool(SceneState);
	if (UDungeonThemeEdModeMarkerNodeTool* NodeEdTool = Cast<UDungeonThemeEdModeMarkerNodeTool>(Tool)) {
		NodeEdTool->SetMarkerNode(ThemeMarkerNode.Get());
	}
	return Tool;
}

void UDungeonThemeEdModeMarkerNodeToolBuilder::SetMarkerNode(UEdGraphNode_DungeonThemeMarker* InMarkerNode) {
	ThemeMarkerNode = InMarkerNode;
}

