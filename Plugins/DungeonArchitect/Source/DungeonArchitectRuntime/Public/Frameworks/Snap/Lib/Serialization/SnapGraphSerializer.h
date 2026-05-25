//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionInfo.h"
#include "Frameworks/Snap/Lib/Serialization/SnapConnectionSerialization.h"
#include "Frameworks/Snap/Lib/SnapLibrary.h"

struct FDefaultSnapConnectionSerializePolicy {
    void GatherConnectionInfo(const FGuid& ModuleA, const FGuid& ModuleB, ESnapConnectionDoorType& OutConnectionType, FString& OutCustomMarkerName) const {
        OutConnectionType = ESnapConnectionDoorType::NormalDoor;
        OutCustomMarkerName = "";
    }
};

template<typename TModuleData, typename TConnectionSerializePolicy = FDefaultSnapConnectionSerializePolicy>
class DUNGEONARCHITECTRUNTIME_API TSnapGraphSerializer {
public:
    static void Serialize(const TArray<SnapLib::FModuleNodePtr>& InNodes, const TConnectionSerializePolicy& ConnectionSerializer,
            TArray<TModuleData>& OutModuleInstances, TArray<FSnapConnectionInstance>& OutConnections, TArray<FSnapWallInstance>& OutWalls) {
        OutConnections.Reset();
        OutModuleInstances.Reset();
        OutWalls.Reset();

        TSet<SnapLib::FModuleNodePtr> Visited;
        for (SnapLib::FModuleNodePtr Node : InNodes) {
            SerializeImpl(Node, ConnectionSerializer, OutModuleInstances, OutConnections, OutWalls, Visited);
        }
    }

private:
    static void SerializeImpl(const SnapLib::FModuleNodePtr& InNode, const TConnectionSerializePolicy& ConnectionSerializer, TArray<TModuleData>& OutModuleInstances,
                              TArray<FSnapConnectionInstance>& OutConnections, TArray<FSnapWallInstance>& OutWalls, TSet<SnapLib::FModuleNodePtr>& Visited) {
        if (!InNode.IsValid() || Visited.Contains(InNode)) {
            return;
        }
        Visited.Add(InNode);

        TModuleData ModuleData;
        ModuleData.ModuleInstanceId = InNode->ModuleInstanceId;
        ModuleData.WorldTransform = InNode->WorldTransform;
        ModuleData.Level = InNode->ModuleDBItem->GetLevel();
        ModuleData.ThemedLevels = InNode->ModuleDBItem->GetThemedLevels();
        ModuleData.Category = InNode->ModuleDBItem->GetCategory();
        ModuleData.ModuleBounds = InNode->ModuleDBItem->GetBounds();
        ModuleData.ModuleBoundShapes = InNode->ModuleDBItem->GetBoundShapes();
        ModuleData.CanvasRoomShapeTextures = InNode->ModuleDBItem->GetCanvasRoomShapeTextures();
        ModuleData.PointsOfInterest = InNode->ModuleDBItem->GetPointsOfInterest();
        
        OutModuleInstances.Add(ModuleData);

        // Save the node's outgoing links in the connection array and traverse those links
        TSet<FGuid> ValidDoors;
        for (const SnapLib::FModuleDoorPtr& Door : InNode->Outgoing) {
            if (!Door.IsValid()) {
                continue;
            }
            if (Door->ConnectedDoor.IsValid() && Door->ConnectedDoor->Owner.IsValid()) {
                const FGuid ModuleA = InNode->ModuleInstanceId;

                const SnapLib::FModuleDoorPtr OtherDoor = Door->ConnectedDoor;
                const FGuid ModuleB = OtherDoor->Owner->ModuleInstanceId;

                FSnapConnectionInstance& DoorConnection = OutConnections.AddDefaulted_GetRef();
                DoorConnection.ModuleA = ModuleA;
                DoorConnection.DoorA = Door->ConnectionId;
                DoorConnection.ModuleB = ModuleB;
                DoorConnection.DoorB = OtherDoor->ConnectionId;
                DoorConnection.WorldTransform = Door->LocalTransform * InNode->WorldTransform;
                DoorConnection.ConnectionInfo = Door->ConnectionInfo;
                ConnectionSerializer.GatherConnectionInfo(ModuleA, ModuleB, DoorConnection.DoorType, DoorConnection.CustomMarkerName);
                
                SerializeImpl(OtherDoor->Owner, ConnectionSerializer, OutModuleInstances, OutConnections, OutWalls, Visited);

                ValidDoors.Add(Door->ConnectionId);
            }
        }
        if (InNode->Incoming.IsValid()) {
            ValidDoors.Add(InNode->Incoming->ConnectionId);
        }

        for (const SnapLib::FModuleDoorPtr& DoorOpening : InNode->Doors) {
            if (!DoorOpening.IsValid()) continue;
            if (ValidDoors.Contains(DoorOpening->ConnectionId)) {
                // Already processed
                continue;
            }

            FSnapWallInstance& WallConnection = OutWalls.AddDefaulted_GetRef();
            WallConnection.ModuleId = InNode->ModuleInstanceId;
            WallConnection.DoorId = DoorOpening->ConnectionId;
            WallConnection.ConnectionInfo = DoorOpening->ConnectionInfo;
            WallConnection.WorldTransform = DoorOpening->LocalTransform * InNode->WorldTransform;
         } 
    }
};

