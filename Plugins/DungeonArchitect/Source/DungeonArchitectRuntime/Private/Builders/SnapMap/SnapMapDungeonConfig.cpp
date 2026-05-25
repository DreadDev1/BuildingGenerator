//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SnapMap/SnapMapDungeonConfig.h"


DEFINE_LOG_CATEGORY(SnapMapDungeonConfigLog);


USnapMapDungeonConfig::USnapMapDungeonConfig(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
      , CollisionTestContraction(100) {
}

