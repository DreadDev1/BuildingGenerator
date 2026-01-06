#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "RoomGenerationTypes.generated.h"

// Forward declaration for the MasterRoom to use this in USTRUCTs
class UWallData; 
class UFloorData;
class UDoorData;

// Defines Coordinate System: +X = North (Player Forward), +Y = East, -X = South, -Y = West
UENUM(BlueprintType)
enum class EWallEdge : uint8
{
	None 	UMETA(DisplayName = "None"),
	North 	UMETA(DisplayName = "North (+X)"),
	South 	UMETA(DisplayName = "South (-X)"),
	East 	UMETA(DisplayName = "East (+Y)"),
	West 	UMETA(DisplayName = "West (-Y)")
};

/* Corner positions in room (clockwise from bottom-left) */
UENUM(BlueprintType)
enum class ECornerPosition : uint8
{
	None,
	SouthWest = 0  UMETA(DisplayName = "Southwest (Bottom-Left)"),
	SouthEast = 1  UMETA(DisplayName = "Southeast (Bottom-Right)"),
	NorthEast = 2  UMETA(DisplayName = "Northeast (Top-Right)"),
	NorthWest = 3  UMETA(DisplayName = "Northwest (Top-Left)")
};

/* Side fill strategy for doorways smaller than standard width */
UENUM(BlueprintType)
enum class EDoorwaySideFill :  uint8
{
	None            UMETA(DisplayName = "None (Leave Empty)"),
	WallModules     UMETA(DisplayName = "Wall Modules (Bin-Packing)"),
	CustomMeshes    UMETA(DisplayName = "Custom Meshes"),
	CornerPieces    UMETA(DisplayName = "Corner Pieces")
};


// --- Mesh Placement Info  ---
USTRUCT(BlueprintType)
struct FMeshPlacementInfo
{
	GENERATED_BODY()

	// The actual mesh asset to be placed. TSoftObjectPtr is good for Data Assets.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Info")
	TSoftObjectPtr<UStaticMesh> MeshAsset; 

	// The size of the mesh footprint in 100cm cells (e.g., X=2, Y=4 for 200x400cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Info")
	FIntPoint GridFootprint = FIntPoint(1, 1);

	// Relative weight for randomization (NEW: Clamped between 0.0 and 10.0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Info", meta=(ClampMin="0.0", ClampMax="10.0", UIMin="0.0", UIMax="10.0"))
	float PlacementWeight = 1.0f; // Default remains 1.0f

	// If the mesh is non-square, define allowed rotations (e.g., 0 and 90)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Info")
	TArray<int32> AllowedRotations = {0}; 
};

/* Struct to track placed mesh information */
USTRUCT()
struct FPlacedMeshInfo
{
	GENERATED_BODY()

	// Grid position (top-left cell)
	UPROPERTY()
	FIntPoint GridPosition;

	// Size in cells
	UPROPERTY()
	FIntPoint GridFootprint;

	// Rotation angle (0, 90, 180, 270)
	UPROPERTY()
	int32 Rotation;

	// Mesh placement info from data asset
	UPROPERTY()
	FMeshPlacementInfo MeshInfo;

	// World transform for spawning
	UPROPERTY()
	FTransform LocalTransform;

	FPlacedMeshInfo() : GridPosition(FIntPoint:: ZeroValue), GridFootprint(FIntPoint::ZeroValue), Rotation(0) {}
};

// Struct for designer-defined rectangular empty regions
USTRUCT(BlueprintType)
struct FForcedEmptyRegion
{
	GENERATED_BODY()

	// First corner of the rectangular region (inclusive)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
	FIntPoint StartCell = FIntPoint:: ZeroValue;

	// Opposite corner of the rectangular region (inclusive)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
	FIntPoint EndCell = FIntPoint::ZeroValue;
};

// Wall segment tracking structure (matches MasterRoom's FWallSegmentInfo)
USTRUCT()
struct FGeneratorWallSegment
{
	GENERATED_BODY()

	EWallEdge Edge;
	int32 StartCell;
	int32 SegmentLength;
	FTransform BaseTransform;
	UStaticMesh* BaseMesh;
	const FWallModule* WallModule;  // Reference to module for Middle/Top

