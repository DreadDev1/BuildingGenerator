#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomPlacement.h"
#include "FloorManager.generated.h"

class AMasterRoom;
class ABuildingManager;

UCLASS(Blueprintable, ClassGroup = "BuildingGenerator")
class BUILDINGGENERATOR_API AFloorManager : public AActor
{
	GENERATED_BODY()

public:
	AFloorManager();

	// ----------------------------------------------------------------
	// Layout configuration — set by designer in the Details panel
	// ----------------------------------------------------------------

	// All rooms on this floor. Positions (GridOrigin) are in cell units
	// relative to this FloorManager's local grid origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorManager|Layout")
	TArray<FRoomPlacement> RoomPlacements;

	// Height of every room on this floor in cm.
	// Passed to AMasterRoom::InitializeRoom() as FloorRoomHeightCm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorManager|Layout", meta = (ClampMin = "100"))
	int32 RoomHeightCm = 300;

	// Bounding grid for this floor in cells. PreviewLayout rejects any room
	// placement whose footprint extends beyond these bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorManager|Layout", meta = (ClampMin = "1"))
	FIntPoint FloorGridSize = FIntPoint(20, 20);

	// Which floor this is (0 = ground). Used to compute the world Z offset
	// relative to ABuildingManager. Each floor is offset by FloorIndex * RoomHeightCm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorManager|Layout", meta = (ClampMin = "0"))
	int32 FloorIndex = 0;

	// The BuildingManager that owns this floor. Must be set before calling GenerateLayout.
	// ABuildingManager assigns this when it spawns FloorManagers at runtime;
	// set it manually in the Details panel when testing a standalone FloorManager.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorManager|Layout")
	TObjectPtr<ABuildingManager> OwningBuildingManager;

	// ----------------------------------------------------------------
	// Runtime state
	// ----------------------------------------------------------------

	// Rooms spawned by GenerateLayout. Cleared by ClearLayout.
	UPROPERTY(BlueprintReadOnly, Category = "FloorManager|Runtime")
	TArray<TObjectPtr<AMasterRoom>> SpawnedRooms;

#if WITH_EDITOR
	// ----------------------------------------------------------------
	// Editor buttons
	// ----------------------------------------------------------------

	// Runs validation checks and draws a debug grid overlay.
	// No actors are spawned — safe to call at any time.
	// Validation rules are defined in CLAUDE.md Section 9.
	UFUNCTION(CallInEditor, Category = "FloorManager|Preview")
	void PreviewLayout();

	// Validates the layout then spawns all AMasterRoom actors and generates
	// their interior geometry. Calls ABuildingManager::RequestRoomSpawn().
	// Implemented in Step 7.
	UFUNCTION(CallInEditor, Category = "FloorManager|Preview")
	void GenerateLayout();

	// Destroys all AMasterRoom actors spawned by the last GenerateLayout call.
	UFUNCTION(CallInEditor, Category = "FloorManager|Preview")
	void ClearLayout();

	// Flushes all persistent debug lines and strings drawn by PreviewLayout.
	UFUNCTION(CallInEditor, Category = "FloorManager|Preview")
	void ClearPreview();
#endif // WITH_EDITOR
};
