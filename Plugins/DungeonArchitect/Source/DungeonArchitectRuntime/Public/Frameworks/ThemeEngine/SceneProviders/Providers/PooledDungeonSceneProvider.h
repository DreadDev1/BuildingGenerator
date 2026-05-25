//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProvider.h"

DECLARE_LOG_CATEGORY_EXTERN(PooledDungeonSceneProvider, Log, All);

class DUNGEONARCHITECTRUNTIME_API FPooledDungeonSceneProvider : public FDungeonSceneProvider {
public:
    virtual TSharedPtr<IDungeonSceneProviderSpawnStrategy> CreateSpawnStrategy() override;
};
