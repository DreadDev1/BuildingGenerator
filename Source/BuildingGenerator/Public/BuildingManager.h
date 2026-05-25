#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomPlacement.h"
#include "BuildingManager.generated.h"

class AFloorManager;
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

	// All floors in this building. Assign AFloorManager actors placed in the level.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingManager|Layout")
	TArray<TObjectPtr<AFloorManager>> FloorManagers;

	// Seed used for all deterministic generation in this building.
	// Replicated to clients so they regenerate geometry locally from the same seed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "BuildingManager|Generation")
	int32 GenerationSeed = 0;

	// ----------------------------------------------------------------
	// Spawning — server only
	// ----------------------------------------------------------------

	// Spawns a MasterRoom and calls InitializeRoom on it. Returns null on clients.
	// Called by AFloorManager::GenerateLayout for each FRoomPlacement.
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

	// Calls GenerateLayout on every FloorManager in the FloorManagers array.
	UFUNCTION(CallInEditor, Category = "BuildingManager|Preview")
	void GenerateBuilding();

	// Calls ClearLayout on every FloorManager in the FloorManagers array.
	UFUNCTION(CallInEditor, Category = "BuildingManager|Preview")
	void ClearBuilding();
#endif // WITH_EDITOR
};
