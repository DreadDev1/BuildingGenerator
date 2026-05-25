//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Actors/DungeonMesh.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProviderContext.h"

#include "Components/PointLightComponent.h"

class UStaticMesh;
class UPointLightComponent;
class USpotLightComponent;
class UParticleSystem;
class UDungeonActorTemplate;
class IDungeonSceneProviderSpawnStrategy;
class ADungeonInstancedMeshActor;

DECLARE_LOG_CATEGORY_EXTERN(DungeonSceneProvider, Log, All);


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProvider {
public:
    virtual ~FDungeonSceneProvider() {}

    virtual void OnDungeonBuildStart(ADungeon* InDungeon, const FGuid& InChunkTag);
    virtual void OnDungeonBuildStop(ADungeon* InDungeon, const FGuid& InChunkTag);

    virtual TSharedPtr<IDungeonSceneProviderSpawnStrategy> CreateSpawnStrategy() = 0;

    void AddStaticMesh(const FDungeonSceneProviderContext& Context, UDungeonMesh* Mesh);
    void AddStaticMesh(const FDungeonSceneProviderContext& Context, UStaticMesh* Mesh, bool bCanAffectNavigation);
    void AddLight(const FDungeonSceneProviderContext& Context, UPointLightComponent* LightTemplate);
    void AddParticleSystem(const FDungeonSceneProviderContext& Context, UParticleSystem* ParticleTemplate);
    void AddActorFromTemplate(const FDungeonSceneProviderContext& Context, UClass* ClassTemplate, bool bCanAffectNavigation);
    void AddClonedActor(const FDungeonSceneProviderContext& Context, UDungeonActorTemplate* ActorTemplate);
    void AddGroupActor(const FDungeonSceneProviderContext& Context, const TArray<FName>& ActorNodeIds);
    void AddCustomCommand(TSharedPtr<FDungeonSceneProviderCommand> SceneCommand);
    virtual void ProcessUnsupportedObject(UObject* Object, const FDungeonSceneProviderContext& Context) {}

    // Run all the queued commands. This will be called from the game thread. The commands might have been inserted from the background thread
    void ExecuteAllCommands(UWorld* InWorld);
    void RunGameThreadCommands(UWorld* InWorld, float MaxBuildTimePerFrameMs);
    bool IsRunningGameThreadCommands() const;
    void SetBuildPriorityLocation(const FVector& Location);
    //void SetLevelOverride(ULevel* InLevel) { LevelOverride = InLevel; }

    FORCEINLINE int32 GetTotalWorkUnits() const { return TotalCommandCount; }
    FORCEINLINE int32 GetCompletedWorkUnits() const { return CompletedCommandCount; }
        
private:
    void ApplyExecutionWeights() const;

protected:
    // The location to start building from and spreading out from here, while building asynchronously
    FVector BuildPriorityLocation = FVector::ZeroVector;

    FDungeonSceneProviderCommandList CommandList;
    TSharedPtr<IDungeonSceneProviderSpawnStrategy> SpawnStrategy;

private:
    // Modern command tracking
    TArray<TSharedPtr<FDungeonSceneProviderCommand>> PendingCommands;
    TArray<TSharedPtr<FDungeonSceneProviderCommand>> ActiveCommands;
    int32 TotalCommandCount = 0;
    int32 CompletedCommandCount = 0;
};

