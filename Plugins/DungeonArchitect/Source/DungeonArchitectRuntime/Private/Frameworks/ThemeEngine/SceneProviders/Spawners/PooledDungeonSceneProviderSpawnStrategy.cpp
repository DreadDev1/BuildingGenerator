//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Spawners/PooledDungeonSceneProviderSpawnStrategy.h"

#include "Core/Actors/DungeonMesh.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandCore.h"
#include "Frameworks/ThemeEngine/SceneProviders/Utils/DungeonSceneProviderLib.h"

#include "GameFramework/Actor.h"
#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("DungeonPool"), STATGROUP_DungeonPool, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Node Actor Search Time"), STAT_NodeActorSearch, STATGROUP_DungeonPool);

/////////////////////////////// FPooledDungeonSceneProviderSpawnStrategy ///////////////////////////////
void FPooledDungeonSceneProviderSpawnStrategy::Initialize(FDungeonSceneProviderCommandList& InCommandList, ADungeon* InDungeon, const FGuid& InChunkTag) {
    CacheReusableActorPool(InCommandList, InDungeon, InChunkTag);
}

void FPooledDungeonSceneProviderSpawnStrategy::Finalize(FDungeonSceneProviderCommandList& InCommandList, ADungeon* InDungeon, const FGuid& InChunkTag) {
    DestroyUnusedActorsFromPool(InCommandList);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UDungeonMesh* Mesh) {
    if (Mesh->StaticMesh == nullptr) return;


    TSharedPtr<FDungeonSceneProviderCommand> Command;

    // Check if we have a free actor of this type in the pool. If so use it.
    if (AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform)) {
        Command = MakeShareable(new FDungeonSceneProviderCommand_ReuseStaticMesh(Context, Actor, Mesh));
    }
    else {
        // No free actor exists.  Create a new actor later in the game thread
        Command = MakeShareable(new FDungeonSceneProviderCommand_CreateMesh(Context, Mesh));
    }
    CommandList.Add(Command);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddStaticMesh(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UStaticMesh* Mesh, bool bCanAffectNavigation) {
    if (!Mesh) return;

    TSharedPtr<FDungeonSceneProviderCommand> Command;

    // Check if we have a free actor of this type in the pool. If so use it.
    if (AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform)) {
        Command = MakeShareable(new FDungeonSceneProviderCommand_ReuseStaticMeshSimple(Context, Actor, Mesh, bCanAffectNavigation));
    }
    else {
        // No free actor exists.  Create a new actor later in the game thread
        Command = MakeShareable(new FDungeonSceneProviderCommand_CreateMeshSimple(Context, Mesh, bCanAffectNavigation));
    }
    CommandList.Add(Command);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddLight(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UPointLightComponent* LightTemplate) {
    // Check if we have a free actor of this type in the pool. If so use it.
    TSharedPtr<FDungeonSceneProviderCommand> Command;
    if (AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform)) {
        Command = MakeShareable(new FDungeonSceneProviderCommand_ReuseLight(Context, Actor, LightTemplate));
    }
    else {
        // No free actor exists.  Create a new actor later in the game thread
        Command = MakeShareable(new FDungeonSceneProviderCommand_AddLight(Context, LightTemplate));
    }
    CommandList.Add(Command);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddParticleSystem(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList, UParticleSystem* ParticleTemplate) {
    // Check if we have a free actor of this type in the pool. If so use it.
    TSharedPtr<FDungeonSceneProviderCommand> Command;
    if (AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform)) {
        Command = MakeShareable(
            new FDungeonSceneProviderCommand_ReuseParticleSystem(Context, Actor, ParticleTemplate));
    }
    else {
        // No free actor exists.  Create a new actor later in the game thread
        Command = MakeShareable(new FDungeonSceneProviderCommand_AddParticleSystem(Context, ParticleTemplate));
    }
    CommandList.Add(Command);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddActorFromTemplate(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList,
                                                                    UClass* ClassTemplate, bool bCanAffectNavigation) {
    // Check if we have a free actor of this type in the pool. If so use it.
    TSharedPtr<FDungeonSceneProviderCommand> Command;
    if (AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform)) {
        Command = MakeShareable(new FDungeonSceneProviderCommand_ReuseActorTemplate(Context, Actor, ClassTemplate, bCanAffectNavigation));
    }
    else {
        Command = MakeShareable(new FDungeonSceneProviderCommand_AddActor(Context, ClassTemplate, bCanAffectNavigation));
    }
    CommandList.Add(Command);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddClonedActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList,
                                                              UDungeonActorTemplate* ActorTemplate) {
    // Check if we have a free actor of this type in the pool. If so use it.
    TSharedPtr<FDungeonSceneProviderCommand> Command;
    if (AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform)) {
        Command = MakeShareable(new FDungeonSceneProviderCommand_ReuseClonedActor(Context, Actor, ActorTemplate));
    }
    else {
        Command = MakeShareable(new FDungeonSceneProviderCommand_CloneActor(Context, ActorTemplate));
    }
    CommandList.Add(Command);
}

