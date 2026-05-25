//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Dungeon.h"

struct FRandomStream;
class ULevel;
class UDungeonSpawnLogic;
class IDungeonMarkerUserData;

struct DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderContext {
    FDungeonSceneProviderContext()
        : Transform(FTransform::Identity)
    {
    }

    FTransform Transform;
    FGuid NodeId;
    FGuid ChunkTag;
    const FRandomStream* RandomStream{};
    TArray<TWeakObjectPtr<UDungeonSpawnLogic>> SpawnLogics;
    TSharedPtr<IDungeonMarkerUserData> MarkerUserData;
    int32 MarkerIndex = INDEX_NONE;
    TWeakObjectPtr<ADungeon> Dungeon;
    TWeakObjectPtr<ULevel> LevelOverride;
    TArray<FName> ActorTags;
};

