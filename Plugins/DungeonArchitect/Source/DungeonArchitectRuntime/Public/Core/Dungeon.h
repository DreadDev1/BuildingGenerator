//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonCommon.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamer.h"
#include "Frameworks/Lib/Async/DependencyGraph.h"
#include "Frameworks/PCG/DungeonPCGCache.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Markers/DungeonMarkerEmitter.h"
#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"

#include "Async/AsyncWork.h"
#include "GameFramework/Actor.h"
#include "Dungeon.generated.h"

class UDungeonDebugVisualizer;
class UBoxComponent;
class UPCGComponent;
class ADungeon;
class UBillboardComponent;
class UDungeonModel;
class UDungeonQuery;
class UDungeonConfig;
class UDungeonBuilder;
class UDungeonToolData;
class UDungeonEventListener;
class UProceduralMarkerEmitter;
class UDungeonBuildSystemDungeonComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonBuildCompleteBindableEvent, ADungeon*, Dungeon, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonPropertyChangedEvent, ADungeon*, Dungeon, const FName&, PropertyName);

DECLARE_LOG_CATEGORY_EXTERN(DungeonLog, Log, All);

/**
* The main dungeon actor responsible for creating a dungeon.  
* Drop this actor into your scene, assign a theme and click "Build Dungeon" button
* to create your dungeon.  
* From code, call ADungeon::BuildDungeon after adding an entry into the ADungeon::Themes array
*/
UCLASS(NotBlueprintable, ShowCategories=(Transform, Dungeon))
class DUNGEONARCHITECTRUNTIME_API ADungeon : public AActor {
    GENERATED_BODY()

public:
    ADungeon(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void BuildDungeon();
    
    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void BuildDungeonWithSettings(const FDungeonBuildSettings& InBuildSettings);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void DestroyDungeon();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    UDungeonBuilder* SetBuilderClass(TSubclassOf<UDungeonBuilder> InBuilderClass);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    UDungeonQuery* GetQuery();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void RandomizeSeed();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    UDungeonCanvasComponent* GetDungeonCanvas() const;

    /** A status message to show on your loading screen progress bar */
    UFUNCTION(BlueprintCallable, Category = Dungeon)
    FDungeonBuildStatusMessage GetBuildStatusMessage();

    /**
     * Resets the dungeon id.  Do this if you have another copy of the dungeon somehow (e.g. streaming in the same level multiple times on the scene)
     * NOTE: This will unlink all the existing dungeon spawned items and they will not clean up. Make sure the existing dungeon is destroyed first
     */
    UFUNCTION(BlueprintCallable, Category=Advanced)
    void ResetDungeonId();

    virtual void Destroyed() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent, bool bIncludeFromChildActors) const override;

    void CreateBuilderInstance();
    void CreateBuilderInstance(const FObjectInitializer& ObjectInitializer);
    TObjectPtr<UDungeonBuildSystemDungeonComponent> GetBuildSystemComponent() const;

    virtual UDungeonToolData* GetToolData() const { return ToolData; }
    virtual UDungeonConfig* GetConfig() const { return Config; }
    virtual UDungeonModel* GetModel() const { return DungeonModel; }
    virtual UDungeonBuilder* GetBuilder() const { return Builder; }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif

#if WITH_EDITORONLY_DATA
    UBillboardComponent* GetSpriteComponent() const;
#endif //WITH_EDITORONLY_DATA

    void PostDungeonBuild();
    void ReleasePCGResources();

    /** For streaming dungeons, fires the deferred build complete event */
    void FireDeferredBuildCompleteEvent();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Set when PostDungeonBuild is deferred for streaming dungeons */
    bool bPendingBuildCompleteEvent = false;

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent;
#endif //WITH_EDITORONLY_DATA

private:
    void InitializeQueryObject();
    void ProcessLevelStreaming() const;
    void AssignNewDungeonId();
    void ClearAllDebugVisualizations();
public:
    UPROPERTY()
    FGuid Uid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<TObjectPtr<UDungeonThemeAsset>> Themes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<TObjectPtr<UPCGGraph>> PCGGraphs;
    
