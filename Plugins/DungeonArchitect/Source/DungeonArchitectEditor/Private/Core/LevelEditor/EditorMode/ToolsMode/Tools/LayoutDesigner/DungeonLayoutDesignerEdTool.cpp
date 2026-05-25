//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/Tools/LayoutDesigner/DungeonLayoutDesignerEdTool.h"

#include "Engine/World.h"
#include "InteractiveToolManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonLayoutDesigner, Log, All);

//////////////////////////////// UDungeonLayoutDesignerEdToolBuilder ////////////////////////////////
bool UDungeonLayoutDesignerEdToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const {
	return ToolClass != nullptr;
}

UInteractiveTool* UDungeonLayoutDesignerEdToolBuilder::BuildTool(const FToolBuilderState& SceneState) const {
	if (ToolClass) {
		if (UDungeonLayoutDesignerEdToolBase* NewTool = NewObject<UDungeonLayoutDesignerEdToolBase>(SceneState.ToolManager, ToolClass)) {
			NewTool->SetWorld(SceneState.World);
			return NewTool;
		}
	}
	return nullptr;
}

void UDungeonLayoutDesignerEdToolBase::Setup() {
	Super::Setup();

	if (PropertyClass) {
		Properties = NewObject<UDungeonLayoutDesignerEdToolPropertiesBase>(this, PropertyClass);
		AddToolPropertySource(Properties);
	}
}

void UDungeonLayoutDesignerEdToolBase::SetWorld(UWorld* InWorld) {
	TargetWorld = InWorld;
}


//////////////////////////////// UDungeonLayoutDesignerEdTool ////////////////////////////////
UDungeonLayoutDesignerEdTool::UDungeonLayoutDesignerEdTool() {
	PropertyClass = UDungeonLayoutDesignerEdToolProperties::StaticClass();
}


void UDungeonLayoutDesignerEdToolProperties::CreateNewSegment() {
	UE_LOG(LogDungeonLayoutDesigner, Log, TEXT("Create New Segment clicked"));
}

