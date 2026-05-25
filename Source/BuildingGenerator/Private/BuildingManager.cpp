#include "BuildingManager.h"
#include "FloorManager.h"
#include "MasterRoom.h"
#include "Net/UnrealNetwork.h"

ABuildingManager::ABuildingManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

// ============================================================
// Replication
// ============================================================

void ABuildingManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABuildingManager, GenerationSeed);
}

// ============================================================
// Spawning
// ============================================================

AMasterRoom* ABuildingManager::RequestRoomSpawn(
	TSubclassOf<AMasterRoom> RoomClass,
	const FTransform&        SpawnTransform,
	const FRoomPlacement&    Placement,
	int32                    FloorRoomHeightCm)
{
	if (!HasAuthority()) return nullptr;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMasterRoom* Room = GetWorld()->SpawnActor<AMasterRoom>(RoomClass, SpawnTransform, Params);
	if (Room)
	{
		Room->InitializeRoom(Placement, GenerationSeed, FloorRoomHeightCm);
	}
	return Room;
}

// ============================================================
// Editor
// ============================================================

#if WITH_EDITOR

void ABuildingManager::GenerateBuilding()
{
	for (TObjectPtr<AFloorManager>& Floor : FloorManagers)
	{
		if (IsValid(Floor))
		{
			Floor->GenerateLayout();
		}
	}
}

void ABuildingManager::ClearBuilding()
{
	for (TObjectPtr<AFloorManager>& Floor : FloorManagers)
	{
		if (IsValid(Floor))
		{
			Floor->ClearLayout();
		}
	}
}

#endif // WITH_EDITOR
