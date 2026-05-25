//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarker.h"
#include "Core/Utils/DungeonEditorViewportProperties.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeCompiledGraph.h"
#include "DungeonThemeAsset.generated.h"

class UEdGraph;
class UMarkerGenModel;

UENUM()
enum class EDungeonThemeAssetVersion : uint8
{
    None = 0,
    
    // Initial version with Props array
    V1_Initial,
    
    // Version where Props moved to CompiledThemeData
    V2_MovedToCompiledData,
    
    // -----<new versions can be added above this line>-----
    LastVersionPlusOne,
    LatestVersion = LastVersionPlusOne - 1
};

/** A Dungeon Theme asset lets you design the look and feel of you dungeon with an intuitive graph based approach */
UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonThemeAsset : public UObject {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    int32 Version;
    
    UPROPERTY()
    FDungeonThemeCompiledGraph CompiledThemeGraph;
    
    UPROPERTY()
    TObjectPtr<UMarkerGenModel> MarkerGenerationModel;

#if WITH_EDITORONLY_DATA
    /** EdGraph based representation */
    UPROPERTY()
    TObjectPtr<UEdGraph> UpdateGraph;
#endif // WITH_EDITORONLY_DATA

    UPROPERTY()
    TObjectPtr<UDungeonEditorViewportProperties> PreviewViewportProperties;

public:
    static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
    bool IsLatestVersion() const;
    virtual void Serialize(FArchive& Ar) override;
    
public:
    UPROPERTY()
    TArray<FDungeonThemeVisualNodeData> Props_DEPRECATED;
};


/**
A dungeon builder can cluster different parts of the dungeon into groups where different themes can be applied to each group
This registry contains the theme registration for each group
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FClusterThemeInfo {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    FString ClusterThemeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<TObjectPtr<UDungeonThemeAsset>> Themes;
};