void FPooledDungeonSceneProviderSpawnStrategy::AddGroupActor(const FDungeonSceneProviderContext& Context, FDungeonSceneProviderCommandList& CommandList,
                                                             const TArray<FName>& ActorNodeIds) {
    // Check if we have a free actor of this type in the pool. If so use it.
    AActor* Actor = ReuseFromPool(Context.NodeId, Context.MarkerIndex, Context.Transform);
    if (!Actor) {
        TSharedPtr<FDungeonSceneProviderCommand> Command = MakeShareable(new FDungeonSceneProviderCommand_CreateGroupActor(Context, ActorNodeIds));
        CommandList.Add(Command);
    }
}

void FPooledDungeonSceneProviderSpawnStrategy::CacheReusableActorPool(FDungeonSceneProviderCommandList& CommandList, const ADungeon* InDungeon, const FGuid& InChunkTag) {
    if (UWorld* World = InDungeon ? InDungeon->GetWorld() : nullptr) {
        NodeActorPool.Reset();
        const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(InDungeon);

        // Collect all actors that have a "Dungeon" tag
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
            AActor* Actor = *ActorItr;
            if (!IsValid(Actor) || !Actor->ActorHasTag(DungeonTag)) {
                continue;
            }

            if (InChunkTag.IsValid() && !Actor->ActorHasTag(FName(InChunkTag.ToString()))) {
                // Belongs to the dungeon, but not to the chunk
                continue;
            }

            FGuid NodeId;
            if (!FDungeonSceneProviderLib::GetNodeId(Actor, DungeonTag, NodeId)) {
                // This actor does not belong to the dungeon
                continue;
            }

            if (Actor->Tags.Contains(FDungeonSceneProviderCommand::TagComplexActor)) {
                // We don't want to reuse a complex actor.  Instead we will create new ones and these old ones
                TSharedPtr<FDungeonSceneProviderCommand> Command = MakeShareable(new FDungeonSceneProviderCommand_DestroyActor(Actor));
                Command->ExecutionPriority = MAX_int32;
                CommandList.Add(Command);
                continue;
            }
            
            int32 MarkerIndex =  INDEX_NONE;
            if (UDungeonSpawnDataComponent* SpawnData = Actor->FindComponentByClass<UDungeonSpawnDataComponent>()) {
                MarkerIndex = SpawnData->MarkerIndex;
            }
            
            TArray<FReusableItem>& ActorList = NodeActorPool.FindOrAdd(NodeId);
            ActorList.Add({ Actor, MarkerIndex });
        }
    }
}

void FPooledDungeonSceneProviderSpawnStrategy::DestroyUnusedActorsFromPool(FDungeonSceneProviderCommandList& CommandList) {
    // Destroy every unused object in the actor pool, since the dungeon building has ended
    TArray<TArray<FReusableItem>> ActorValues;
    NodeActorPool.GenerateValueArray(ActorValues);
    for (const TArray<FReusableItem>& ActorArray : ActorValues) { 
        for (const FReusableItem& Item : ActorArray) {
            if (Item.WeakActor.IsValid()) {
                TSharedPtr<FDungeonSceneProviderCommand> Command = MakeShareable(new FDungeonSceneProviderCommand_DestroyActor(Item.WeakActor.Get()));
                CommandList.Add(Command);
            }
        }
    }

    NodeActorPool.Reset();
}


AActor* FPooledDungeonSceneProviderSpawnStrategy::ReuseFromPool(const FGuid& NodeId, int32 InMarkerIndex, const FTransform& InTransform) {
    // Check if we have a free actor of this type in the pool
    if (NodeActorPool.Contains(NodeId)) {
        TArray<FReusableItem>& NodeActorList = NodeActorPool[NodeId];
        if (NodeActorList.Num() > 0) {
            // A free actor is available in the pool.
            
            // Try to find an actor that shares the same marker index
            for (int i = 0; i < NodeActorList.Num(); i++) {
                TWeakObjectPtr<AActor> Actor = NodeActorList[i].WeakActor;
                if (Actor.IsValid()) {
                    if (NodeActorList[i].MarkerIndex == InMarkerIndex) {
                        NodeActorList.RemoveAtSwap(i);
                        return Actor.Get();
                    }
                }
            }
            
            // Next, try to find an actor that shares the same transform
            for (int i = 0; i < NodeActorList.Num(); i++) {
                TWeakObjectPtr<AActor> Actor = NodeActorList[i].WeakActor;
                if (Actor.IsValid() && Actor->GetTransform().Equals(InTransform)) {
                    NodeActorList.RemoveAtSwap(i);
                    return Actor.Get();
                }
            }

            // We did not find a free actor that has the requested transform or the marker index
            // Grab the first free actor and remove it from the pool
            for (int i = 0; i < NodeActorList.Num(); i++) {
                TWeakObjectPtr<AActor> Actor = NodeActorList[i].WeakActor;
                if (Actor.IsValid()) {
                    NodeActorList.RemoveAtSwap(i);
                    return Actor.Get();
                }
            }
        }
    }

    // No free objects of this type in the pool
    return nullptr;
}

