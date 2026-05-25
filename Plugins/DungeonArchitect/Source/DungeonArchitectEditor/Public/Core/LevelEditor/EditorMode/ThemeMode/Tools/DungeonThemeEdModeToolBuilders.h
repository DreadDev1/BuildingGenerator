//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "InteractiveToolBuilder.h"
#include "DungeonThemeEdModeToolBuilders.generated.h"

class UEdGraphNode_DungeonThemeMarker;
class UDungeonThemeEdMode;
class UDungeonThemeEdModeToolBase;

UCLASS()
class UDungeonThemeEdModeToolBuilderBase : public UInteractiveToolBuilder {
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
	void SetEdMode(UDungeonThemeEdMode* InEdMode);

protected:
	TSubclassOf<UDungeonThemeEdModeToolBase> ToolClass;
	TWeakObjectPtr<UDungeonThemeEdMode> EdMode;
};


UCLASS()
class UDungeonThemeEdModeSelectToolBuilder  : public UDungeonThemeEdModeToolBuilderBase
{
	GENERATED_BODY()
public:
	UDungeonThemeEdModeSelectToolBuilder();
};

UCLASS()
class UDungeonThemeEdModeVisualNodeToolBuilder  : public UDungeonThemeEdModeToolBuilderBase
{
	GENERATED_BODY()
public:
	UDungeonThemeEdModeVisualNodeToolBuilder();
};

UCLASS()
class UDungeonThemeEdModeMarkerNodeToolBuilder  : public UDungeonThemeEdModeToolBuilderBase
{
	GENERATED_BODY()
public:
	UDungeonThemeEdModeMarkerNodeToolBuilder();
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
	void SetMarkerNode(UEdGraphNode_DungeonThemeMarker* InMarkerNode);

private:
	TWeakObjectPtr<UEdGraphNode_DungeonThemeMarker> ThemeMarkerNode;
};
