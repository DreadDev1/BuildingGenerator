#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FloorData.h"
#include "CeilingData.generated.h"

UCLASS(BlueprintType)
class BUILDINGGENERATOR_API UCeilingData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CeilingData|Settings")
	EFloorFillMode FillMode = EFloorFillMode::Random;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CeilingData|Mesh")
	TArray<FFloorMeshEntry> CeilingMeshes;
};
