//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonShapes.h"

#include "Engine/LevelStreamingDynamic.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "DungeonLevelStreamingModel.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLevelStreamingModel, Log, All);

class ADungeon;
class ALevelInstance;
class ULevelStreaming;
class UDungeonLevelStreamingModel;
enum class EDungeonLevelStreamUnloadMethod : uint8;

UCLASS()
class ADungeonStreamingChunkLevelInstance : public ALevelInstance {
    GENERATED_BODY()
public:
    ADungeonStreamingChunkLevelInstance();
    virtual void OnLevelInstanceLoaded() override;
    virtual const FGuid& GetLevelInstanceGuid() const override { return NetworkGuid; }
    virtual bool IsNameStableForNetworking() const override { return true; }
    
    void RegisterStreamingCallbacks();
    void UnregisterStreamingCallbacks() const;

    TWeakObjectPtr<ULevelStreamingDynamic> GetCachedLevelStreaming() const { return LevelStreaming; }

    bool SetWorldAssetImpl(const TSoftObjectPtr<UWorld>& NewWorldAsset);
    virtual void SetActorHiddenInGame(bool bNewHidden) override;

    void SetNetworkGuid(const FGuid& InGuid);

    
private:
    bool SetWorldAssetImplRuntime(const TSoftObjectPtr<UWorld>& NewWorldAsset);
    void CacheOriginalHiddenStates();

public:
    UPROPERTY()
    TWeakObjectPtr<UDungeonStreamingChunk> Chunk;

    UPROPERTY()
    FGuid NetworkGuid;
    
private:
    UPROPERTY()
    TWeakObjectPtr<ULevelStreamingDynamic> LevelStreaming;

    UPROPERTY()
    TMap<uint32, bool> ChildActorHiddenStates;
};

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonStreamingChunk : public UObject {
    GENERATED_UCLASS_BODY()
public:

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FGuid ID;

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FBox Bounds;

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FDAShapeList BoundShapes;
    
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    TSet<TObjectPtr<UDungeonStreamingChunk>> Neighbors;

    // TODO: use data Layers to separate always-resident actors (PlayerStart, spawn platform) from streamable geometry. This would allow spawn room geometry to stream in/out while keeping spawn functionality always available. check UDataLayerAsset

    /**
     * When true, this chunk is always loaded and never unloaded regardless of player position.
     * Used for spawn rooms to prevent race conditions during player spawn.
     */
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    bool bAlwaysResident = false;
    

public:
    UFUNCTION(BlueprintCallable, Category=Dungeon)
    TArray<AActor*> GetLoadedChunkActors();
    
    UFUNCTION(BlueprintCallable, Category=Dungeon)
    TArray<AActor*> GetLoadedChunkActorsOfType(const UClass* ActorClass);
    
    UFUNCTION(BlueprintCallable, Category=Dungeon)
    AActor* GetLoadedChunkActorOfType(const UClass* ActorClass);
    
private:
    UPROPERTY()
    bool bShouldBeVisible = false;

private:
    UPROPERTY()
    TWeakObjectPtr<ADungeonStreamingChunkLevelInstance> LevelInstanceActor;
    
    UPROPERTY(Transient)
    TObjectPtr<UPackage> LevelPackage = nullptr;;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ChunkListeners;

    TArray<TWeakObjectPtr<AActor>> ManagedActors;
    
private:
    bool bIsVisible = false;
    bool bIsLoaded = false;
    FRandomStream FallbackRandomStream;

public:
    void CreateLevelStreaming(UObject* WorldContextObject, TSoftObjectPtr<UWorld> Level, const FGuid& InstanceId, const FTransform& Transform, bool& bOutSuccess);
    void SetStreamingLevelState(bool bInVisible, EDungeonLevelStreamUnloadMethod UnloadMethod);
    bool RequiresStateUpdate(bool bInVisible) const;
    void RegisterManagedActor(TWeakObjectPtr<AActor> InActor);
    virtual void DestroyChunk(UWorld* InWorld);
    
    UPackage* GetLevelPackage() const;
    ULevel* GetLoadedLevel() const;
    ULevelStreamingDynamic* GetStreamingLevel() const;
    FTransform GetLevelTransform() const;
    bool IsLevelVisible() const { return bIsVisible; }

    UFUNCTION()
    virtual void HandleChunkVisible();

    UFUNCTION()
    virtual void HandleChunkHidden();
    
    UFUNCTION()
    virtual void HandleChunkLoaded();

    UFUNCTION()
    virtual void HandleChunkUnloaded();

private:
    void NotifyEventListener_OnVisible();
    void NotifyEventListener_OnHidden();
    void NotifyEventListener_OnLoaded();
    void NotifyEventListener_OnUnloaded();

    ADungeon* GetOwningDungeon() const;
    FRandomStream& GetRandomStream(ADungeon* InDungeon);

protected:
    ADungeon* GetDungeon() const;
    UDungeonLevelStreamingModel* GetLevelStreamingModel() const;
};

