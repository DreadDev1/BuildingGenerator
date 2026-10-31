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
// Visualizer's coordinate labels. CellsX x CellsY is the mesh's own size in cells (like a FloorData
// entry); a 90 / 270 RotationYaw swaps them, so the footprint always grows from the origin cell
// toward +X / +Y. The mesh is aligned by its bounds to the origin cell's corner, so a half-cell
// mesh (e.g. 0.5 x 4) sits flush against the origin cell's low edges.
USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FForcedPlacement
{
	GENERATED_BODY()

	// Origin cell of the footprint (lowest X,Y covered). Must be a floor cell, not the wall ring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	FIntPoint CellPosition = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	// The mesh's own size in cells along its X axis, before RotationYaw. 0 = work it out from the
	// mesh bounds (rounded up, so a 50 cm strip counts as 1 cell).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement", meta = (ClampMin = "0"))
	int32 CellsX = 0;

	// The mesh's own size in cells along its Y axis, before RotationYaw. 0 = from the mesh bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement", meta = (ClampMin = "0"))
	int32 CellsY = 0;

	// 0 / 90 / 180 / 270. At 90 / 270 the footprint is CellsY x CellsX. The rotated mesh is placed so
	// its bounds start at the origin cell's corner.
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
