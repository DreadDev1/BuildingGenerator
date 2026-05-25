//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/Grid/GridDungeonModel.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonModel.h"
#include "Core/Utils/PMRandom.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "GridDungeonBuilder.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(GridDungeonBuilderLog, Log, All);

class ADungeon;
class UGridDungeonConfig;
class UGridDungeonModel;
class UGridDungeonQuery;

namespace GridDungeonBuilderImpl {
    struct FCellHeightNode;
}

/**
*
*/
UCLASS(Meta=(DisplayName="Grid", Description="Procedural dungeon generator with interesting features like height variations and other extension points"))
class DUNGEONARCHITECTRUNTIME_API UGridDungeonBuilder : public UDungeonBuilder {
    GENERATED_BODY()

public:
    virtual void BuildDungeonImpl(UWorld* World) override;
    virtual void EmitDungeonMarkers_Implementation() override;
    virtual void DrawDebugData(UWorld* InWorld, bool bPersistant = false, float LifeTime = -1.0f) override;
    virtual void MirrorDungeon() override;
    virtual void InitializeState() override;
    virtual TSubclassOf<UDungeonModel> GetModelClass() override;
    virtual TSubclassOf<UDungeonConfig> GetConfigClass() override;
    virtual TSubclassOf<UDungeonToolData> GetToolDataClass() override;
    virtual TSubclassOf<UDungeonQuery> GetQueryClass() override;
    virtual void CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules) override;
    
    virtual void GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) override;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    bool ContainsCell(int32 x, int32 y);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    void GetRooms(TArray<FGridDungeonCell>& RoomCells);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    void GetCorridors(TArray<FGridDungeonCell>& CorridorCells);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    void GetCellsOfType(EGridDungeonCellType CellType, TArray<FGridDungeonCell>& Cells);

    /* Check if there is a path from the source to destination cell, taking the height information and currently connected stairs into account */
    bool ContainsAdjacencyPath(int32 cellA, int32 cellB, int32 maxDepth, bool bForceRoomConnection);
    bool ContainsStair(const FGridDungeonCell& baseCell, int32 x, int32 y);
    bool ContainsStair(int32 ownerCellId, int32 connectedToCellId);
    void AddCorridorPadding(const FIntVector& P);
    void AddCorridorPadding(int x, int y, int z);
    virtual void GenerateAdjacencyLookup();

