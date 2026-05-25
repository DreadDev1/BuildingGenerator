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

USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FForcedPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	FIntPoint CellPosition = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData|ForcedPlacement")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoomData|ForcedPlacement")
	TArray<FForcedPlacement> ForcedPlacements;
};
