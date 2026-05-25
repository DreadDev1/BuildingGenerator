#pragma once

#include "CoreMinimal.h"
#include "RoomData.h"
#include "DoorData.h"
#include "RoomPlacement.generated.h"

class AMasterRoom;

UENUM(BlueprintType)
enum class ERoomFace : uint8
{
	North	UMETA(DisplayName = "North  (+Y)"),
	South	UMETA(DisplayName = "South  (-Y)"),
	East	UMETA(DisplayName = "East   (+X)"),
	West	UMETA(DisplayName = "West   (-X)")
};

USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FDoorPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERoomFace Face = ERoomFace::North;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CellOffset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UDoorData> DoorDataOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsExteriorDoor = false;
};

USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FRoomPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AMasterRoom> RoomClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URoomData> RoomDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint GridOrigin = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint SizeOverride = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RotationDegrees = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDoorPlacement> Doors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEntranceRoom = false;
};
