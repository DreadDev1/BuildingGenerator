#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "FloorData.h"
#include "CeilingData.h"
#include "WallData.h"
#include "DoorData.h"
#include "RoomData.generated.h"

UENUM(BlueprintType)
enum class ERoomGenerationType : uint8
{
	Uniform		UMETA(DisplayName = "Uniform"),
	BSP			UMETA(DisplayName = "BSP")
};

// A designer-chosen floor mesh pinned to a grid cell. Forced placements are placed before the random
// floor fill, which then fills the remaining floor cells around them.
// CellPosition is the footprint's origin cell: the lowest (X,Y) it covers, as shown by the
// Visualizer's coordinate labels. CellsX x CellsY is the footprint as it sits in the room.
USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FForcedPlacement
{
	GENERATED_BODY()

	// Origin cell of the footprint (lowest X,Y covered). Must be a floor cell, not the wall ring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	FIntPoint CellPosition = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	// Footprint size in cells along the room's X axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement", meta = (ClampMin = "1"))
	int32 CellsX = 1;

	// Footprint size in cells along the room's Y axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement", meta = (ClampMin = "1"))
	int32 CellsY = 1;

	// Turns the mesh around the footprint centre (use 0 / 90 / 180 / 270). CellsX / CellsY stay the
	// footprint in the room, so a 2x4 mesh turned 90 degrees is entered as CellsX 4, CellsY 2.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	float RotationYaw = 0.f;
};

UCLASS(BlueprintType)
class BUILDINGGENERATOR_API URoomData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|References")
	TObjectPtr<UFloorData> FloorData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|References")
	TObjectPtr<UCeilingData> CeilingData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|References")
	TObjectPtr<UWallData> WallData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|References")
	TObjectPtr<UDoorData> DoorData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|Generation")
	ERoomGenerationType GenerationType = ERoomGenerationType::Uniform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|Size")
	FIntPoint MinRoomSize = FIntPoint(2, 2);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|Size")
	FIntPoint MaxRoomSize = FIntPoint(10, 10);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|Flavor")
	TArray<TObjectPtr<UStaticMesh>> FlavorMeshes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|Flavor")
	float FlavorDensity = 0.25f;

	// Applied to every room that uses this RoomData. For one room only, use FRoomPlacement::ForcedPlacements.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|ForcedPlacement")
	TArray<FForcedPlacement> ForcedPlacements;
};
