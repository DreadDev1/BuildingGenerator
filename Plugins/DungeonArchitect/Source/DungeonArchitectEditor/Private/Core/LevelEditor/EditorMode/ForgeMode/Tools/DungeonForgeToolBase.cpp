//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgeToolBase.h"

#include "Engine/World.h"
#include "InteractiveToolManager.h"

///////////////////////////// UDungeonForgeToolBase /////////////////////////////
void UDungeonForgeToolBase::SetWorld(UWorld* World) {
	check(World);
	this->TargetWorld = World;
}

void UDungeonForgeToolBase::SetEdMode(const TWeakObjectPtr<UDungeonForgeEditorMode>& InEdMode) {
	EdMode = InEdMode;
}

///////////////////////////// UDungeonForgeToolBuilderBase /////////////////////////////
bool UDungeonForgeToolBuilderBase::CanBuildTool(const FToolBuilderState& SceneState) const {
	return true;
}

UInteractiveTool* UDungeonForgeToolBuilderBase::BuildTool(const FToolBuilderState& SceneState) const {
	UDungeonForgeToolBase* NewTool = NewObject<UDungeonForgeToolBase>(SceneState.ToolManager, ToolClass);
	NewTool->SetWorld(SceneState.World);
	NewTool->SetEdMode(EdMode);
	return NewTool;
}

void UDungeonForgeToolBuilderBase::SetEdMode(const TWeakObjectPtr<UDungeonForgeEditorMode>& InEdMode) {
	EdMode = InEdMode;
}

