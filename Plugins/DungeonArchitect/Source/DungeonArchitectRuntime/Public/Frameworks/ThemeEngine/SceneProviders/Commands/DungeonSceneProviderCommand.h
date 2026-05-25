//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Dungeon.h"
#include "Frameworks/ThemeEngine/Rules/Spawn/DungeonSpawnLogic.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProviderContext.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/Class.h"

DECLARE_DELEGATE_OneParam(FDungeonSceneProviderCommandActorEvent, AActor*);

class FDungeonSceneProviderCommand;

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommandList {
public:
    void Add(const TSharedPtr<FDungeonSceneProviderCommand>& InCommand);
    void Insert(const TSharedPtr<FDungeonSceneProviderCommand>& InCommand, int32 Index);
    const TArray<TSharedPtr<FDungeonSceneProviderCommand>>& GetCommands() const { return Commands; }
    TArray<TSharedPtr<FDungeonSceneProviderCommand>>& GetMutableCommands() { return Commands; }
    
private:
    TArray<TSharedPtr<FDungeonSceneProviderCommand>> Commands;
};


struct FDungeonSceneProviderCommandHints {
    bool bIsLatent = false;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand(const FDungeonSceneProviderContext& InContext) :
        ExecutionPriority(MAX_int32), Context(InContext), bIsFinished(true)
    {
    }

    virtual ~FDungeonSceneProviderCommand() {
        AdditionalEmittedCommands.Reset();
    }

    virtual void Execute(UWorld* World);
    
    // Called each frame for multi-frame commands that aren't finished
    virtual void Tick(UWorld* World, float DeltaTime) {}
    
    // Check if this command has completed execution
    virtual bool IsFinished() const { return bIsFinished; }

    static bool WeightSortPredicate(const TSharedPtr<FDungeonSceneProviderCommand>& CommandA, const TSharedPtr<FDungeonSceneProviderCommand>& CommandB) {
        return (CommandA->ExecutionPriority < CommandB->ExecutionPriority);
    }

    int32 ExecutionPriority;

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) {
    }

    virtual void PostInitializeActor(AActor* InActor, bool bRunSpawnLogic = true, bool bProcessUserData = true);

    // If tagged as a complex object, it will not reuse the object, instead destroy and recreate it
    static void TagAsComplexObject(AActor* Actor);
    static void MoveToFolder(AActor* ActorToMove, const ADungeon* InDungeon);

    bool HasAdditionalEmittedCommands() const { return AdditionalEmittedCommands.Num() > 0; }
    const TArray<TSharedPtr<FDungeonSceneProviderCommand>>& GetAdditionalEmittedCommands() const { return AdditionalEmittedCommands; }
    
    FDungeonSceneProviderContext GetContext() const { return Context; }
    FDungeonSceneProviderCommandActorEvent& GetOnActorSpawned() { return OnActorSpawned; }
    
    // If this tag is preset, we do not reuse this object, instead destroy and recreate it
    static FName TagComplexActor;
    const FDungeonSceneProviderCommandHints& GetHints() const { return Hints; }
    
protected:
    virtual void ExecuteImpl(UWorld* World) = 0;
    void SetDungeonMetadata(AActor* InActor) const;
    void UpdateExecutionPriorityByDistance(const FVector& BuildPosition, const FTransform& CommandTransform);
    void UpdateExecutionPriorityByDistance(const FVector& BuildPosition, const FVector& InContextLocation);

    template <typename T>
    T* AddActor(UWorld* World, ULevel* Level, const FTransform& transform, const FGuid& NodeId) {
        FActorSpawnParameters SpawnParams;
        SpawnParams.OverrideLevel = Level;
        T* Actor = World->SpawnActor<T>(T::StaticClass(), SpawnParams);
        Actor->SetActorTransform(transform);
        Actor->Tags.Add(FName("Dungeon"));
        PostInitializeActor(Actor);
        return Actor;
    }

protected:
    FDungeonSceneProviderContext Context;
    FDungeonSceneProviderCommandActorEvent OnActorSpawned;
    FRandomStream FallbackRandomStream;
    bool bIsFinished; 

    TArray<TSharedPtr<FDungeonSceneProviderCommand>> AdditionalEmittedCommands;
    FDungeonSceneProviderCommandHints Hints{};
    
private:
    static void ExecuteSpawnLogics(AActor* SpawnedActor, const ADungeon* InDungeon, const FRandomStream& InRandom, const TArray<TWeakObjectPtr<UDungeonSpawnLogic>>& SpawnLogics);

};
