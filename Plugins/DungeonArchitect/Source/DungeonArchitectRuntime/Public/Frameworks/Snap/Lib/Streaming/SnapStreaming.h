//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionComponent.h"
#include "SnapStreaming.generated.h"

class USnapStreamingChunkBase;
class UDungeonStreamingActorData;
class ASnapConnectionActor;
struct FSnapConnectionInstance;

DECLARE_DELEGATE_OneParam(FSnapChunkEvent, USnapStreamingChunkBase*);
UCLASS()
class DUNGEONARCHITECTRUNTIME_API USnapStreamingChunkBase : public UDungeonStreamingChunk {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    FTransform ModuleTransform;

public:
    //// UDungeonStreamingChunk Interface ////
    virtual void HandleChunkVisible() override;
    virtual void HandleChunkLoaded() override;
    virtual void HandleChunkUnloaded() override;
    virtual void HandleChunkHidden() override;
    virtual void DestroyChunk(UWorld* InWorld) override;
    //// End of UDungeonStreamingChunk Interface ////

private:
    void UpdateChunkDoorStates(ULevel* PersistentLevel) const;
    void HideChunkDoorActors() const;
    void DestroyChunkDoorActors(); 
    void Internal_SpawnChunkConnections(const FGuid& ChunkID, TArray<FSnapConnectionInstance>& RegisteredConnections,
            const TArray<ASnapConnectionActor*>& ChunkConnectionActors, ULevel* DoorLevel, ULevel* WallLevel) const;


protected:
    virtual void OnConnectionDoorCreated(FSnapConnectionInstance* ConnectionData) const {}
    virtual void UpdateConnectionDoorType(const FSnapConnectionInstance* ConnectionData, USnapConnectionComponent* ConnectionComponent) const;

    TArray<FSnapConnectionInstance>* GetAllConnections() const;
    
private:
    UPROPERTY()
    TObjectPtr<UDungeonStreamingActorData> SerializedData;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API USnapMapStreamingChunk : public USnapStreamingChunkBase {
    GENERATED_BODY()
public:
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API USnapGridFlowStreamingChunk : public USnapStreamingChunkBase {
    GENERATED_BODY()
public:
    virtual void OnConnectionDoorCreated(FSnapConnectionInstance* ConnectionData) const override;
    virtual void UpdateConnectionDoorType(const FSnapConnectionInstance* ConnectionData, USnapConnectionComponent* ConnectionComponent) const override;

public:
    UPROPERTY()
    bool bPlaceableMarkersSpawned{};
};


namespace SnapLib {
    typedef TSharedPtr<struct FModuleNode> FModuleNodePtr;
    typedef TSharedPtr<struct FModuleDoor> FModuleDoorPtr;
};

class DUNGEONARCHITECTRUNTIME_API FSnapStreamingLib {
public:
    static void GenerateLevelStreamingModel(UWorld* World, const TArray<SnapLib::FModuleNodePtr>& InModuleNodes,
            UDungeonLevelStreamingModel* LevelStreamingModel, TSubclassOf<UDungeonStreamingChunk> ChunkClass,
            TFunction<void(UDungeonStreamingChunk*, SnapLib::FModuleNodePtr)> FnInitChunk);
};

