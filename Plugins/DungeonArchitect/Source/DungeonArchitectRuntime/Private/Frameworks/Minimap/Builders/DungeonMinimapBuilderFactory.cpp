//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Minimap/Builders/DungeonMinimapBuilderFactory.h"

#include "Builders/GridFlow/GridFlowBuilder.h"
#include "Frameworks/Minimap/Builders/DungeonMinimapBuilder.h"
#include "Frameworks/Minimap/Builders/Impl/GridFlowMinimapBuilder.h"

FDungeonMinimapBuilderPtr FDungeonMinimapBuilderFactory::Create(TSubclassOf<UDungeonBuilder> InBuilderClass) {
	if (InBuilderClass == UGridFlowBuilder::StaticClass()) {
		return MakeShared<FGridFlowMinimapBuilder>();
	}
	else {
		return MakeShared<FDungeonMinimapBuilder>();
	}
}

