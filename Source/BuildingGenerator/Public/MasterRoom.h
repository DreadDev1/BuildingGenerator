#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CellTypes.h"
#include "RoomPlacement.h"
#include "MasterRoom.generated.h"

class UDebugLog;
class UInstancedStaticMeshComponent;
class UStaticMesh;

UCLASS(Blueprintable, ClassGroup = "BuildingGenerator")
class BUILDINGGENERATOR_API AMasterRoom : public AActor
{
	GENERATED_BODY()

public:
	AMasterRoom();

	// ----------------------------------------------------------------
	// Initialization
	// ----------------------------------------------------------------

	// Called by ABuildingManager immediately after spawn.
	UFUNCTION(BlueprintCallable, Category = "MasterRoom|Generation")
	void InitializeRoom(const FRoomPlacement& Placement, int32 Seed, int32 FloorRoomHeightCm);

	// ----------------------------------------------------------------
	// ISMCs — owned geometry components
	// ----------------------------------------------------------------

	// One ISMC per unique static mesh encountered during floor generation.
	// Populated dynamically by PlaceFloorMeshes — empty until first generation.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MasterRoom|Meshes")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> FloorISMCPool;

	// One ISMC per unique static mesh encountered during ceiling generation.
	// Populated dynamically by PlaceCeilingMeshes — empty until first generation.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MasterRoom|Meshes")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> CeilingISMCPool;

	// One ISMC per unique static mesh encountered during wall generation.
	// Populated dynamically by PlaceWallMeshStacks — empty until first generation.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MasterRoom|Meshes")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> WallISMCPool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MasterRoom|Meshes")
	TObjectPtr<UInstancedStaticMeshComponent> FlavorISMC;

	// ----------------------------------------------------------------
	// Runtime state (set by InitializeRoom)
	// ----------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "MasterRoom|Layout")
	FRoomPlacement ActivePlacement;

	UPROPERTY(BlueprintReadOnly, Category = "MasterRoom|Layout")
	int32 GenerationSeed = 0;

	// Height of this room in cm — driven by AFloorManager::RoomHeightCm.
	UPROPERTY(BlueprintReadOnly, Category = "MasterRoom|Layout")
	int32 RoomHeightCm = 300;

	// Resolved grid size in cells: SizeOverride if set, otherwise random from RoomData Min/Max.
	UPROPERTY(BlueprintReadOnly, Category = "MasterRoom|Layout")
	FIntPoint RoomGridSize = FIntPoint(4, 4);

	// Flat cell array, row-major: Index = Y * RoomGridSize.X + X
	UPROPERTY(BlueprintReadOnly, Category = "MasterRoom|Layout")
	TArray<ECellType> CellGrid;

	// ----------------------------------------------------------------
	// Debug component
	// ----------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MasterRoom|Debug")
	TObjectPtr<UDebugLog> DebugLog;

	// ----------------------------------------------------------------
	// Exterior wall suppression (called by ABuildingManager)
	// ----------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "MasterRoom|Generation")
	void SuppressExteriorWallISM(ERoomFace Face);

protected:
	// ----------------------------------------------------------------
	// 8-step generation sequence
	// ----------------------------------------------------------------

	// Step 1 — allocate CellGrid sized to RoomGridSize
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void BuildGrid();
	virtual void BuildGrid_Implementation();

	// Step 2 — edges → Wall, corners → Corner, interior → Floor
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void ClassifyCells();
	virtual void ClassifyCells_Implementation();

	// Step 3 — weighted random floor mesh fill
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void PlaceFloorMeshes();
	virtual void PlaceFloorMeshes_Implementation();

	// Step 4 — mirror floor pass at RoomHeightCm Z offset
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void PlaceCeilingMeshes();
	virtual void PlaceCeilingMeshes_Implementation();

	// Step 5 — wall stack per WallCell; door mesh at door cells
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void PlaceWallMeshStacks();
	virtual void PlaceWallMeshStacks_Implementation();

	// Step 6 — apply URoomData::ForcedPlacements overrides
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void ApplyForcedPlacements();
	virtual void ApplyForcedPlacements_Implementation();

	// Step 7 — scatter flavor meshes at FlavorDensity
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void PlaceFlavorMeshes();
	virtual void PlaceFlavorMeshes_Implementation();

	// Step 8 — spawn replicated UBoxComponent door triggers
	UFUNCTION(BlueprintNativeEvent, Category = "MasterRoom|Generation")
	void SpawnDoorTriggers();
	virtual void SpawnDoorTriggers_Implementation();

	// ----------------------------------------------------------------
	// Helpers
	// ----------------------------------------------------------------

	// Returns the resolved grid size from SizeOverride or RoomData bounds.
	FIntPoint ResolveRoomGridSize(FRandomStream& Stream) const;

