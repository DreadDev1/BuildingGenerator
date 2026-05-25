//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Providers/PooledDungeonSceneProvider.h"

#include "Frameworks/ThemeEngine/SceneProviders/Spawners/PooledDungeonSceneProviderSpawnStrategy.h"

DEFINE_LOG_CATEGORY(PooledDungeonSceneProvider);

/////////////////////////////// FPooledDungeonSceneProvider ///////////////////////////////
TSharedPtr<IDungeonSceneProviderSpawnStrategy> FPooledDungeonSceneProvider::CreateSpawnStrategy() {
    return MakeShared<FPooledDungeonSceneProviderSpawnStrategy>();
}
