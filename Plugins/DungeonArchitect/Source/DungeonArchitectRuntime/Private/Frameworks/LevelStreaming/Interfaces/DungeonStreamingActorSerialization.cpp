//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/LevelStreaming/Interfaces/DungeonStreamingActorSerialization.h"

#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonStreamingData, Log, All);

void UDungeonStreamingActorData::SaveLevel(ULevel* InLevel) {
    if (!InLevel) return;

    ActorEntries.Empty();

    int32 NumActorsSaved = 0;
    for (AActor* Actor : InLevel->Actors) {

        if (Actor && Actor->Implements<UDungeonStreamingActorSerialization>()) {
            FDungeonStreamingActorDataEntry& Entry = ActorEntries.AddDefaulted_GetRef();
            SaveActor(Actor, Entry);
            NumActorsSaved++;
        }
    }

    //UE_LOG(LogDungeonStreamingData, Log, TEXT("Saved %d actors"), NumActorsSaved);

    Modify();
}

void UDungeonStreamingActorData::LoadLevel(ULevel* InLevel) {
    if (!InLevel) return;

    // Build a map for faster access
    TMap<FName, const FDungeonStreamingActorDataEntry*> ActorToEntryMap;
    for (const FDungeonStreamingActorDataEntry& Entry : ActorEntries) {
        FName Key = *Entry.ActorName;
        if (!ActorToEntryMap.Contains(Key)) {
            ActorToEntryMap.Add(Key, &Entry);
        }
    }

    int32 NumActorsLoaded = 0;
    for (AActor* Actor : InLevel->Actors) {
        if (Actor && Actor->Implements<UDungeonStreamingActorSerialization>()) {
            FName Key = Actor->GetFName();
            const FDungeonStreamingActorDataEntry** SearchResult = ActorToEntryMap.Find(Key);
            if (SearchResult) {
                const FDungeonStreamingActorDataEntry* EntryPtr = *SearchResult;
                if (EntryPtr->ActorClass == Actor->GetClass()) {
                    LoadActor(Actor, *EntryPtr);
                    NumActorsLoaded++;
                }

                // TODO: Handle deletion / spawning of actors not in the list
            }
        }
    }

    //UE_LOG(LogDungeonStreamingData, Log, TEXT("Deserialized %d actors"), NumActorsLoaded);
}

struct FSnapMapSaveChunkArchive : public FObjectAndNameAsStringProxyArchive {
    FSnapMapSaveChunkArchive(FArchive& InInnerArchive)
        : FObjectAndNameAsStringProxyArchive(InInnerArchive, true) {
        ArIsSaveGame = true;
    }
};

void UDungeonStreamingActorData::SaveActor(AActor* InActor, FDungeonStreamingActorDataEntry& OutEntry) {
    OutEntry.ActorName = InActor->GetName();
    OutEntry.ActorClass = InActor->GetClass();
    OutEntry.ActorTransform = InActor->GetActorTransform();

    FMemoryWriter MemoryWriter(OutEntry.ActorData, true);
    FSnapMapSaveChunkArchive Ar(MemoryWriter);

    InActor->Serialize(Ar);
}

void UDungeonStreamingActorData::LoadActor(AActor* InActor, const FDungeonStreamingActorDataEntry& InEntry) {
    if (!InActor) return;
    check(InActor->GetName() == InEntry.ActorName);
    check(InActor->GetClass() == InEntry.ActorClass);
    if (InActor->GetRootComponent() && InActor->GetRootComponent()->Mobility == EComponentMobility::Movable) {
        InActor->SetActorTransform(InEntry.ActorTransform);
    }

    FMemoryReader Reader(InEntry.ActorData, true);
    FSnapMapSaveChunkArchive Ar(Reader);
    InActor->Serialize(Ar);

    IDungeonStreamingActorSerialization::Execute_OnSerializedDataLoaded(InActor);
}

