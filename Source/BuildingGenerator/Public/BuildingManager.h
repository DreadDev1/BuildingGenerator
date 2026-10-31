#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingData.h"
#include "RoomPlacement.h"
#include "BuildingManager.generated.h"

class AMasterRoom;

UCLASS(Blueprintable, ClassGroup = "BuildingGenerator")
class BUILDINGGENERATOR_API ABuildingManager : public AActor
{
	GENERATED_BODY()

public:
	ABuildingManager();

	// ----------------------------------------------------------------
	// Layout configuration
	// ----------------------------------------------------------------

	// Building configuration DataAsset. Defines all floors and their room layouts.
	// Assign in the Details panel; GenerateBuilding and SpawnBuilding read from this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingManager|Layout")
	TObjectPtr<UBuildingData> BuildingDataAsset;

	// Seed used for all deterministic generation in this building.
	// Replicated to clients so they regenerate geometry locally from the same seed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "BuildingManager|Generation")
	int32 GenerationSeed = 0;

	// ----------------------------------------------------------------
	// Runtime state
	// ----------------------------------------------------------------

	// AFloorManager actors spawned from BuildingDataAsset. Populated by
	// GenerateBuilding / SpawnBuilding; destroyed by ClearBuilding. Read-only.
	UPROPERTY(BlueprintReadOnly, Category = "BuildingManager|Runtime")
	TArray<TObjectPtr<AFloorManager>> SpawnedFloorManagers;

	// ----------------------------------------------------------------
	// Spawning — server only
	// ----------------------------------------------------------------

	// Spawns a MasterRoom and calls InitializeRoom on it. Returns null on clients.
	// Called by AFloorManager::GenerateLayoutWith for each FRoomPlacement.
	UFUNCTION(BlueprintCallable, Category = "BuildingManager|Generation")
	AMasterRoom* RequestRoomSpawn(
		TSubclassOf<AMasterRoom> RoomClass,
		const FTransform&        SpawnTransform,
		const FRoomPlacement&    Placement,
		int32                    FloorRoomHeightCm);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	// ----------------------------------------------------------------
	// Editor buttons
	// ----------------------------------------------------------------

	// Spawns AFloorManager actors from BuildingDataAsset and draws the floor grid
	// for each. No room geometry is generated — use this to plan room placements.
	// All floor grids are visible simultaneously (debug lines have no depth occlusion).
	UFUNCTION(CallInEditor, Category = "BuildingManager|Preview")
	void GenerateBuilding();

	// Spawns AFloorManager actors from BuildingDataAsset and generates room geometry
	// on every floor. Call after planning layouts with GenerateBuilding.
	UFUNCTION(CallInEditor, Category = "BuildingManager|Preview")
	void SpawnBuilding();

	// Destroys all spawned floor managers, their rooms, and all floor grid previews.
	UFUNCTION(CallInEditor, Category = "BuildingManager|Preview")
	void ClearBuilding();
#endif // WITH_EDITOR

private:
	// Destroys any existing SpawnedFloorManagers, then spawns one AFloorManager per
	// FFloorDefinition in BuildingDataAsset, positioning each at the cumulative Z
	// offset of all floors below it. Returns false if BuildingDataAsset is null/empty.
	bool SpawnFloorManagersFromAsset();
};
