//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

class UObject;

DUNGEONARCHITECTGAMEPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogDAG, Log, All);
DUNGEONARCHITECTGAMEPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogDAGExperience, Log, All);
DUNGEONARCHITECTGAMEPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogDAGAbilitySystem, Log, All);
DUNGEONARCHITECTGAMEPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogDAGTeams, Log, All);

DUNGEONARCHITECTGAMEPLAY_API FString DAG_GetClientServerContextString(UObject* ContextObject = nullptr);

