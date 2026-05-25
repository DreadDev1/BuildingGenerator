//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionConstants.h"
#include "SnapConnectionSerialization.generated.h"

class USnapConnectionInfo;

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapConnectionInstance {
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    FGuid ModuleA;

    UPROPERTY()
    FGuid DoorA;

    UPROPERTY()
    FGuid ModuleB;

    UPROPERTY()
    FGuid DoorB;

    UPROPERTY()
    FTransform WorldTransform;

    UPROPERTY()
    ESnapConnectionDoorType DoorType = ESnapConnectionDoorType::NormalDoor;

    UPROPERTY()
    TWeakObjectPtr<USnapConnectionInfo> ConnectionInfo;
    
    UPROPERTY()
    FString CustomMarkerName;
    
    UPROPERTY()
    bool bHasSpawnedDoorActor = false;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> SpawnedDoorActors;
};


USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapWallInstance {
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    FGuid ModuleId;

    UPROPERTY()
    FGuid DoorId;

    UPROPERTY()
    FTransform WorldTransform;

    UPROPERTY()
    TWeakObjectPtr<USnapConnectionInfo> ConnectionInfo;
};

