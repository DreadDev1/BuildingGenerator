//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/GridDungeonConfig.h"

#include "Builders/Grid/Stair/GridDungeonStairGeneratorLegacy.h"
#include "Builders/Grid/Stair/GridDungeonStairGeneratorV2.h"

DEFINE_LOG_CATEGORY(GridDungeonConfigLog);

UGridDungeonConfig::UGridDungeonConfig(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
      , NumCells(100)
      , GridCellSize(FVector(400, 400, 200))
      , MinCellSize(2)
      , MaxCellSize(5)
      , RoomAreaThreshold(15)
      , RoomAspectDelta(0.4f)
      , SpanningTreeLoopProbability(0.15f)
      , DoorProximitySteps(6)
      , NormalMean(0)
      , NormalStd(0.3f)
      , LaneWidth(2)
      , bEnableClusteredTheming(false)
      , bClusterWithHeightVariation(false)
      , InitialRoomRadius(15)
      , DungeonWidth(10000)
      , DungeonLength(10000)
{
	StairGenerator = CreateDefaultSubobject<UGridDungeonBuilderStairGeneratorV2>("StairGenerator");
}

void UGridDungeonConfig::PostLoad() {
	Super::PostLoad();

	if (!StairGenerator) {
		StairGenerator = NewObject<UGridDungeonBuilderStairGeneratorLegacy>(this, "StairGeneratorOverride");
		Modify();
	}
}

FDungeonConfigGenericSettings UGridDungeonConfig::GatherGenericSettings() {
	FDungeonConfigGenericSettings Settings;
	Settings.bIsGridBased = true;
	Settings.GridSize = GridCellSize;
	return Settings;
}

