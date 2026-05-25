//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/GridFlow/GridFlowConfig.h"


DEFINE_LOG_CATEGORY(GridFlowConfigLog);

UGridFlowConfig::UGridFlowConfig(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
}

FDungeonConfigGenericSettings UGridFlowConfig::GatherGenericSettings() {
    FDungeonConfigGenericSettings Settings;
    Settings.bIsGridBased = true;
    Settings.GridSize = GridSize;
    return Settings;
}
