//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/Customizations/DungeonActorEditorCustomization.h"

class FExtender;
class FDungeonCanvasMaterialThemeEditor;
/**
 * Handles all of the toolbar related construction for the render grid blueprint editor.
 */
class FDungeonCanvasBlueprintEditorToolbar : public TSharedFromThis<FDungeonCanvasBlueprintEditorToolbar>
{
public:
	FDungeonCanvasBlueprintEditorToolbar();

	/** Adds the mode-switch UI to the editor. */
	void AddDungeonCanvasBlueprintEditorModesToolbar(TSharedPtr<FExtender> Extender);

	//void AddListingModeToolbar(UToolMenu* InMenu);
	void AddCanvasGraphModeToolbar(UToolMenu* InMenu);
	void OnBuilderChanged() const;
};

