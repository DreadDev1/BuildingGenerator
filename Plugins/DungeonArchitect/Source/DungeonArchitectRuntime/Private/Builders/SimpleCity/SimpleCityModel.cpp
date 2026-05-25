//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SimpleCity/SimpleCityModel.h"


void USimpleCityModel::Reset() {
	Super::Reset();
    CityWidth = 0;
    CityLength = 0;
    Cells.Reset();
}

FDungeonFloorSettings USimpleCityModel::CreateFloorSettings(const UDungeonConfig* InConfig) const {
    constexpr FDungeonFloorSettings Settings = {};
    return Settings;
}

