#pragma once

#include "CoreMinimal.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "ChunkyRoomGenerator.generated.h"

// Forward declarations
enum class EWallEdge : uint8;

UCLASS()
class BUILDINGGENERATOR_API UChunkyRoomGenerator : public URoomGenerator
{
    GENERATED_BODY()

public:
    /** Override:  Create chunky grid pattern */
    virtual void CreateGrid() override;
    
#pragma region Room Generation Interface
    virtual bool GenerateFloor() override;
    virtual bool GenerateWalls() override;
    virtual bool GenerateCorners() override;
    virtual bool GenerateDoorways() override;
    virtual bool GenerateCeiling() override;
#pragma endregion

#pragma region ChunkyRoom generation parameters
    /** Minimum number of protrusions to add */
    UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "0", ClampMax = "20"))
    int32 MinProtrusions = 3;
    
    /** Maximum number of protrusions to add */
    UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "0", ClampMax = "20"))
    int32 MaxProtrusions = 8;
    
    /** Minimum protrusion size in chunks (1 chunk = 2×2 cells = 200cm) */
    UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "2", ClampMax = "10"))
    int32 MinProtrusionSizeChunks = 2;
    
    /** Maximum protrusion size in chunks */
    UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "2", ClampMax = "10"))
    int32 MaxProtrusionSizeChunks = 4;
    
    /** Percentage of grid used for base room (0.5 = 50%, 0.8 = 80%) */
    UPROPERTY(EditAnywhere, Category = "Chunky Generation", meta = (ClampMin = "0.3", ClampMax = "0.95"))
    float BaseRoomPercentage = 0.7f;
    
    /** Random seed for generation (-1 = random each time, 0+ = deterministic) */
    UPROPERTY(EditAnywhere, Category = "Chunky Generation")
    int32 RandomSeed = -1;
#pragma endregion
    
private:
#pragma region Internal Variables
    // ===== CHUNK-BASED (for generation) =====
    /** Chunk grid dimensions (1 chunk = 2×2 cells) */
    FIntPoint ChunkGridSize;
    
    /** Base room in chunk coordinates */
    FIntPoint BaseRoomStartChunks;
    FIntPoint BaseRoomSizeChunks;
    
    /** Chunk state (true = room, false = void) */
    TArray<bool> ChunkState;

    // ===== CELL-BASED (for external use - doorways/walls) =====
    /** Base room in cell coordinates */
    FIntPoint BaseRoomStart;
    FIntPoint BaseRoomSize;
    
    /** Random stream for generation */
    FRandomStream RandomStream;
    
    // ===== CORNER TRACKING ===== 
    /** Void cells occupied by interior corners (blocked from wall placement) */
    TSet<FIntPoint> CornerOccupiedCells;
#pragma endregion

#pragma region Chunk Helper Functions
    /** Mark a rectangular area of CHUNKS as room */
    void MarkChunkRectangle(int32 StartX, int32 StartY, int32 Width, int32 Height);
    
    /** Add a random chunk-aligned protrusion */
    void AddRandomProtrusionChunked();
    
    /** Convert chunk state to cell state (mark cells in room chunks as ECT_Custom) */
    void ConvertChunksToCells();
    
    /** Convert chunk coordinate to cell coordinate */
    FIntPoint ChunkToCell(FIntPoint ChunkCoord) const;
    
    /** Convert cell coordinate to chunk coordinate */
    FIntPoint CellToChunk(FIntPoint CellCoord) const;
#pragma endregion

#pragma region Wall/Floor Helper Functions
    bool HasFloorNeighbor(FIntPoint Cell, FIntPoint Direction) const;
    TArray<FIntPoint> GetPerimeterCells() const;
    FIntPoint GetDirectionOffset(EWallEdge Direction) const;
    TArray<FIntPoint> GetPerimeterCellsForEdge(EWallEdge Edge) const;
    void FillChunkyWallEdge(EWallEdge Edge);
    FVector CalculateWallPositionForSegment(EWallEdge Direction, FIntPoint StartCell, int32 ModuleFootprint,
        float NorthOffset, float SouthOffset, float EastOffset, float WestOffset) const;
#pragma endregion
    
#pragma region Corner Generation
    /** Generate interior corners (void cells with 2 adjacent Custom neighbors) */
    void GenerateInteriorCorners();
    
    /** Check if a void cell is an interior corner */
    bool IsVoidCornerCell(FIntPoint Cell, TArray<EWallEdge>& OutAdjacentFloorEdges) const;
#pragma endregion
    
};