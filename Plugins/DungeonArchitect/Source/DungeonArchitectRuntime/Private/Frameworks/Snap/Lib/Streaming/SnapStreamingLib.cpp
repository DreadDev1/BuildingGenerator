//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/Streaming/SnapStreaming.h"

#include "Builders/SnapGridFlow/SnapGridFlowDungeon.h"
#include "Builders/SnapMap/SnapMapDungeonModel.h"
#include "Core/Dungeon.h"
#include "Frameworks/GraphGrammar/Script/GrammarScriptGraph.h"
#include "Frameworks/LevelStreaming/Interfaces/DungeonStreamingActorSerialization.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"
#include "Frameworks/Snap/Lib/Serialization/SnapConnectionSerialization.h"
#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"
#include "Frameworks/Snap/Lib/SnapLibrary.h"

#include "Engine/Engine.h"
#include "Engine/Level.h"

///////////////////////////////////// USnapStreamingChunk /////////////////////////////////////

USnapStreamingChunkBase::USnapStreamingChunkBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SerializedData = ObjectInitializer.CreateDefaultSubobject<UDungeonStreamingActorData>(this, "SerializedData");
}

void USnapStreamingChunkBase::HandleChunkVisible() {
    ULevel* Level = GetLoadedLevel();

    // Serialize the level actor data
    if (Level) {
        SerializedData->LoadLevel(Level);
    }
    
    Super::HandleChunkVisible();

    ULevel* PersistentLevel = GetWorld() ? GetWorld()->PersistentLevel : nullptr;
    UpdateChunkDoorStates(PersistentLevel);
}

void USnapStreamingChunkBase::HandleChunkHidden() {
    Super::HandleChunkHidden();

    ULevel* Level = GetLoadedLevel();
    if (Level) {
        SerializedData->SaveLevel(Level);
    }

    HideChunkDoorActors();
}


void USnapStreamingChunkBase::HandleChunkLoaded() {
    Super::HandleChunkLoaded();

}

void USnapStreamingChunkBase::HandleChunkUnloaded() {
    Super::HandleChunkUnloaded();

    DestroyChunkDoorActors();
}

void USnapStreamingChunkBase::DestroyChunk(UWorld* InWorld) {
    Super::DestroyChunk(InWorld);
}

namespace {
    template <typename T>
    TArray<T*> GetActorsOfType(ULevel* Level) {
        TArray<T*> Result;
        if (Level) {
            for (int i = 0; i < Level->Actors.Num(); i++) {
                if (T* TargetActor = Cast<T>(Level->Actors[i])) {
                    Result.Add(TargetActor);
                }
            }
        }
        return Result;
    }
}

void USnapStreamingChunkBase::Internal_SpawnChunkConnections(const FGuid& ChunkID, TArray<FSnapConnectionInstance>& RegisteredConnections,
            const TArray<ASnapConnectionActor*>& ChunkConnectionActors, ULevel* DoorLevel, ULevel* WallLevel) const {

    if (ADungeon* Dungeon = GetDungeon()) {
        for (ASnapConnectionActor* ChunkConnectionActor : ChunkConnectionActors) {
            if (!ChunkConnectionActor || !ChunkConnectionActor->ConnectionComponent) {
                continue;
            }
            
            const FGuid ConnectionId = ChunkConnectionActor->GetConnectionId();
            FSnapConnectionInstance* ConnectionData{};
            FGuid ThisModuleId, OtherModuleId;
            bool bFlipDoorDirection{};
            for (FSnapConnectionInstance& ConnectionEntry : RegisteredConnections) {
                if (ConnectionEntry.ModuleA == ChunkID && ConnectionEntry.DoorA == ConnectionId) {
                    ConnectionData = &ConnectionEntry;
                    ThisModuleId = ConnectionEntry.ModuleA;
                    OtherModuleId = ConnectionEntry.ModuleB;
                    break;
                }
            
                if (ConnectionEntry.ModuleB == ChunkID && ConnectionEntry.DoorB == ConnectionId) {
                    ConnectionData = &ConnectionEntry;
                    ThisModuleId = ConnectionEntry.ModuleB;
                    OtherModuleId = ConnectionEntry.ModuleA;
                    bFlipDoorDirection = true;
                    break;
                }
            }
        
            if (ConnectionData) {
                if (bFlipDoorDirection) {
                    ChunkConnectionActor->ConnectionComponent->SpawnOffset = FTransform(FQuat(FVector::UpVector, PI));
                }
                // We have a connection to another chunk node.  This is a door
                if (!ConnectionData->bHasSpawnedDoorActor) {
                    ChunkConnectionActor->DestroyConnectionInstance();
                    UpdateConnectionDoorType(ConnectionData, ChunkConnectionActor->ConnectionComponent);

                    ChunkConnectionActor->BuildConnectionInstance(Dungeon, ThisModuleId, OtherModuleId, DoorLevel);
                    ConnectionData->SpawnedDoorActors = ChunkConnectionActor->GetSpawnedInstancesPtr();
                    ConnectionData->bHasSpawnedDoorActor = true;
                    OnConnectionDoorCreated(ConnectionData);
                }
            
                // Show the actor in the persistent level (it might have been previously hidden due to level streaming)
                if (ConnectionData->bHasSpawnedDoorActor) {
                    for (TWeakObjectPtr<AActor> SpawnedDoorActor : ConnectionData->SpawnedDoorActors) {
                        if (SpawnedDoorActor.IsValid()) {
                            SpawnedDoorActor->SetActorHiddenInGame(false);
                        }
                    }
                }
            }
            else {
                // No connection exists. This is a wall
                ChunkConnectionActor->ConnectionComponent->ConnectionState = ESnapConnectionState::Wall;
                ChunkConnectionActor->BuildConnectionInstance(Dungeon, ChunkID, {}, WallLevel);
            }
        }
    }
}

