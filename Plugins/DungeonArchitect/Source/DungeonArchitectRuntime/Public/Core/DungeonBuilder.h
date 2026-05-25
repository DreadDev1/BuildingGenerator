//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonCommon.h"
#include "Core/DungeonModel.h"
#include "Core/DungeonQuery.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Rules/Selector/GenericDungeonSelectorLogic.h"
#include "Frameworks/Voxel/Meshing/VoxelMeshGenerationSettings.h"
#include "Frameworks/Voxel/VDB/VDBLib.h"

#include "Templates/SubclassOf.h"
#include "DungeonBuilder.generated.h"

class IMarkerGenProcessor;
class FDungeonSceneProvider;
class IDungeonMarkerUserData;
struct FDungeonBuilderParameters;
struct FDungeonThemeVisualNodeData;
struct FDungeonThemeEngineSettings;

class ADungeon;
class UDungeonMarkerEmitter;
class ADungeonMirrorVolume;
class UDungeonModel;
class UDungeonConfig;
class UDungeonQuery;
class UDungeonToolData;
class UDungeonSelectorLogic;
class UDungeonTransformLogic;
class UProceduralDungeonTransformLogic;
class UProceduralMarkerEmitter;
class UDungeonLevelStreamingModel;
class FDungeonMarkerVisualizationRules;
class FDungeonSceneProviderCommandList;
enum class EDungeonMarkerEmitterExecStage : uint8;

DECLARE_LOG_CATEGORY_EXTERN(DungeonBuilderLog, Log, All);

struct FDungeonBuilderCapabilities {
    bool bSupportsTheming = true;
    bool bSupportsLevelStreaming = false;
    bool bSupportsOverlappingFloors = false;
};

UCLASS(Blueprintable, abstract)
class DUNGEONARCHITECTRUNTIME_API UDungeonBuilder : public UObject {
    GENERATED_BODY()

public:
    void BuildDungeon(ADungeon* InDungeon, UWorld* InWorld, const FDungeonBuilderParameters& InBuilderParameters);
    void BuildDungeon(UDungeonModel* InModel, UDungeonConfig* InConfig, UDungeonQuery* InQuery, UWorld* InWorld, const FDungeonBuilderParameters& InBuilderParameters);
    void DestroyDungeon(UDungeonModel* InModel, UDungeonConfig* InConfig, UDungeonQuery* InQuery, ADungeon* InDungeon, UWorld* InWorld);

    void ApplyDungeonTheme(const FDungeonThemeEngineSettings& InThemeSettings);
    
    virtual void MirrorDungeon();
    virtual void DrawDebugData(UWorld* InWorld, bool bPersistant = false, float LifeTime = -1.0f) {}

    virtual TSubclassOf<UDungeonModel> GetModelClass() { return nullptr; }
    virtual TSubclassOf<UDungeonConfig> GetConfigClass() { return nullptr; }
    virtual TSubclassOf<UDungeonToolData> GetToolDataClass() { return nullptr; }
    virtual TSubclassOf<UDungeonQuery> GetQueryClass() { return nullptr; }
    virtual bool SupportsProperty(const FName& PropertyName) const { return true; }
    virtual FDungeonBuilderCapabilities GetCapabilities() { return {}; }
    
    virtual TSharedPtr<FDungeonSceneProvider> CreateSceneProvider(bool bInInstanced);
    virtual bool CanBuildDungeon(FString& OutMessage) { return true; }
    virtual void CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules);
    virtual void EmitCustomSceneObjects(FDungeonSceneProvider& SceneProvider);

    FRandomStream& GetRandomStreamRef() { return Random; }
    UDungeonModel* GetModel() const { return DungeonModel; }

    FDungeonMarkerInstance& AddMarker(const FString& InMarkerName, const FTransform& InTransform);

    virtual void GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) {}

    UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
    void EmitDungeonMarkers();
    virtual void EmitDungeonMarkers_Implementation();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void EmitMarker(const FString& InMarkerName, const FTransform& InTransform);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    void GetRandomStream(FRandomStream& OutRandomStream);

    void EmitCustomMarkers(TArray<UDungeonMarkerEmitter*> MarkerEmitters, EDungeonMarkerEmitterExecStage InExecutionStage);
    void EmitProceduralMarkers(const TArray<UProceduralMarkerEmitter*>& InProceduralMarkerEmitters);

    static UClass* DefaultBuilderClass();
    void ProcessMarkerReplacementVolumes() const;

    bool HasBuildSucceeded() const { return bBuildSucceeded; }
    virtual bool IsGenerating() const { return false; }
    virtual void AbortGeneration() {}

    virtual void ProcessThemeItemUserData(TSharedPtr<IDungeonMarkerUserData> UserData, AActor* SpawnedActor);
    void ClearMarkerList();

    virtual bool PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
        return false;
    }

    virtual bool PerformGenericSelectionLogic(const TArray<UGenericDungeonSelectorLogic*>& GenericSelectionLogics, const FDungeonMarkerInstance& InMarker);

    virtual FTransform PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) {
        return FTransform::Identity;
    }

    virtual FTransform PerformProceduralTransformLogic(const TArray<UProceduralDungeonTransformLogic*>& ProceduralTransformLogics, const FDungeonMarkerInstance& InMarkerInfo);
    virtual TSharedPtr<IMarkerGenProcessor> CreateMarkerGenProcessor(const FTransform& InDungeonTransform) const { return nullptr; }
    
protected:
    virtual void BuildDungeonImpl(UWorld* World) { };
    virtual void CreateVoxelChunkCommands(const FDAVoxelMeshGenerationSettings& InVoxelMeshSettings, const FDAVoxelNoiseSettings& InVoxelNoiseSettings,
            class UDungeonVoxelSDFModel* InSDFModel, FDungeonSceneProviderCommandList& OutCommands);
    virtual void DestroyDungeonImpl(UWorld* InWorld) { }
    virtual void MirrorDungeonWithVolume(ADungeonMirrorVolume* MirrorVolume) { }
    virtual bool IdentifyBuildSucceeded() const { return true; }
    virtual bool EmitProceduralMarkers(const UProceduralMarkerEmitter* InProceduralMarkerEmitter) { return false; }
    virtual void InitializeState();
    virtual bool ShouldDestroyBeforeBuild() const;
    void ProcessMarkerReplacementVolume(class ADungeonMarkerReplaceVolume* MarkerReplaceVolume) const;
    UDungeonLevelStreamingModel* GetLevelStreamingModel();
    FTransform GetDungeonTransform() const;
    
public:
    UPROPERTY()
    TObjectPtr<UDungeonConfig> DungeonConfig;

protected:
    FRandomStream Random;

    UPROPERTY()
    TObjectPtr<ADungeon> Dungeon;

    UPROPERTY()
    TObjectPtr<UDungeonModel> DungeonModel;

    UPROPERTY()
    TObjectPtr<UDungeonQuery> DungeonQuery;
    
    int32 _MarkerIdCounter;

    bool bBuildSucceeded = true;

    FDungeonBuilderParameters BuilderParameters;
};

