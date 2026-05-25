#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "WallData.generated.h"

// One wall module = one set of stacked meshes with a specific face-width footprint.
// UWallData holds a pool of these. The generator bin-packs them along each wall face
// using CellsWide for sizing and Weight for weighted random selection.
USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FWallModule
{
	GENERATED_BODY()

	// Width of this module along the wall face, in 100cm cells (1=100cm, 2=200cm, 4=400cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Footprint")
	int32 CellsWide = 1;

	// Required — generation skips any module with a null BaseMesh or TopMesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes")
	TObjectPtr<UStaticMesh> BaseMesh;

	// Optional. Leave null for a 2-piece stack (Base + Top only).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes")
	TObjectPtr<UStaticMesh> MiddleMesh1;

	// Optional. Only used when MiddleMesh1 is also set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes")
	TObjectPtr<UStaticMesh> MiddleMesh2;

	// Required — see BaseMesh note above.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes")
	TObjectPtr<UStaticMesh> TopMesh;

	// Applied in wall-forward space (X = depth into room, Y = along face, Z = up).
	// Corrects for meshes whose interior face does not land flush on the cell boundary.
	// Example: a mesh that is 135cm deep in a 100cm wall cell → set X = -35 to pull it flush.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Placement")
	FVector PlacementOffset = FVector::ZeroVector;

	// Stack height overrides — used instead of the mesh bounding box when non-zero.
	// Set when the bounding box does not match the intended stacking height.
	// Example: BaseMesh bounding box reports 135 but visually tops out at 125 → set BaseMeshStackHeight = 125.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Heights", meta = (ClampMin = "0.0"))
	float BaseMeshStackHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Heights", meta = (ClampMin = "0.0"))
	float MiddleMesh1StackHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Heights", meta = (ClampMin = "0.0"))
	float MiddleMesh2StackHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Placement", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

UCLASS(BlueprintType)
class BUILDINGGENERATOR_API UWallData : public UDataAsset
{
	GENERATED_BODY()

public:
	// Wall module pool for this style. Add one entry per distinct mesh width.
	// At least one module with CellsWide=1 is required so every cell can always be filled.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallData|Modules")
	TArray<FWallModule> WallModules;

	// Single corner piece placed at all four room corners (ECellType::Corner cells).
	// Pivot: BottomCenter of a 100×100cm footprint. The mesh is rotated 0°/90°/180°/270°
	// per corner so the interior-facing side always points toward the room center.
	// Leave null to skip corner geometry for this wall style.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallData|Corner")
	TObjectPtr<UStaticMesh> CornerMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallData|Exterior")
	bool bIsExteriorCapable = false;
};