void USnapStreamingChunkBase::UpdateChunkDoorStates(ULevel* PersistentLevel) const {
    ULevel* ChunkLevel = GetLoadedLevel();
    if (!ChunkLevel) {
        return;
    }
    
    TArray<FSnapConnectionInstance>* ConnectionsPtr = GetAllConnections();
    if (!ConnectionsPtr) {
        return;
    }
    
    const TArray<ASnapConnectionActor*> ConnectionActors = GetActorsOfType<ASnapConnectionActor>(ChunkLevel);
    TArray<FSnapConnectionInstance>& Connections = *ConnectionsPtr;
    Internal_SpawnChunkConnections(ID, Connections, ConnectionActors, PersistentLevel, ChunkLevel);
}

void USnapStreamingChunkBase::HideChunkDoorActors() const {
    // Grab all the visible module chunks
    if (const UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
        TSet<FGuid> VisibleModulesIds;
        for (const UDungeonStreamingChunk* RegisteredChunk : LevelStreamingModel->GetChunks()) {
            if (RegisteredChunk && RegisteredChunk->IsLevelVisible() && RegisteredChunk != this) {
                VisibleModulesIds.Add(RegisteredChunk->ID);
            }
        }

        if (TArray<FSnapConnectionInstance>* ConnectionsPtr = GetAllConnections()) {
            TArray<FSnapConnectionInstance>& AllConnections = *ConnectionsPtr;
            for (const FSnapConnectionInstance& ConnectionInfo : AllConnections) {
                FGuid OtherModuleID;
                if (ConnectionInfo.ModuleA == ID) {
                    OtherModuleID = ConnectionInfo.ModuleB;
                }
                else if (ConnectionInfo.ModuleB == ID) {
                    OtherModuleID = ConnectionInfo.ModuleA;
                }
                else {
                    // Connection Doesn't belong to this chunk
                    continue;
                }

                // This chunk is hidden. Check if the other module is also hidden
                check(!VisibleModulesIds.Contains(ID));
                if (!VisibleModulesIds.Contains(OtherModuleID)) {
                    for (TWeakObjectPtr<AActor> PersistentDoorActor : ConnectionInfo.SpawnedDoorActors) {
                        if (PersistentDoorActor.IsValid()) {
                            PersistentDoorActor->SetActorHiddenInGame(true);
                        }
                    }
                }
            }
        }
    }
}

void USnapStreamingChunkBase::DestroyChunkDoorActors() {
    ULevel* ChunkLevel = GetLoadedLevel();
    TArray<ASnapConnectionActor*> ConnectionActors = GetActorsOfType<ASnapConnectionActor>(ChunkLevel);
    for (ASnapConnectionActor* ConnectionActor : ConnectionActors) {
        ConnectionActor->DestroyConnectionInstance();
    }
}

void USnapStreamingChunkBase::UpdateConnectionDoorType(const FSnapConnectionInstance* ConnectionData, USnapConnectionComponent* ConnectionComponent) const {
    ConnectionComponent->ConnectionState = ESnapConnectionState::Door;
}

TArray<FSnapConnectionInstance>* USnapStreamingChunkBase::GetAllConnections() const {
    if (const ADungeon* Dungeon = GetDungeon()) {
        if (USnapDungeonModelBase* SnapModel = Cast<USnapDungeonModelBase>(Dungeon->GetModel())) {
            return &SnapModel->Connections;
        }
    }
    return nullptr;
}

