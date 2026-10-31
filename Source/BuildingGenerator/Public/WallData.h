#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "WallData.generated.h"

// Optional per-mesh adjustment for one slot of a wall or corner module.
// Lets a designer reuse a mesh in another orientation without duplicating the asset, e.g. stand a
// floor panel up as a wall panel (Pitch 90) or flip a base trim to use it as a top trim (Roll 180).
// Left at its defaults, the mesh is placed exactly as authored.
USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FWallMeshAdjust
{
	GENERATED_BODY()

	// Rotation applied in the mesh's own space before it is placed on the wall.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshAdjust")
	FRotator Rotation = FRotator::ZeroRotator;

	// Scale applied in the mesh's own space. A negative axis mirrors the mesh (Z = -1 turns it upside down).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshAdjust")
	FVector Scale = FVector::OneVector;

	// Only used when Rotation or Scale is changed. Re-seats the adjusted mesh so it lines up like a
	// converted kit mesh: wall slots get their back face on X = 0, centred along Y, bottom on Z = 0;
	// corner slots are centred on the corner cell with their bottom on Z = 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshAdjust")
	bool bAutoAlign = true;

	// Extra nudge applied last, in module space (X = into the room, Y = along the face, Z = up).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshAdjust")
	FVector Offset = FVector::ZeroVector;

	bool HasRotationOrScale() const
	{
		return !Rotation.IsNearlyZero() || !Scale.Equals(FVector::OneVector);
	}
};

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

	// Single-piece module (e.g. a full-height pillar used as a wall): only BaseMesh is placed.
	// TopMesh and the middle meshes are not required and are ignored.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes")
	bool bUseBaseMeshOnly = false;

	// Required — generation skips any module with a null BaseMesh, or a null TopMesh unless bUseBaseMeshOnly is set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes")
	TObjectPtr<UStaticMesh> BaseMesh;

	// Optional. Leave null for a 2-piece stack (Base + Top only).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes", meta = (EditCondition = "!bUseBaseMeshOnly"))
	TObjectPtr<UStaticMesh> MiddleMesh1;

	// Optional. Only used when MiddleMesh1 is also set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes", meta = (EditCondition = "!bUseBaseMeshOnly"))
	TObjectPtr<UStaticMesh> MiddleMesh2;

	// Required unless bUseBaseMeshOnly is set — see BaseMesh note above.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Meshes", meta = (EditCondition = "!bUseBaseMeshOnly"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Heights", meta = (ClampMin = "0.0", EditCondition = "!bUseBaseMeshOnly"))
	float MiddleMesh1StackHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Heights", meta = (ClampMin = "0.0", EditCondition = "!bUseBaseMeshOnly"))
	float MiddleMesh2StackHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Placement", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	// Per-slot adjustments (rotation / mirror / nudge). Stack heights use the adjusted size, so a
	// floor panel stood upright stacks by its new height. The StackHeight overrides still win.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Adjust")
	FWallMeshAdjust BaseMeshAdjust;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Adjust", meta = (EditCondition = "!bUseBaseMeshOnly"))
	FWallMeshAdjust MiddleMesh1Adjust;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Adjust", meta = (EditCondition = "!bUseBaseMeshOnly"))
	FWallMeshAdjust MiddleMesh2Adjust;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallModule|Adjust", meta = (EditCondition = "!bUseBaseMeshOnly"))
	FWallMeshAdjust TopMeshAdjust;
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

	// Legacy single-mesh corner. Only used when CornerModule.BaseMesh is empty.
	// Single corner piece placed at all four room corners (ECellType::Corner cells).
	// Pivot: BottomCenter of a 100×100cm footprint. The mesh is rotated 0°/90°/180°/270°
	// per corner so the interior-facing side always points toward the room center.
	// Leave null to skip corner geometry for this wall style.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallData|Corner")
	TObjectPtr<UStaticMesh> CornerMesh;

	// Corner built like a wall module: Base / Middle1 / Middle2 / Top with per-slot adjustments and
	// stack heights. Replaces CornerMesh when its BaseMesh is set. For a single-piece corner set
	// BaseMesh and tick bUseBaseMeshOnly. CellsWide and Weight are ignored.
	// Placed at the centre of each corner cell and rotated 0/90/180/270 like CornerMesh. At 0 (the SW
	// corner) the room interior is toward +X/+Y; PlacementOffset is in that corner space.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallData|Corner")
	FWallModule CornerModule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallData|Exterior")
	bool bIsExteriorCapable = false;
};
