//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProvider.h"

class DUNGEONARCHITECTRUNTIME_API FSnapThemeSceneProvider : public FDungeonSceneProvider {
public:
    virtual void OnDungeonBuildStop(ADungeon* InDungeon, const FGuid& InChunkTag) override;
    
    TArray<TWeakObjectPtr<AActor>> GetSpawnedActors() const { return SpawnedActors; }
    virtual TSharedPtr<IDungeonSceneProviderSpawnStrategy> CreateSpawnStrategy() override;
    
private:
    void OnActorSpawned(AActor* InActor);
    
private:
    TWeakObjectPtr<UWorld> WorldPtr;
    TArray<TWeakObjectPtr<AActor>> SpawnedActors;
};
