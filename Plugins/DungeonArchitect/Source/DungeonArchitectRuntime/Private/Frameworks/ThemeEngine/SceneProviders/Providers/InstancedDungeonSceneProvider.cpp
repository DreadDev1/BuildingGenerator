//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Providers/InstancedDungeonSceneProvider.h"

#include "Frameworks/ThemeEngine/SceneProviders/Spawners/InstancedDungeonSceneProviderSpawnStrategy.h"

TSharedPtr<IDungeonSceneProviderSpawnStrategy> FInstancedDungeonSceneProvider::CreateSpawnStrategy() {
    return MakeShared<FInstancedDungeonSceneProviderSpawnStrategy>();
}