protected:
    virtual void BuildDungeonCells();
    virtual void GenerateRoomConnections();
    virtual bool IdentifyBuildSucceeded() const override;
    virtual TSharedPtr<IMarkerGenProcessor> CreateMarkerGenProcessor(const FTransform& InDungeonTransform) const override;

    virtual void MirrorDungeonWithVolume(ADungeonMirrorVolume* MirrorVolume) override;
    virtual bool PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) override;
    virtual FTransform PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) override;

    FGridDungeonCell BuildCell();
    FGridDungeonCell BuildCell(const FRectangle& InBounds);
    FIntVector GenerateCellSize();
    void BuildDungeonCellsIterative();
    void BuildDungeonCellsDistributed();
    bool CanFitDistributionCell(const TSet<FIntVector>& Occupancy, const FRectangle& bounds);
    void SetDistributionCellOccupied(TSet<FIntVector>& Occupancy, const FRectangle& bounds);


    void Shuffle();

    void BuildCells();
    void Seperate();
    void AddUserDefinedPlatforms(UWorld* World);
    void AddUserDefinedPlatform(class AGridDungeonPlatformVolume* Volume);
    void AddUserDefinedPlatform(const FRectangle& Bounds, const EGridDungeonCellType& CellType, bool bInHeightClamped);
    void ApplyNegationVolumes(UWorld* World);
    void TriangulateRooms();
    void BuildMinimumSpanningTree();
    void RemoveRedundantDoors();

    virtual void ConnectCorridors();

    /** Groups the dungeon into various clusters.  This is used to apply different themes on various parts of the dungeon, if specified, to give variations */
    void CreateLayoutClusterGroups();
    void AssignClusteredThemesOnMarkers();
    FString GetClusterThemeForId(int32 ClusterId, int32 CellId);
    void FloodFillCorridorGroup(FGridDungeonCell& StartCell, int32 GroupId);

    /** Applies the specified offset to all the cells in the  model */
    void ApplyCellOffset();

    FIntVector GetDungeonOffset() const;
    DungeonModelBuildState GetBuildState();
    FGridDungeonCell GetCell(int32 Id);
    TArray<FGridDungeonCellDoor> GetDoors();

    void BuildMesh_Floor(const FGridDungeonCell& cell);
    void BuildMesh_Room(const FGridDungeonCell& cell);
    void BuildMesh_RoomDecoration(const FGridDungeonCell& cell);
    void BuildMesh_Corridor(const FGridDungeonCell& cell);
    void BuildMesh_Corridor_BlockWalls(const FGridDungeonCell& cell);
    void BuildMesh_Stairs(const FGridDungeonCell& cell);

    TArray<FGridDungeonCell*> GetCellsOfType(EGridDungeonCellType CellType);

    static bool CompareFromCenterPredicate(const FGridDungeonCell& cellA, const FGridDungeonCell& cellB) {
        FIntVector ca = cellA.Bounds.Center();
        FIntVector cb = cellB.Bounds.Center();
        int32 distA = ca.X * ca.X + ca.Y * ca.Y + ca.Z * ca.Z;
        int32 distB = cb.X * cb.X + cb.Y * cb.Y + cb.Z * cb.Z;
        return distA < distB;
    }

    static void ConnectCells(FGridDungeonCell& c1, FGridDungeonCell& c2) {
        AddUnique<int32>(c1.ConnectedRooms, c2.Id);
        AddUnique<int32>(c2.ConnectedRooms, c1.Id);
    }

    template <typename T>
    static void AddUnique(TArray<T>& Array, T value) {
        if (!Array.Contains(value)) {
            Array.Add(value);
        }
    }

    bool CheckLoop(FGridDungeonCell* currentNode, FGridDungeonCell* comingFrom, TSet<FGridDungeonCell*>& visited);
    bool ContainsLoop(const TArray<FGridDungeonCell*>& rooms);
    void ConnectCooridorRecursive(int32 incomingRoom, int32 currentRoom, TSet<int32>& visited);
    void ConnectRooms(int32 roomA, int32 roomB);
    void ConnectAdjacentCells(int32 roomA, int32 roomB);
    int32 RegisterCorridorCell(int cellX, int cellY, int cellZ, int32 roomA, int32 roomB,
                               bool canRegisterDoors = false);
    bool GetStair(int32 ownerCell, int32 connectedToCell, FGridDungeonStairInfo& outStair);
    bool CanDrawFence(const FGridDungeonCell& baseCell, int32 x, int32 y, bool& isElevatedFence, bool& drawPillar,
                      int& elevationHeight);
    int32 GetStairHeight(const FGridDungeonStairInfo& stair);
    void FixDoorTransform(int32 x0, int32 y0, int32 x1, int32 y1, FTransform& OutTransform);
    void RemoveStairAtDoor(const FGridDungeonCellDoor& Door);
    void SerializeModel();

    static int GetForceDirectionMultiplier(float a, float b, float a1, float b1);

    FIntVector GetRandomPointInCircle(double radius);

    bool AreCellsAdjacent(int32 cellAId, int32 cellBId);

    int32 GetRandomRoomSize();

    void CheckAndMarkAdjacent(FGridDungeonCell& cell, int32 otherCellX, int32 otherCellY);


    /**
    * Gets the elevation of the baseCell from the cell defined in (x,y)
    * Also outputs the z value that needs to be applied to the base cell
    */
    int32 GetElevation(const FGridDungeonCell& baseCell, int32 x, int32 y, int32& OutZOffset);

    int32 GetNextCellId();

    void AddGridMarker(const FString& InMarkerName, const FTransform& InTransform, const TSharedPtr<class IDungeonMarkerUserData>& InUserData = nullptr);
    void AddGridMarker(const FString& InMarkerName, const FTransform& InTransform, int InCount, const FVector& InterOffset, const TSharedPtr<IDungeonMarkerUserData>& InUserData = nullptr);
    
    // Generates a list of room rects based on the painted 1x1 tiles.  This function must have all tiles on the same Z plane
    void GenerateRoomsFromPaintData(const TSet<FIntVector>& Cells);

    void FindConnectionPath(FGridDungeonCell* roomA, FGridDungeonCell* roomB, TArray<FIntVector>& OutPath);
    static void GetRoomConnectionPointsForDoors(const FRectangle& InBounds, bool bInSkipCornerTiles,
                                                TArray<FIntVector>& OutConnectionPoints);
    static void RemovePointsInsideBounds(const FRectangle& InBounds, TArray<FIntVector>& OutConnectionPoints);
    static bool ContainsPointInsideBounds(const FRectangle& InBounds, const TArray<FIntVector>& InPoints);
    static bool ContainsPointInsideCells(const TArray<FIntVector>& InPoints, TArray<FGridDungeonCell>& InCells);
    static void GetManhattanPath(const FIntVector& InStart, const FIntVector& InMid, const FIntVector& InEnd,
                                 TArray<FIntVector>& OutPath);
    static void GetPathOnAxisLine(const FIntVector& InStart, const FIntVector& InEnd, TArray<FIntVector>& OutPath);

protected:
    int32 CellIdCounter{};
    
    UPROPERTY(Transient)
    TObjectPtr<UGridDungeonModel> GridModel = {};;
    
    UPROPERTY(Transient)
    TObjectPtr<UGridDungeonConfig> GridConfig = {};;

    UPROPERTY(Transient)
    TObjectPtr<UGridDungeonQuery> GridQuery;
    
    FVector GridToMeshScale;
    FTransform DungeonTransform;

    PMRandom GaussRandom;
};


class DUNGEONARCHITECTRUNTIME_API FGridBuilderMarkerUserData : public IDungeonMarkerUserData {
public:
    FGridBuilderMarkerUserData(int32 InCellId, int32 InClusterId)
        : CellId(InCellId)
          , ClusterId(InClusterId) {
    }

    FGridBuilderMarkerUserData() {
    }

    int32 CellId = -1;
    int32 ClusterId = -1;
};

