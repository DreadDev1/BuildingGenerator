//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"
#include "Core/Utils/Attributes.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"
#include "Frameworks/Flow/FlowProcessor.h"
#include "Frameworks/FlowImpl/GridFlow/Tilemap/GridFlowTilemap.h"

#include "Components/ActorComponent.h"
#include "GridFlowBuilder.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(GridFlowBuilderLog, Log, All);

class ADungeon;
class ADungeonVolume;
class UGridFlowConfig;
class UGridFlowQuery;
class UGridFlowModel;

class UGridFlowTilemap;
struct FFlowTilemapEdge;

UCLASS(Meta=(DisplayName="Grid Flow", Description="Design the layout of your procedural dungeons using the flow editor. Then create an infinite number of procedural dungeons that follow this layout rule. Create cyclic-paths, key-locks, teleporters, shops, treasure rooms, boss rooms and much more"))
class DUNGEONARCHITECTRUNTIME_API UGridFlowBuilder : public UDungeonBuilder {
    GENERATED_BODY()

public:
    virtual void BuildDungeonImpl(UWorld* World) override;
    virtual void DestroyDungeonImpl(UWorld* InWorld) override;
    virtual void EmitDungeonMarkers_Implementation() override;
    virtual void InitializeState() override;
    virtual void DrawDebugData(UWorld* InWorld, bool bPersistant = false, float lifeTime = -1.0f) override;
    virtual TSubclassOf<UDungeonModel> GetModelClass() override;
    virtual TSubclassOf<UDungeonConfig> GetConfigClass() override;
    virtual TSubclassOf<UDungeonToolData> GetToolDataClass() override;
    virtual TSubclassOf<UDungeonQuery> GetQueryClass() override;
    virtual void GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) override;
    virtual bool PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) override;
    virtual FTransform PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) override;
    virtual void ProcessThemeItemUserData(TSharedPtr<IDungeonMarkerUserData> UserData, AActor* SpawnedActor) override;
    virtual void CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules) override;

protected:
    bool ExecuteGraph() const;
    void EmitMarkerAt(const FVector& WorldLocation, const FString& MarkerName, const FQuat& Rotation,
                      TSharedPtr<class IDungeonMarkerUserData> InUserData = nullptr);
    void EmitMarkerAt(const FVector& WorldLocation, const FString& MarkerName, float Angle,
                      TSharedPtr<class IDungeonMarkerUserData> InUserData = nullptr);

    void EmitEdgeMarker(const FFlowTilemapEdge& Edge, const FVector& TileCoord,
                        const FVector& GridSize, UGridFlowTilemap* Tilemap, const TMap<FGuid, const UFlowGraphItem*>& Items);

    virtual bool IdentifyBuildSucceeded() const override;
    virtual TSharedPtr<IMarkerGenProcessor> CreateMarkerGenProcessor(const FTransform& InDungeonTransform) const override;
    
    void CreateDebugVisualizations(const FGuid& InDungeonId, const FTransform& InTransform) const;
    void DestroyDebugVisualizations(const FGuid& InDungeonId) const;
    FIntPoint GetTilemapOffset() const;
private:
    TWeakObjectPtr<UGridFlowModel> GridFlowModel;
    TWeakObjectPtr<UGridFlowConfig> GridFlowConfig;
    TWeakObjectPtr<UGridFlowQuery> GridFlowQuery;
    FDAAttributeList AttributeList;
};


class DUNGEONARCHITECTRUNTIME_API FGridFlowBuilderMarkerUserData : public IDungeonMarkerUserData {
public:
    FFlowTilemapCoord TileCoord;
    bool bIsItem = false;
    TWeakObjectPtr<const UFlowGraphItem> Item;
};


class DUNGEONARCHITECTRUNTIME_API FGridFlowProcessDomainExtender : public IFlowProcessDomainExtender {
public:
    virtual void ExtendDomains(FFlowProcessor& InProcessor) override;

};