	// Returns true if the cell at (X, Y) is declared as a door cell in ActivePlacement.
	bool IsDoorCell(int32 X, int32 Y) const;

	// Safe index into CellGrid.
	int32 CellIndex(int32 X, int32 Y) const { return Y * RoomGridSize.X + X; }

	// Returns which room face a Wall cell sits on. Used by SuppressExteriorWallISM (Step 11).
	ERoomFace GetFaceForWallCell(int32 X, int32 Y) const;

	// Component-local FTransform for a BottomBackCenter-pivoted wall module.
	// FacePos   — start position along the face (X for N/S faces, Y for E/W faces), in cell units.
	// CellsWide — module width along the face in cell units.
	// PlacementOffset — in wall-forward space (X=into-room, Y=along-face, Z=up); rotated by face yaw.
	FTransform MakeWallTransform(int32 FacePos, ERoomFace Face, int32 CellsWide, float Z, const FVector& PlacementOffset) const;

private:
	void GenerateRoomInterior();

	// ----------------------------------------------------------------
	// Seed offsets — XOR with GenerationSeed per step so each step's
	// FRandomStream is independent regardless of how many values the
	// previous step consumed.
	// ----------------------------------------------------------------
	static constexpr int32 SeedOffset_BuildGrid   = 0x1000;
	static constexpr int32 SeedOffset_FloorFill   = 0x2000;
	static constexpr int32 SeedOffset_CeilingFill = 0x3000;
	static constexpr int32 SeedOffset_WallFill    = 0x4000;
	static constexpr int32 SeedOffset_FlavorFill  = 0x5000;

	// ----------------------------------------------------------------
	// Mesh fill helpers — shared by floor, ceiling, and future steps
	// ----------------------------------------------------------------

	// Returns true if a mesh footprint (CellsX × CellsY) fits at (X, Y):
	// every covered cell must be ECellType::Floor and unclaimed.
	bool CanPlaceMesh(int32 X, int32 Y, int32 CellsX, int32 CellsY, const TArray<bool>& Claimed) const;

	// Marks all cells in the (CellsX × CellsY) footprint at (X, Y) as claimed.
	void ClaimCells(int32 X, int32 Y, int32 CellsX, int32 CellsY, TArray<bool>& Claimed);

	// Returns a component-local FTransform for a mesh with a BottomCenter pivot
	// covering a (CellsX × CellsY) footprint whose origin cell is (X, Y).
	// Z defaults to 0 (floor). Pass RoomHeightCm for ceiling placement.
	FTransform MakeCellTransform(int32 X, int32 Y, int32 CellsX, int32 CellsY, float Yaw = 0.f, float Z = 0.f) const;

	// Returns an existing ISMC from FloorISMCPool whose static mesh matches Mesh,
	// or creates, attaches, and registers a new one if none exists.
	UInstancedStaticMeshComponent* GetOrCreateFloorISMC(UStaticMesh* Mesh);

	// Returns an existing ISMC from CeilingISMCPool whose static mesh matches Mesh,
	// or creates, attaches, and registers a new one if none exists.
	UInstancedStaticMeshComponent* GetOrCreateCeilingISMC(UStaticMesh* Mesh);

	// Returns an existing ISMC from WallISMCPool whose static mesh matches Mesh,
	// or creates, attaches, and registers a new one if none exists.
	UInstancedStaticMeshComponent* GetOrCreateWallISMC(UStaticMesh* Mesh);

	// Returns Door.DoorDataOverride if set; otherwise returns RoomData's default DoorData.
	// May return null if neither is set — callers must guard with IsValid().
	const UDoorData* GetEffectiveDoorData(const FDoorPlacement& Door) const;

#if WITH_EDITORONLY_DATA
	// ----------------------------------------------------------------
	// In-editor preview (no PIE required)
	// Set SizeOverride on PreviewPlacement to get a fixed grid without
	// a RoomDataAsset. Enable bShowGrid / bShowCellStates on DebugLog.
	// ----------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "MasterRoom|Debug")
	FRoomPlacement PreviewPlacement;

	UPROPERTY(EditAnywhere, Category = "MasterRoom|Debug")
	int32 PreviewSeed = 12345;

	// Room height used during editor preview. Set this to match your wall stack height.
	// At runtime this is authoritative from AFloorManager::RoomHeightCm via InitializeRoom().
	UPROPERTY(EditAnywhere, Category = "MasterRoom|Debug")
	int32 PreviewRoomHeightCm = 300;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	void DrawDebugGrid() const;

	UFUNCTION(CallInEditor, Category = "MasterRoom|Preview")
	void PreviewRoom();

	UFUNCTION(CallInEditor, Category = "MasterRoom|Preview")
	void ClearPreview();
#endif // WITH_EDITOR
};
