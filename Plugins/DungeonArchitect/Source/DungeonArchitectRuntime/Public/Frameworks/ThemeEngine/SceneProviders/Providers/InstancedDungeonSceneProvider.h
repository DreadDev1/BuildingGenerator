//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/Providers/PooledDungeonSceneProvider.h"

class DUNGEONARCHITECTRUNTIME_API FInstancedDungeonSceneProvider : public FDungeonSceneProvider {
public:
    virtual TSharedPtr<IDungeonSceneProviderSpawnStrategy> CreateSpawnStrategy() override;
};