class UDungeonLevelStreamingNavigation;
enum class EDungeonLevelStreamChunkSelection : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDALevelStreamerBindableEvent, ADungeon*, Dungeon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDALevelStreamerStateChangeDelegate, ADungeon*, Dungeon, UDungeonStreamingChunk*, Chunk);

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonLevelStreamingModel : public UObject {
    GENERATED_UCLASS_BODY()

public:

    UPROPERTY()
    TObjectPtr<UDungeonLevelStreamingNavigation> StreamingNavigation;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerBindableEvent OnInitialChunksLoaded;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkLoaded;
    
    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkUnloaded;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkVisible;
    
    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkHidden;

public:
    void Initialize(UWorld* InWorld);
    void Release(UWorld* InWorld);
    bool HasNotifiedInitialChunkLoadEvent() const { return bNotifiedInitialChunkLoadEvent; }
    void GetStreamingSourceLocations(UWorld* InWorld, EDungeonLevelStreamChunkSelection InChunkSelection, TArray<FVector>& OutSourceLocations) const;
    
    void NotifyInitialChunksLoaded();
    void NotifyChunkLoaded(UDungeonStreamingChunk* InChunk);
    void NotifyChunkUnloaded(UDungeonStreamingChunk* InChunk);
    void NotifyChunkVisible(UDungeonStreamingChunk* InChunk);
    void NotifyChunkHidden(UDungeonStreamingChunk* InChunk);

    template<typename T>
    T* CreateChunk() {
        return Cast<T>(CreateChunk(T::StaticClass()));
    }
    UDungeonStreamingChunk* CreateChunk(TSubclassOf<UDungeonStreamingChunk> ChunkClass) {
        UDungeonStreamingChunk* Chunk = NewObject<UDungeonStreamingChunk>(this, ChunkClass);
        Chunks.Add(Chunk);
        return Chunk;
    }
    
    const TArray<UDungeonStreamingChunk*>& GetChunks() const { return Chunks; }
    
protected:
    UPROPERTY()
    TArray<TObjectPtr<UDungeonStreamingChunk>> Chunks;
    
private:
    bool bNotifiedInitialChunkLoadEvent = false;
};


class DUNGEONARCHITECTRUNTIME_API FLevelStreamLoader {
public:
    static ULevelStreamingDynamic* LoadLevelInstanceBySoftObjectPtr(const UObject* WorldContextObject,
                                                                    const TSoftObjectPtr<UWorld>& Level, uint32 InstanceId,
                                                                    const FTransform& Transform, bool& bOutSuccess, UPackage*& OutLevelPackage);
private:
    // Counter used by LoadLevelInstance to create unique level names
    static ULevelStreamingDynamic* LoadLevelInstance_Internal(UWorld* World, const FString& LongPackageName,
                                                              uint32 InstanceId, const FTransform& Transform, UPackage*& OutLevelPackage);
};

