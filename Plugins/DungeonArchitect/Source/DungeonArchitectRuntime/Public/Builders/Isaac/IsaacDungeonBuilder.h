//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/Isaac/IsaacDungeonModel.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonModel.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "Containers/Queue.h"
#include "IsaacDungeonBuilder.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(IsaacDungeonBuilderLog, Log, All);

class ADungeon;
class UIsaacDungeonConfig;
class UIsaacDungeonModel;
class FIsaacRoomLayoutBuilder;
class FDAIsaacRoomFactory;

struct FDAIsaacLevelGrowthNode;
/**
*
*/
UCLASS(HideDropdown, Experimental, Meta=(DisplayName="Isaac"))
class DUNGEONARCHITECTRUNTIME_API UIsaacDungeonBuilder : public UDungeonBuilder {
    GENERATED_BODY()

public:
    virtual void BuildDungeonImpl(UWorld* World) override;
    virtual void EmitDungeonMarkers_Implementation() override;
    virtual void DrawDebugData(UWorld* InWorld, bool bPersistant = false, float lifeTime = -1.0f) override;
    virtual void InitializeState() override;
    virtual void MirrorDungeon() override;
    virtual TSubclassOf<UDungeonModel> GetModelClass() override;
    virtual TSubclassOf<UDungeonConfig> GetConfigClass() override;
    virtual TSubclassOf<UDungeonToolData> GetToolDataClass() override;
    virtual TSubclassOf<UDungeonQuery> GetQueryClass() override;
    virtual void GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) override;

private:
    void GenerateLevelLayout();
    void AddNextRoomNode(TSharedPtr<FDAIsaacRoomFactory> roomFactory, TQueue<FDAIsaacLevelGrowthNode>& queue,
                         TSet<FIntVector>& visited, int maxRooms, FIsaacRoomPtr parentRoom, int direction,
                         float probability);
    void ConnectRoomsWithDoors(FIsaacRoomPtr roomA, FIsaacRoomPtr roomB);
    FIsaacRoomPtr GetRoomAt(const FIntVector& position);
    bool ContainsDoorBetween(int roomA, int roomB);
    void CreateDoor(FIsaacRoomPtr roomA, FIsaacRoomPtr roomB, float ratio);
    FIsaacRoomLayout GenerateEmptyRoomLayout();
    void CreateMarkerAt(float gridX, float gridY, float angle, const FString& markerName,
                        const FVector& roomBasePosition, const FVector& tileSize);
    void EmitBoundaryMarkers(const FString& markerName, EIsaacRoomTileType adjacentTile1,
                             EIsaacRoomTileType adjacentTile2,
                             const FIsaacRoom& room, int32 roomWidth, int32 roomHeight, const FVector& roomBasePosition,
                             const FVector& tileSize);

protected:

    virtual void MirrorDungeonWithVolume(ADungeonMirrorVolume* MirrorVolume) override;
    virtual bool PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) override;
    virtual FTransform PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) override;

private:
    void BuildLayout();

private:
    int32 _CellIdCounter;
    UIsaacDungeonModel* isaacModel;
    UIsaacDungeonConfig* isaacConfig;

    TArray<FIsaacRoomPtr> rooms;
    TArray<FIsaacDoorPtr> doors;
};

