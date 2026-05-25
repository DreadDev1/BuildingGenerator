//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/EditorImpl/AppModeEditorImplFactory.h"

#include "Builders/CellFlow/CellFlowBuilder.h"
#include "Builders/FloorPlan/FloorPlanBuilder.h"
#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/GridFlow/GridFlowBuilder.h"
#include "Builders/SimpleCity/SimpleCityBuilder.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/EditorImpl/GridAppModeEditor.h"

TSharedPtr<IMGAppModeEditorInterface> FMGAppModeEditorImplFactory::Create(const TSubclassOf<UDungeonBuilder>& InBuilderClass) {
	if (InBuilderClass == UGridDungeonBuilder::StaticClass()
			|| InBuilderClass == UGridFlowBuilder::StaticClass()
			|| InBuilderClass == UCellFlowBuilder::StaticClass()
			|| InBuilderClass == USimpleCityBuilder::StaticClass()
			|| InBuilderClass == UFloorPlanBuilder::StaticClass()) {
		return MakeShareable(new FMGGridAppModeEditor);
	}

	return nullptr;
}

