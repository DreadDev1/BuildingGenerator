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
// Floor Manager Spawning
// ============================================================

bool ABuildingManager::SpawnFloorManagersFromAsset()
{
	if (!IsValid(BuildingDataAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("ABuildingManager::SpawnFloorManagersFromAsset — BuildingDataAsset is not set"));
		return false;
	}

	if (BuildingDataAsset->Floors.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABuildingManager::SpawnFloorManagersFromAsset — BuildingDataAsset has no floors defined"));
		return false;
	}

	// Destroy any previously spawned floor managers and their rooms
	for (TObjectPtr<AFloorManager>& Floor : SpawnedFloorManagers)
	{
		if (IsValid(Floor))
		{
			Floor->ClearLayout();
			Floor->Destroy();
		}
	}
	SpawnedFloorManagers.Empty();

	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector BuildingOrigin   = GetActorLocation();
	float         CumulativeHeight = 0.f;

	for (int32 i = 0; i < BuildingDataAsset->Floors.Num(); i++)
	{
		const FFloorDefinition& Def = BuildingDataAsset->Floors[i];

		// Fall back to base AFloorManager if no subclass is specified
		TSubclassOf<AFloorManager> FloorClass = Def.FloorClass
			? Def.FloorClass
			: TSubclassOf<AFloorManager>(AFloorManager::StaticClass());

		const FTransform FloorTransform(
			FRotator::ZeroRotator,
			BuildingOrigin + FVector(0.f, 0.f, CumulativeHeight));

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AFloorManager* Floor = World->SpawnActor<AFloorManager>(FloorClass, FloorTransform, Params);
		if (IsValid(Floor))
		{
			Floor->FloorIndex     = i;
			Floor->RoomHeightCm   = Def.RoomHeightCm;
			Floor->FloorGridSize  = Def.FloorGridSize;
			Floor->RoomPlacements = Def.RoomPlacements;
			SpawnedFloorManagers.Add(Floor);
		}

		CumulativeHeight += static_cast<float>(Def.RoomHeightCm);
	}

	return SpawnedFloorManagers.Num() > 0;
}

// ============================================================
// Editor
// ============================================================

#if WITH_EDITOR

void ABuildingManager::GenerateBuilding()
{
	if (!SpawnFloorManagersFromAsset()) return;

	for (TObjectPtr<AFloorManager>& Floor : SpawnedFloorManagers)
	{
		if (IsValid(Floor))
		{
			Floor->PreviewLayout();
		}
	}
}

void ABuildingManager::SpawnBuilding()
{
	if (!SpawnFloorManagersFromAsset()) return;

	for (TObjectPtr<AFloorManager>& Floor : SpawnedFloorManagers)
	{
		if (IsValid(Floor))
		{
			Floor->GenerateLayoutWith(this);
		}
	}
}

void ABuildingManager::ClearBuilding()
{
	for (TObjectPtr<AFloorManager>& Floor : SpawnedFloorManagers)
	{
		if (IsValid(Floor))
		{
			Floor->ClearLayout();
			Floor->ClearPreview();
			Floor->Destroy();
		}
	}
	SpawnedFloorManagers.Empty();
}

#endif // WITH_EDITOR