///////////////////////////////////// Snap Map streaming chunk implementation /////////////////////////////////////


///////////////////////////////////// Snap Grid Flow streaming chunk implementation /////////////////////////////////////
void USnapGridFlowStreamingChunk::OnConnectionDoorCreated(FSnapConnectionInstance* ConnectionData) const {
    Super::OnConnectionDoorCreated(ConnectionData);
    
    const ADungeon* Dungeon = GetDungeon();
    const UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel();
    const USnapGridFlowModel* SnapGridModel = Dungeon ? Cast<USnapGridFlowModel>(Dungeon->GetModel()) : nullptr;
    if (!ConnectionData || !LevelStreamingModel || !SnapGridModel || !SnapGridModel->AbstractGraph) {
        return;
    }
    if (!ConnectionData->bHasSpawnedDoorActor) {
        return;
    }

    UFlowAbstractLink* Link = SnapGridModel->AbstractGraph->GetLink(ConnectionData->ModuleA, ConnectionData->ModuleB, true);
    if (Link && Link->LinkItems.Num() > 0) {
        for (TWeakObjectPtr<AActor> PersistentDoorActor : ConnectionData->SpawnedDoorActors) {
            if (PersistentDoorActor.IsValid()) {
                if (UDungeonFlowItemMetadataComponent* ItemComponent = PersistentDoorActor->FindComponentByClass<UDungeonFlowItemMetadataComponent>()) {
                    ItemComponent->SetFlowItem(Link->LinkItems[0]);
                }
            }
        }
    }
}

void USnapGridFlowStreamingChunk::UpdateConnectionDoorType(const FSnapConnectionInstance* ConnectionData, USnapConnectionComponent* ConnectionComponent) const {
    if (!ConnectionData) {
        ConnectionComponent->ConnectionState = ESnapConnectionState::Unknown;
        return;
    }

    const ADungeon* Dungeon = GetDungeon();
    const USnapGridFlowModel* SnapGridModel = Dungeon ? Cast<USnapGridFlowModel>(Dungeon->GetModel()) : nullptr;
    if (!SnapGridModel) {
        return;
    }
    
    
    UFlowAbstractLink* Link = SnapGridModel->AbstractGraph->GetLink(ConnectionData->ModuleA, ConnectionData->ModuleB, true);
    if (!Link) {
        ConnectionComponent->ConnectionState = ESnapConnectionState::Wall;
        return;
    }

    ConnectionComponent->SpawnOffset = FTransform::Identity;
    ConnectionComponent->ConnectionState = ESnapConnectionState::Door;
    ConnectionComponent->DoorType = ESnapConnectionDoorType::NormalDoor;

    USnapGridFlowAbstractGraph* Graph = SnapGridModel->AbstractGraph;
    UFlowAbstractNode* SourceNode = Link->SourceSubNode.IsValid() ? Graph->FindSubNode(Link->SourceSubNode) : Graph->GetNode(Link->Source);
    UFlowAbstractNode* DestNode = Link->DestinationSubNode.IsValid() ? Graph->FindSubNode(Link->DestinationSubNode) : Graph->GetNode(Link->Destination);

    if (Link->Type == EFlowAbstractLinkType::OneWay) {
        check(SourceNode && DestNode);
        // Check if this is a vertical link
        const FVector::FReal SourceZ = SourceNode->Coord.Z;
        const FVector::FReal DestZ = DestNode->Coord.Z;
        if (FMath::IsNearlyEqual(SourceZ, DestZ, 1e-4f)) {
            ConnectionComponent->DoorType = ESnapConnectionDoorType::OneWayDoor;
        }
        else if (SourceZ < DestZ) {
            ConnectionComponent->DoorType = ESnapConnectionDoorType::OneWayDoorUp;
        }
        else if (SourceZ > DestZ) {
            ConnectionComponent->DoorType = ESnapConnectionDoorType::OneWayDoorDown;
        }
    }
    else if (Link->LinkItems.Num() > 0) {
        UFlowGraphItem* Item = Link->LinkItems[0];
        if (Item && Item->ItemType == EFlowGraphItemType::Lock) {
            ConnectionComponent->DoorType = ESnapConnectionDoorType::LockedDoor;
            ConnectionComponent->MarkerName = Item->MarkerName;
        }
    }

    const bool bFixDoorOrientation = ConnectionComponent->DoorType != ESnapConnectionDoorType::OneWayDoorUp && ConnectionComponent->DoorType != ESnapConnectionDoorType::OneWayDoorDown;
    if (bFixDoorOrientation) {
        // Handle orientation towards the correct one-way direction
        const FVector Forward = ConnectionComponent->GetComponentTransform().GetRotation().GetForwardVector();
        FVector LinkDir = DestNode->Coord - SourceNode->Coord;
        LinkDir.Normalize();

        const FVector::FReal Dot = FVector::DotProduct(Forward, LinkDir);
        // Dot value will be -1 if the vectors point in the opposite direction: https://chortle.ccsu.edu/VectorLessons/vch09/vch09_6.html
        // If this is the case, rotate the spawned one-way door by 180, to fix the direction
        if (Dot < 0) {
            // Points in the opposite direction
            ConnectionComponent->SpawnOffset = FTransform(FQuat(FVector::UpVector, PI));
        }
    }
}



