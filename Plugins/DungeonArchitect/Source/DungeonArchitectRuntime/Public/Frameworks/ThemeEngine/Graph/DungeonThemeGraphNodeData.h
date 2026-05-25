//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonThemeGraphNodeData.generated.h"

class UDungeonSpawnLogic;
class UProceduralDungeonTransformLogic;
class UDungeonTransformLogic;
class UDungeonSelectorLogic;
class UGenericDungeonSelectorLogic;

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDungeonThemeMarkerNodeData {
    GENERATED_BODY()
    
    UPROPERTY()
    FGuid NodeGuid;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    FString MarkerName;
};

/** Theme nodes can emit new markers when they are inserted, to add more child markers relative to them */
USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDungeonThemeMarkerEmitterNodeData {
    GENERATED_BODY()

    UPROPERTY()
    FGuid NodeGuid;
    
    UPROPERTY()
    TArray<FGuid> ParentNodes;
    
    UPROPERTY()
    FGuid MarkerNodeGuid;
    
    /** Used to sort the nodes when executing them from left to right */
    UPROPERTY()
    float ExecutionOrder{};
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    FTransform Offset = FTransform::Identity;
    
    UPROPERTY()
    FString MarkerName_DEPRECATED;
};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDungeonThemeVisualNodeData {
    GENERATED_BODY()

    FDungeonThemeVisualNodeData()
        : AssetObject(nullptr)
        , Probability(1)
        , bUseSelectionLogic(false)
        , bLogicOverridesAffinity(true)
        , bUseTransformLogic(false)
        , bUseSpawnLogic(false)
        , ConsumeOnAttach(false)
        , Offset(FTransform::Identity)
    {
    }

    UPROPERTY()
    FGuid NodeGuid;
    
    UPROPERTY()
    TArray<FGuid> ParentNodes;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<UObject> AssetObject;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float Probability;

    /** Used to sort the nodes when executing them from left to right */
    UPROPERTY()
    float ExecutionOrder{};
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SelectionLogic)
    bool bUseSelectionLogic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SelectionLogic)
    bool bLogicOverridesAffinity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SelectionLogic)
    TArray<TObjectPtr<UDungeonSelectorLogic>> SelectionLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SelectionLogic)
    TArray<TObjectPtr<UGenericDungeonSelectorLogic>> GenericSelectionLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransformLogic)
    bool bUseTransformLogic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransformLogic)
    TArray<TObjectPtr<UDungeonTransformLogic>> TransformLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransformLogic)
    bool bUseProceduralTransformLogic{};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransformLogic)
    TArray<TObjectPtr<UProceduralDungeonTransformLogic>> ProceduralTransformLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpawnLogic)
    bool bUseSpawnLogic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpawnLogic)
    TArray<TObjectPtr<UDungeonSpawnLogic>> SpawnLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool ConsumeOnAttach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    FTransform Offset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<FName> ActorTags;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool bAlignToVoxelGeometry = false;
    
    UPROPERTY()
    FName NodeId_DEPRECATED;
    
    UPROPERTY(Meta=(DeprecatedProperty))
    FString MarkerName_DEPRECATED;
    
    UPROPERTY(Meta=(DeprecatedProperty))
    TArray<FDungeonThemeMarkerEmitterNodeData> MarkerEmitters_DEPRECATED;
};


