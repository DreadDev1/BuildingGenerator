//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Rules/Spawn/DungeonSpawnLogic.h"
#include "DungeonMarker.generated.h"

class UProceduralDungeonTransformLogic;

USTRUCT()
struct FDungeonMarkerInstanceTags {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> Tags;

    UPROPERTY()
    TMap<FName, int> IntValues;
    
    UPROPERTY()
    TMap<FName, float> FloatValues;
    
    UPROPERTY()
    TMap<FName, FVector> VectorValues;

    void Append(const FDungeonMarkerInstanceTags& Other);
};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDungeonMarkerInstance {
    GENERATED_BODY()

    FDungeonMarkerInstance() : Id(-1) {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    int32 Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    FString MarkerName;

    /** The name of the clustered theme to override on this marker */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    FString ClusterThemeOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    FTransform Transform;

    TSharedPtr<class IDungeonMarkerUserData> UserData;

    UPROPERTY()
    FDungeonMarkerInstanceTags Tags;

    FORCEINLINE bool operator==(const FDungeonMarkerInstance& Other) const {
        return Other.Id == Id;
    }
};

class DUNGEONARCHITECTRUNTIME_API IDungeonMarkerUserData {
};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDungeonMarkerBuildData {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
    FTransform Transform;

    UPROPERTY()
    FGuid NodeId;

    UPROPERTY()
    int32 MarkerIndex = INDEX_NONE;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<TObjectPtr<UDungeonSpawnLogic>> SpawnLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<UObject> TemplateObject = nullptr;;

    UPROPERTY()
    TArray<FName> ActorTags;
    
    UPROPERTY()
    FDungeonMarkerInstanceTags MarkerTags;
    
    TSharedPtr<IDungeonMarkerUserData> UserData;

    FORCEINLINE bool operator==(const FDungeonMarkerBuildData& other) const {
        return other.NodeId == NodeId;
    }
};