    /** Lets you swap out the default dungeon builder with your own implementation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TSubclassOf<UDungeonBuilder> BuilderClass;

    /** Lets you emit your own markers into the scene using custom blueprints */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category="Marker Emitters", DisplayName="Marker Emitter Blueprints")
    TArray<TObjectPtr<UDungeonMarkerEmitter>> MarkerEmitters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category="Marker Emitters")
    TArray<TObjectPtr<UProceduralMarkerEmitter>> ProceduralMarkerEmitters;

    UPROPERTY(EditAnywhere, Category="Voxel")
    bool bCarveVoxels = false;
    
    UPROPERTY(EditAnywhere, Category="Voxel", meta=(EditCondition="bCarveVoxels"))
    FDAVoxelMeshGenerationSettings VoxelMeshSettings;

    UPROPERTY(EditAnywhere, Category="Voxel", meta=(EditCondition="bCarveVoxels"))
    FDAVoxelNoiseSettings VoxelNoiseSettings;

    /** SDF Model for world generation (Cave, Island, etc.) */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category="Voxel", meta=(EditCondition="bCarveVoxels"))
    TObjectPtr<UDungeonVoxelSDFModel> VoxelSDFModel;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = Dungeon, AdvancedDisplay)
    TArray<TObjectPtr<UDungeonEventListener>> EventListeners;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon, AdvancedDisplay)
    TArray<FClusterThemeInfo> ClusterThemes;

    UPROPERTY(EditAnywhere, Category = Dungeon, AdvancedDisplay)
    bool bDrawDebugData;

    UPROPERTY(EditAnywhere, Category = Dungeon, AdvancedDisplay)
    bool bUseCustomItemFolderName{};
    
    UPROPERTY(EditAnywhere, Category = Dungeon, AdvancedDisplay, meta=(EditCondition="bUseCustomItemFolderName"))
    FString CustomItemFolderName{};
    
    UPROPERTY(BlueprintReadOnly, Category = Misc)
    TObjectPtr<UDungeonBuilder> Builder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Misc)
    TObjectPtr<UDungeonConfig> Config;

    UPROPERTY(BlueprintReadOnly, Category = Misc)
    TObjectPtr<UDungeonModel> DungeonModel;

    UPROPERTY()
    TObjectPtr<UDungeonToolData> ToolData;

    UPROPERTY() 
    TObjectPtr<UDungeonQuery> Query;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Dungeon)
    TObjectPtr<UDungeonDebugVisualizer> DebugVisualizer;
    
    UPROPERTY()
    TMap<FGuid, TObjectPtr<UDungeonDebugVisualizer>> TemporaryDebugVisualizers;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Dungeon)
    TObjectPtr<USceneComponent> DungeonSceneRoot = {};;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Dungeon)
    TObjectPtr<UBoxComponent> DungeonBoundsComponent = {};;
    
    UPROPERTY()
    TArray<TObjectPtr<UPCGComponent>> PCGComponents;

    UPROPERTY()
    TObjectPtr<UDungeonPCGCache> PCGCache = {};;
    
    UPROPERTY()
    FDungeonLevelStreamingConfig LevelStreamingConfig;

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    TObjectPtr<UDungeonLevelStreamingModel> LevelStreamingModel;

    /** When the dungeon is built asynchronously over multiple frames, objects closer to this point are built first */
    UPROPERTY(BlueprintReadWrite, Category = Dungeon)
    FVector BuildPriorityLocation;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDungeonBuildCompleteBindableEvent OnDungeonBuildComplete;

#if WITH_EDITOR
    /** Broadcast when any property changes in the editor (especially useful for tracking BuilderClass changes) */
    FDungeonPropertyChangedEvent OnDungeonPropertyChanged;
#endif

    /** The folder under which the spawned actors should be placed in */
    FName ItemFolderPath;

    TWeakPtr<const FDADependencyGraph> GetDependencyGraph() const;
    
private:
    UPROPERTY(Category = Dungeon, Replicated, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UDungeonBuildSystemDungeonComponent> BuildSystemComponent = {};

    TSharedPtr<FDADependencyGraph> BuildGraph;

    friend class FDungeonActorBuildManager;
};


/** This component would be added to any actor spawned into the scene. It contains metadata about the spawned actor */
UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonSpawnDataComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UDungeonSpawnDataComponent();

    UPROPERTY()
    TWeakObjectPtr<ADungeon> OwningDungeon;

    UPROPERTY()
    FGuid ThemeNodeId = {};
    
    UPROPERTY()
    FGuid ChunkId = {};
    
    UPROPERTY()
    int32 MarkerIndex = INDEX_NONE;
};

