#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "FloorData.generated.h"

UENUM(BlueprintType)
enum class EFloorFillMode : uint8
{
	Random   UMETA(DisplayName = "Random"),
	Uniform  UMETA(DisplayName = "Uniform")
};

USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FFloorMeshEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorData|Mesh")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorData|Mesh")
	int32 CellsX = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorData|Mesh")
	int32 CellsY = 1;

	// For non-square meshes: both footprint orientations are always tried to ensure complete
	// coverage; this flag controls whether the starting orientation is randomized (true) or
	// always prefers CellsX×CellsY first (false). For square meshes: permits 0°/90°/180°/270°
	// aesthetic rotation to break up tiling patterns.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorData|Mesh")
	bool AllowRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorData|Mesh")
	float Weight = 1.f;
};

UCLASS(BlueprintType)
class BUILDINGGENERATOR_API UFloorData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FloorData|Settings")
	EFloorFillMode FillMode = EFloorFillMode::Random;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FloorData|Mesh")
	TArray<FFloorMeshEntry> FloorMeshes;
};