	FGeneratorWallSegment() : Edge(EWallEdge::North), StartCell(0), SegmentLength(0), BaseMesh(nullptr), WallModule(nullptr) {}
};

// Struct for complex wall modules (Base, Middle, Top)
USTRUCT(BlueprintType)
struct FWallModule
{
	GENERATED_BODY()

	// The length of this module in 100cm grid units (e.g., 2 for 200cm wall)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Info")
	int32 Y_AxisFootprint = 1;

	// Meshes that compose the module, using TSoftObjectPtr for async loading
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Meshes")
	TSoftObjectPtr<UStaticMesh> BaseMesh; 

	// Middle layer 1 (first middle layer, 100cm or 200cm tall)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Meshes")
	TSoftObjectPtr<UStaticMesh> MiddleMesh1;

	// Middle layer 2 (optional second middle layer, typically 100cm tall)
	// Only used if Middle1Mesh is also assigned
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Meshes")
	TSoftObjectPtr<UStaticMesh> MiddleMesh2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Meshes")
	TSoftObjectPtr<UStaticMesh> TopMesh;
	
	// Placement weight (NEW: Clamped between 0.0 and 10.0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Info", meta=(ClampMin="0.0", ClampMax="10.0", UIMin="0.0", UIMax="10.0"))
	float PlacementWeight = 1.0f;
};

// Placed wall info (for tracking spawned walls)
USTRUCT()
struct FPlacedWallInfo
{
	GENERATED_BODY()

	// Which edge this wall is on
	UPROPERTY()
	EWallEdge Edge;

	// Starting cell coordinate on that edge
	UPROPERTY()
	int32 StartCell;

	// Number of cells this wall spans
	UPROPERTY()
	int32 SpanLength;

	// Wall module used
	UPROPERTY()
	FWallModule WallModule;

	// World transforms for each mesh layer
	UPROPERTY()
	FTransform BottomTransform;

	UPROPERTY()
	FTransform Middle1Transform;

	UPROPERTY()
	FTransform Middle2Transform;

	UPROPERTY()
	FTransform TopTransform;

	FPlacedWallInfo()
		: Edge(EWallEdge::North)
		, StartCell(0)
		, SpanLength(0)
	{}
};

// --- Forced Wall Placement (Designer Override System) ---
USTRUCT(BlueprintType)
struct FForcedWallPlacement
{
	GENERATED_BODY()

	// Which edge of the room to place this wall on
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Placement")
	EWallEdge Edge = EWallEdge::North;

	// The starting cell index along this edge (0-based)
	// For North/South edges: index along Y-axis (0 to GridSize.Y-1)
	// For East/West edges: index along X-axis (0 to GridSize.X-1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Placement")
	int32 StartCell = 0;

	// The exact wall module to place (includes footprint, meshes, and all properties)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Placement")
	FWallModule WallModule;
};

// --- Door Position Offsets ---
USTRUCT(BlueprintType)
struct FDoorPositionOffsets
{
	GENERATED_BODY()

	// Offset for the door frame mesh (side pillars) from the wall base position
	// Useful for aligning frames that don't perfectly match wall thickness
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Offsets")
	FVector FramePositionOffset = FVector::ZeroVector;

	// Offset for the door actor (functional doorway) from the frame position
	// Useful for centering the door collision/trigger within the frame
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Offsets")
	FVector ActorPositionOffset = FVector::ZeroVector;
};

// --- Door Placement (Designer Override System) ---
USTRUCT(BlueprintType)
struct FFixedDoorLocation
{
	GENERATED_BODY()

	// Which wall edge the door should be placed on
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Placement")
	EWallEdge WallEdge = EWallEdge::North;

	// Starting cell position along the wall edge
	// For North/South walls: This is the Y coordinate (0 to GridSize.Y-1)
	// For East/West walls: This is the X coordinate (0 to GridSize.X-1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Placement")
	int32 StartCell = 0;

	// Door asset to use (reference to DoorData asset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Placement")
	UDoorData* DoorData = nullptr;