///////////////////////////////////// FSnapStreaming /////////////////////////////////////

namespace {
    UDungeonStreamingChunk* GenerateLevelStreamingChunkRecursive(
            UWorld* World, UDungeonLevelStreamingModel* LevelStreamingModel, TSubclassOf<UDungeonStreamingChunk> ChunkClass, SnapLib::FModuleNodePtr Node,
            UDungeonStreamingChunk* IncomingChunk, TMap<SnapLib::FModuleNodePtr, UDungeonStreamingChunk*>& VisitedChunkMap,
            TFunction<void(UDungeonStreamingChunk*, SnapLib::FModuleNodePtr)> FnInitChunk) {
        if (!Node.IsValid()) return nullptr;

        check(!VisitedChunkMap.Contains(Node));

        UDungeonStreamingChunk* Chunk = LevelStreamingModel->CreateChunk(ChunkClass);
        FnInitChunk(Chunk, Node);
        
        Chunk->ID = Node->ModuleInstanceId;
        Chunk->Bounds = Node->GetModuleBounds();
        Chunk->BoundShapes = Node->GetModuleBoundShapes();
        VisitedChunkMap.Add(Node, Chunk);

        for (SnapLib::FModuleDoorPtr OutgoingDoor : Node->Outgoing) {
            if (!OutgoingDoor.IsValid() || !OutgoingDoor->ConnectedDoor.IsValid()) {
                continue;
            }
            SnapLib::FModuleNodePtr ChildNode = OutgoingDoor->ConnectedDoor->Owner;
            if (!ChildNode.IsValid()) {
                continue;
            }

            if (!VisitedChunkMap.Contains(ChildNode)) {
                UDungeonStreamingChunk* ChildChunk = GenerateLevelStreamingChunkRecursive(
                    World, LevelStreamingModel, ChunkClass, ChildNode, Chunk, VisitedChunkMap, FnInitChunk);
                if (ChildChunk) {
                    Chunk->Neighbors.Add(ChildChunk);
                }
            }
            else {
                // This child has already been processed. Grab the child chunk node and add a reference
                UDungeonStreamingChunk* ChildChunk = VisitedChunkMap[ChildNode];
                if (ChildChunk) {
                    Chunk->Neighbors.Add(ChildChunk);
                    ChildChunk->Neighbors.Add(Chunk);
                }
            }
        }
        if (IncomingChunk) {
            Chunk->Neighbors.Add(IncomingChunk);
        }

        // Generate the level streaming object
        {
            const TSoftObjectPtr<UWorld> ModuleLevelAsset = Node->ModuleDBItem->GetLevel();
            bool bSuccess = false;
            Chunk->CreateLevelStreaming(World, ModuleLevelAsset, Node->ModuleInstanceId, Node->WorldTransform, bSuccess);
        }

        return Chunk;
    }
}


void FSnapStreamingLib::GenerateLevelStreamingModel(UWorld* World, const TArray<SnapLib::FModuleNodePtr>& InModuleNodes,
                                                 UDungeonLevelStreamingModel* LevelStreamingModel, TSubclassOf<UDungeonStreamingChunk> ChunkClass,
                                                 TFunction<void(UDungeonStreamingChunk*, SnapLib::FModuleNodePtr)> FnInitChunk) {

    if (!World || !LevelStreamingModel) return;

    LevelStreamingModel->Initialize(World);

    TMap<SnapLib::FModuleNodePtr, UDungeonStreamingChunk*> VisitedChunkMap;
    for (SnapLib::FModuleNodePtr Node : InModuleNodes) {
        if (VisitedChunkMap.Contains(Node)) continue;
        GenerateLevelStreamingChunkRecursive(World, LevelStreamingModel, ChunkClass, Node, nullptr, VisitedChunkMap, FnInitChunk);
    }

    LevelStreamingModel->Modify();
}

