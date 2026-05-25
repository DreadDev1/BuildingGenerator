//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/Theming/SnapTheme.h"

#include "Frameworks/ThemeEngine/SceneProviders/Spawners/DefaultDungeonSceneProviderSpawnStrategy.h"

void FSnapThemeSceneProvider::OnDungeonBuildStop(ADungeon* InDungeon, const FGuid& InChunkTag) {
    FDungeonSceneProvider::OnDungeonBuildStop(InDungeon, InChunkTag);
    
    for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : CommandList.GetMutableCommands()) {
        Command->GetOnActorSpawned().BindRaw(this, &FSnapThemeSceneProvider::OnActorSpawned);
    }
}

TSharedPtr<IDungeonSceneProviderSpawnStrategy> FSnapThemeSceneProvider::CreateSpawnStrategy() {
    return MakeShared<FDefaultDungeonSceneProviderSpawnStrategy>();
}

void FSnapThemeSceneProvider::OnActorSpawned(AActor* InActor) {
    SpawnedActors.Add(InActor);
}

