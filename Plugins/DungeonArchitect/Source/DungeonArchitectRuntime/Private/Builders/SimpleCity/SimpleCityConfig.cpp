//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SimpleCity/SimpleCityConfig.h"


DEFINE_LOG_CATEGORY(SimpleCityConfigLog);

USimpleCityConfig::USimpleCityConfig(const FObjectInitializer& ObjectInitializer) :
    Super(ObjectInitializer)
    , CellSize(FVector2D(400, 400))
    , MinCitySize(15)
    , MaxCitySize(20)
    , MinBlockSize(2)
    , MaxBlockSize(4)
    , BiggerHouseProbability(0)
    , RoadEdgeRemovalProbability(0.1f) {
}

FDungeonConfigGenericSettings USimpleCityConfig::GatherGenericSettings() {
    FDungeonConfigGenericSettings Settings;
    Settings.bIsGridBased = true;
    Settings.GridSize = FVector(CellSize.X, CellSize.Y, CellSize.X);
    return Settings;
}