	// Position offsets for THIS specific door (allows individual door positioning)
	// If left at zero, no additional offset is applied (beyond base wall alignment)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Placement")
	FDoorPositionOffsets DoorPositionOffsets;
};

/* Doorway layout information (cached, no transforms) */
USTRUCT(BlueprintType)
struct FDoorwayLayoutInfo
{
	GENERATED_BODY()

	/* Which wall edge the doorway is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Layout")
	EWallEdge Edge = EWallEdge::North;

	/* Starting cell index on the edge */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Layout")
	int32 StartCell = 0;

	/* Width of doorway in cells */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Layout")
	int32 WidthInCells = 4;

	/* Door data asset to use */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Layout")
	UDoorData* DoorData = nullptr;

	/* Whether this is a standard (automatic) doorway */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Layout")
	bool bIsStandardDoorway = false;

	/* Manual offsets (only used for forced doorways) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Layout")
	FDoorPositionOffsets ManualOffsets;
};

USTRUCT(BlueprintType)
struct FPlacedDoorwayInfo
{
	GENERATED_BODY()

	// Which edge the doorway is on
	UPROPERTY()
	EWallEdge Edge = EWallEdge::North;

	// Starting cell on the edge
	UPROPERTY()
	int32 StartCell = 0;

	// Width in cells (from DoorData->FrameFootprintY)
	UPROPERTY()
	int32 WidthInCells = 4;

	// Door data used
	UPROPERTY()
	UDoorData* DoorData = nullptr;

	// Frame transform (local/component space)
	UPROPERTY()
	FTransform FrameTransform;

	// Actor spawn transform (local/component space - for future door actors)
	UPROPERTY()
	FTransform ActorTransform;

	// Whether this is an auto-generated standard doorway
	UPROPERTY()
	bool bIsStandardDoorway = false;

	FPlacedDoorwayInfo()
		: Edge(EWallEdge::North)
		, StartCell(0)
		, WidthInCells(4)
		, DoorData(nullptr)
		, bIsStandardDoorway(false)
	{}
};

/* Tracks a placed corner piece (single mesh, no stacking) */
USTRUCT(BlueprintType)
struct FPlacedCornerInfo
{
	GENERATED_BODY()

	// Corner position
	UPROPERTY()
	ECornerPosition Corner;

	// Corner mesh used
	UPROPERTY()
	TSoftObjectPtr<UStaticMesh> CornerMesh;

	// Corner transform (local/component space, relative to room origin)
	UPROPERTY()
	FTransform Transform;

	FPlacedCornerInfo()
		: Corner(ECornerPosition:: SouthWest)
	{}
};

/* Information about a placed ceiling tile */
USTRUCT(BlueprintType)
struct FPlacedCeilingInfo
{
	GENERATED_BODY()
	// Grid position (top-left cell)
	UPROPERTY()
	FIntPoint GridCoordinate;

	// Size in cells (tile size like 4x4, 2x2, 1x1)
	UPROPERTY()
	FIntPoint TileSize;

	// Rotation angle (0, 90, 180, 270) - if needed
	UPROPERTY()
	int32 Rotation;

	// Mesh placement info from data asset
	UPROPERTY()
	FMeshPlacementInfo MeshInfo;

	// World transform for spawning
	UPROPERTY()
	FTransform LocalTransform;  // ← CHANGED:  Match floor naming

	// Constructor with proper initialization
	FPlacedCeilingInfo() 
		: GridCoordinate(FIntPoint::  ZeroValue)
		, TileSize(FIntPoint:: ZeroValue)
		, Rotation(0)
	{}
};

/* Forced ceiling placement structure (designer override system) */
USTRUCT(BlueprintType)
struct FForcedCeilingPlacement
{
	GENERATED_BODY()

	/* Grid coordinate (top-left cell of placement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forced Placement")
	FIntPoint GridCoordinate = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forced Placement")
	FMeshPlacementInfo TileInfo; 
	
	/* Allowed rotations for this placement (0, 90, 180, 270) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forced Placement")
	TArray<int32> AllowedRotations;
};