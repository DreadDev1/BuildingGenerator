//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonModel.h"
#include "Core/Utils/Rectangle.h"
#include "GridDungeonModel.generated.h"

UENUM(BlueprintType)
enum class DungeonModelBuildState : uint8 {
    Initial UMETA(DisplayName = "Initial"),
    Separation UMETA(DisplayName = "Separation"),
    Triangulation UMETA(DisplayName = "Triangulation"),
    SpanningTree UMETA(DisplayName = "SpanningTree"),
    Corridors UMETA(DisplayName = "Corridors"),
    Complete UMETA(DisplayName = "Complete")
};

UENUM(BlueprintType)
enum class EGridDungeonCellType : uint8 {
    Room UMETA(DisplayName = "Room"),
    Corridor UMETA(DisplayName = "Corridor"),
    CorridorPadding UMETA(DisplayName = "CorridorPadding"),
    Unknown UMETA(DisplayName = "Unknown")
};


USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FGridDungeonCellClusterInfo {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Dungeon Architect")
    int32 Id = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category="Dungeon Architect")
    TArray<int32> CellIds;
  
    UPROPERTY(BlueprintReadOnly, Category="Dungeon Architect")
    TArray<FIntPoint> Boundary;

    UPROPERTY(BlueprintReadOnly, Category="Dungeon Architect")
    int32 GridZ = 0;
};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FGridDungeonCell {
    GENERATED_USTRUCT_BODY()

    FGridDungeonCell() : Id(0), CellType(EGridDungeonCellType::Unknown), UserDefined(false), ClusterId(INDEX_NONE) {
    }

    FGridDungeonCell(int32 x, int32 y, int32 width, int32 height)
        : Id(0)
        , CellType(EGridDungeonCellType::Unknown)
        , UserDefined(false)
        , ClusterId(INDEX_NONE)
    {
        Bounds.Location.X = x;
        Bounds.Location.Y = y;
        Bounds.Size.X = width;
        Bounds.Size.Y = height;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    int32 Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    FRectangle Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    EGridDungeonCellType CellType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool UserDefined;

    /** Is the height of this cell clamped? If so, the height generation algorithm will not change this cell's height */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool bHeightClamped = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    int32 ClusterId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<int32> ConnectedRooms;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<int32> FixedRoomConnections;

    UPROPERTY()
    TArray<int32> AdjacentCells;

    FIntVector Center() const {
        return Bounds.Center();
    }

    FORCEINLINE bool operator==(const FGridDungeonCell& other) const {
        return other.Id == Id;
    }

};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FGridDungeonCellDoor {
    GENERATED_USTRUCT_BODY()

    FGridDungeonCellDoor()
        : AdjacentTiles{}
        , AdjacentCells{}
        , bEnabled(true)
    {
    }

    FIntVector AdjacentTiles[2];
    int AdjacentCells[2];
    bool bEnabled;

};

bool operator==(const FGridDungeonCellDoor& A, const FGridDungeonCellDoor& B);

/* Holds metadata about each occupied grid in the cell for quick reference from a lookup */
USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FGridDungeonCellInfo {
    GENERATED_USTRUCT_BODY()
    FGridDungeonCellInfo() : CellId(0), CellType(EGridDungeonCellType::Unknown), ContainsDoor(false) {
    }

    FGridDungeonCellInfo(int32 pCellId, EGridDungeonCellType pCellType) : CellId(pCellId), CellType(pCellType), ContainsDoor(false) {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    int32 CellId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    EGridDungeonCellType CellType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool ContainsDoor;
};

class DUNGEONARCHITECTRUNTIME_API FGridDungeonDoorManager {
public:
    FGridDungeonCellDoor CreateDoor(const FIntVector& p1, const FIntVector& p2, int cellId1, int cellId2) {
        if (doorLookupCache.Contains(p1) && doorLookupCache[p1].Contains(p2)) return doorLookupCache[p1][p2];
        if (doorLookupCache.Contains(p2) && doorLookupCache[p2].Contains(p1)) return doorLookupCache[p2][p1];

        // Create a new door
        FGridDungeonCellDoor door;
        door.AdjacentTiles[0] = p1;
        door.AdjacentTiles[1] = p2;
        door.AdjacentCells[0] = cellId1;
        door.AdjacentCells[1] = cellId2;

        if (!doorLookupCache.Contains(p1)) doorLookupCache.Add(p1, TMap<FIntVector, FGridDungeonCellDoor>());
        if (!doorLookupCache.Contains(p2)) doorLookupCache.Add(p2, TMap<FIntVector, FGridDungeonCellDoor>());

        if (!doorLookupCache[p1].Contains(p2)) doorLookupCache[p1].Add(p2, door);
        if (!doorLookupCache[p2].Contains(p1)) doorLookupCache[p2].Add(p1, door);

        doors.Add(door);
        return door;
    }

    FORCEINLINE const TArray<FGridDungeonCellDoor>& GetDoors() const { return doors; }
    FORCEINLINE TArray<FGridDungeonCellDoor>& GetDoors() { return doors; }

    void RemoveDoor(const FGridDungeonCellDoor& Door) {
        doors.Remove(Door);

        // Remove it from the cache

        TArray<FIntVector> Keys1, Keys2;
        doorLookupCache.GenerateKeyArray(Keys1);
        for (const FIntVector& Key1 : Keys1) {
            doorLookupCache[Key1].GenerateKeyArray(Keys2);
            for (const FIntVector& Key2 : Keys2) {
                FGridDungeonCellDoor TestDoor = doorLookupCache[Key1][Key2];
                if (TestDoor == Door) {
                    doorLookupCache[Key1].Remove(Key2);
                }
            }
        }
    }

    bool ContainsDoorBetweenCells(int cell0, int cell1) {
        for (const FGridDungeonCellDoor& door : doors) {
            if (!door.bEnabled) { continue; }
            if ((door.AdjacentCells[0] == cell0 && door.AdjacentCells[1] == cell1) ||
                (door.AdjacentCells[0] == cell1 && door.AdjacentCells[1] == cell0)) {
                return true;
            }
        }
        return false;
    }

    bool ContainsDoor(const FIntPoint& P1, const FIntPoint& P2) {
        return ContainsDoor(P1.X, P1.Y, P2.X, P2.Y);
    }
    
    bool ContainsDoor(int x1, int y1, int x2, int y2) {
        for (const auto& door : doors) {
            if (!door.bEnabled) { continue; }
            bool containsDoor =
                door.AdjacentTiles[0].X == x1 && door.AdjacentTiles[0].Y == y1 &&
                door.AdjacentTiles[1].X == x2 && door.AdjacentTiles[1].Y == y2;
            if (containsDoor) {
                return true;
            }

            containsDoor =
                door.AdjacentTiles[1].X == x1 && door.AdjacentTiles[1].Y == y1 &&
                door.AdjacentTiles[0].X == x2 && door.AdjacentTiles[0].Y == y2;
            if (containsDoor) {
                return true;
            }
        }
        return false;
    }

private:
    TMap<FIntVector, TMap<FIntVector, FGridDungeonCellDoor>> doorLookupCache;
    TArray<FGridDungeonCellDoor> doors;

};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FGridDungeonStairInfo {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    int32 OwnerCell = 0;

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    int32 ConnectedToCell = 0;

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    FQuat Rotation = FQuat::Identity;

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    FIntVector IPosition = FIntVector::ZeroValue;
};

/**
	* 
	*/
UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UGridDungeonModel : public UDungeonModel {
    GENERATED_BODY()
public:

    FGridDungeonDoorManager DoorManager;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Dungeon)
    DungeonModelBuildState BuildState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<FGridDungeonCell> Cells;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<FGridDungeonCellDoor> Doors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<FGridDungeonStairInfo> Stairs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TMap<int32, FGridDungeonCellClusterInfo> Clusters;
    
    void BuildCellLookup();

    FGridDungeonCell* GetCell(int32 Id);
    const FGridDungeonCell* GetCell(int32 Id) const;

    bool ContainsStairAtLocation(const FIntPoint& P);
    bool ContainsStairAtLocation(int x, int y);
    bool ContainsStairBetweenCells(int32 CellIdA, int32 CellIdB);

    TMap<int32, TArray<FGridDungeonStairInfo>> CellStairsLookup;
    TMap<int32, int32> CellIdToIndexLookup;
    TMap<int32, TMap<int32, FGridDungeonCellInfo>> GridCellInfoLookup;

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    FGridDungeonCellInfo GetGridCellLookup(int32 x, int32 y) const;

    FGridDungeonCellInfo GetGridCellLookup(const FIntPoint& P) const;

    virtual void Reset() override;
    virtual void GenerateLayoutDataImpl(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const override;
    virtual FDungeonFloorSettings CreateFloorSettings(const UDungeonConfig* InConfig) const override;
    
};

