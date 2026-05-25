//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SnapMap/SnapMapDungeonModel.h"

#include "Builders/SnapMap/SnapMapDungeonConfig.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"

USnapMapDungeonModel::USnapMapDungeonModel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , MissionGraph(nullptr)
{
}

void USnapMapDungeonModel::Reset() {
    Super::Reset();
    Connections.Reset();
    Walls.Reset();
    ModuleInstances.Reset();
    WorldMarkers.Reset();
    MissionGraph = nullptr;
}

bool USnapMapDungeonModel::SearchModuleInstance(const FGuid& InNodeId, FSnapModuleInstanceSerializedData& OutModuleData) {
    for (const FSnapModuleInstanceSerializedData& ModuleData : ModuleInstances) {
        if (ModuleData.ModuleInstanceId == InNodeId) {
            OutModuleData = ModuleData;
            return true;
        }
    }
    return false;
}

FDungeonFloorSettings USnapMapDungeonModel::CreateFloorSettings(const UDungeonConfig* InConfig) const {
    FDungeonFloorSettings Settings = USnapDungeonModelBase::CreateFloorSettings(InConfig);
    if (const USnapMapDungeonConfig* SnapMapConfig = Cast<USnapMapDungeonConfig>(InConfig)) {
        if (const USnapMapModuleDatabase* ModDB = SnapMapConfig->ModuleDatabase.LoadSynchronous()) {
            Settings.FloorHeight = ModDB->HintFloorHeight;
            Settings.FloorCaptureHeight = ModDB->HintFloorCaptureHeight;
            Settings.GlobalHeightOffset = -ModDB->HintGroundThickness;
        }
    }
    
    GetFloorIndexRange(Settings.FloorHeight, Settings.GlobalHeightOffset, Settings.FloorLowestIndex, Settings.FloorHighestIndex);

    return Settings;
}

