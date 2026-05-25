#pragma once

#include "CoreMinimal.h"
#include "CellTypes.generated.h"

UENUM(BlueprintType)
enum class ECellType : uint8
{
	Floor	UMETA(DisplayName = "Floor"),
	Wall	UMETA(DisplayName = "Wall"),
	Corner	UMETA(DisplayName = "Corner")
};
