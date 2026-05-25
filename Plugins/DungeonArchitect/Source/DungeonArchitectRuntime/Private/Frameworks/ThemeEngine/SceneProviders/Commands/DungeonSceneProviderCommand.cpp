//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"

#include "Core/Actors/DungeonInstancedMeshActor.h"
#include "Core/Dungeon.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProviderContext.h"
#include "Frameworks/ThemeEngine/SceneProviders/Utils/DungeonSceneProviderLib.h"
#include "Frameworks/Voxel/Utils/VoxelLib.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

FName FDungeonSceneProviderCommand::TagComplexActor = "Dungeon-Complex-Actor";

void FDungeonSceneProviderCommandList::Add(const TSharedPtr<FDungeonSceneProviderCommand>& InCommand) {
    Commands.Add(InCommand);
}

void FDungeonSceneProviderCommandList::Insert(const TSharedPtr<FDungeonSceneProviderCommand>& InCommand, int32 Index) {
    Commands.Insert(InCommand, Index);
}

void FDungeonSceneProviderCommand::Execute(UWorld* World) {
    if (World) {
        ExecuteImpl(World);
        // For backward compatibility, single-frame commands are finished after ExecuteImpl
        // Multi-frame commands should set bIsFinished = false in their ExecuteImpl
    }
}

void FDungeonSceneProviderCommand::PostInitializeActor(AActor* InActor, bool bRunSpawnLogic, bool bProcessUserData) {
    MoveToFolder(InActor, Context.Dungeon.Get());

    // Add the tags
    {
        const FGuid NodeGuid = InActor->IsA<ADungeonInstancedMeshActor>()
                ? FDungeonSceneProviderLib::StaticInstancedMeshNodeId
                : Context.NodeId;
        
        FName NodeTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(NodeGuid);
        InActor->Tags.AddUnique(NodeTag);

        const FName DungeonIdTag = FDungeonUtils::GetDungeonIdTag(Context.Dungeon.Get());
        InActor->Tags.AddUnique(DungeonIdTag);

        if (Context.ChunkTag.IsValid()) {
            InActor->Tags.AddUnique(FName(Context.ChunkTag.ToString()));
        }

        for (const FName& ActorTag : Context.ActorTags) {
            InActor->Tags.AddUnique(ActorTag);
        } 
    }
    
    SetDungeonMetadata(InActor);

    if (bRunSpawnLogic) {
        const FRandomStream* RandomStream = Context.RandomStream ? Context.RandomStream : &FallbackRandomStream;
        ExecuteSpawnLogics(InActor, Context.Dungeon.Get(), *RandomStream, Context.SpawnLogics);
    }

    if (bProcessUserData) {
        if (Context.MarkerUserData.IsValid() && InActor) {
            if (UDungeonBuilder* DungeonBuilder = Context.Dungeon.IsValid() ? Context.Dungeon->GetBuilder() : nullptr) {
                DungeonBuilder->ProcessThemeItemUserData(Context.MarkerUserData, InActor);
            }
        }
    }

    OnActorSpawned.ExecuteIfBound(InActor);
}

void FDungeonSceneProviderCommand::ExecuteSpawnLogics(AActor* SpawnedActor, const ADungeon* InDungeon, const FRandomStream& InRandom,
                                              const TArray<TWeakObjectPtr<UDungeonSpawnLogic>>& SpawnLogics) {
    if (!SpawnedActor) return;
    for (TWeakObjectPtr<UDungeonSpawnLogic> SpawnLogic : SpawnLogics) {
        if (!SpawnLogic.IsValid()) continue;
        SpawnLogic->SetExecState(SpawnedActor->GetWorld(), &InRandom);
        SpawnLogic->OnItemSpawn(SpawnedActor, InDungeon);
        SpawnLogic->ClearExecState();
    }
}

void FDungeonSceneProviderCommand::MoveToFolder(AActor* ActorToMove, const ADungeon* InDungeon) {
#if WITH_EDITOR
    if (ActorToMove && InDungeon) {
        ActorToMove->SetFolderPath(InDungeon->ItemFolderPath);
    }
#endif
}

void FDungeonSceneProviderCommand::SetDungeonMetadata(AActor* InActor) const {
    if (InActor) {
        UDungeonSpawnDataComponent* SpawnData = InActor->FindComponentByClass<UDungeonSpawnDataComponent>();
        if (!SpawnData) {
            SpawnData = NewObject<UDungeonSpawnDataComponent>(InActor);
            SpawnData->RegisterComponent();
        }

        SpawnData->OwningDungeon = Context.Dungeon;
        SpawnData->ThemeNodeId = InActor->IsA<ADungeonInstancedMeshActor>()
                ? FDungeonSceneProviderLib::StaticInstancedMeshNodeId
                : Context.NodeId;

        SpawnData->ChunkId = Context.ChunkTag;
        SpawnData->MarkerIndex = Context.MarkerIndex;
        SpawnData->Modify();
    }
}

void FDungeonSceneProviderCommand::UpdateExecutionPriorityByDistance(const FVector& BuildPosition, const FTransform& CommandTransform) {
    UpdateExecutionPriorityByDistance(BuildPosition, CommandTransform.GetLocation());
}

void FDungeonSceneProviderCommand::UpdateExecutionPriorityByDistance(const FVector& BuildPosition, const FVector& InContextLocation) {
    ExecutionPriority = FMath::RoundToInt((InContextLocation - BuildPosition).Size());
}

void FDungeonSceneProviderCommand::TagAsComplexObject(AActor* Actor) {
    Actor->Tags.Add(TagComplexActor);
}

