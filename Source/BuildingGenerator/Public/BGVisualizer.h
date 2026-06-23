#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CellTypes.h"
#include "BGVisualizer.generated.h"

// Designer-facing debug visualization component.
//
// The ENTIRE class is gated behind #if WITH_EDITORONLY_DATA — nothing here ships
// in a packaged build, so it is zero-cost to audit. Developer-facing logging that
// must survive packaging lives in the separate UBGDevLog component.
//
// The structural cell model (ECellType: Floor/Wall/Corner) is left untouched.
// This component DERIVES a 6-type display state (EBGCellDisplayState) at draw
// time from the structural grid plus overlay cell lists supplied by the owner.

#if WITH_EDITORONLY_DATA

class UTextRenderComponent;

// 6-type display classification, derived at draw time (never stored on cells).
UENUM()
enum class EBGCellDisplayState : uint8
{
	Empty,      // interior floor cell, nothing placed        -> Blue
	Occupied,   // floor cell with a placed mesh (flavor etc.) -> Red
	Custom,     // floor cell overridden by a forced placement -> Yellow
	Void,       // floor cell reserved empty by the designer    -> Teal
	Wall,       // edge / corner cell                           -> Black
	Door        // wall cell within a door span                 -> Green
};

// A rectangular designer-reserved "leave empty" region, in cell coordinates
// (Min and Max are inclusive). Backing data arrives in Step 12; until then the
// draw path is exercised with empty arrays.
USTRUCT()
struct FBGForcedEmptyRegion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Debug")
	FIntPoint Min = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Debug")
	FIntPoint Max = FIntPoint::ZeroValue;
};

// Owner supplies a text render component (created/destroyed on the owning actor).
DECLARE_DELEGATE_RetVal_FourParams(UTextRenderComponent*, FOnCreateTextComponent, FVector /*WorldPos*/, FString /*Text*/, FColor /*Color*/, float /*Scale*/);
DECLARE_DELEGATE_OneParam(FOnDestroyTextComponent, UTextRenderComponent*);
// Fired by the Toggle* buttons so the owner can flush and redraw with current data.
DECLARE_DELEGATE(FOnRedrawRequested);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BUILDINGGENERATOR_API UBGVisualizer : public UActorComponent
{
	GENERATED_BODY()

public:
	UBGVisualizer();

#pragma region Toggles
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bEnableDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bShowGrid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bShowCellStates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bShowCoordinates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bShowForcedEmptyRegions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bShowForcedEmptyCells = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Show")
	bool bShowForcedPlacements = false;
#pragma endregion

#pragma region Cell Colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor EmptyCellColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor OccupiedCellColor = FColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor CustomCellColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor VoidCellColor = FColor(0, 150, 150);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor WallCellColor = FColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor DoorCellColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor ForcedEmptyRegionColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor ForcedEmptyCellBorderColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor ForcedPlacementColor = FColor::Magenta;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor GridColor = FColor::Green;

	// Outer boundary box color (floor-level grid).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor BoundaryColor = FColor::White;
#pragma endregion

#pragma region Appearance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float GridLineThickness = 2.f;

	// Negative = persistent in editor (cleared explicitly by ClearDebugDrawings).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float GridLineLifetime = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance", meta = (ClampMin = "0.0"))
	float CellBoxZOffset = 20.f;

	// Forced-empty overlay sits above the cell-state layer so both read at once.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance", meta = (ClampMin = "0.0"))
	float ForcedEmptyZOffset = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float CellBoxThickness = 3.f;
#pragma endregion

#pragma region Coordinate Text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Text")
	FColor CoordinateTextColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Text", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float CoordinateTextScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Text", meta = (ClampMin = "0.0"))
	float CoordinateTextHeight = 30.f;

	// World rotation applied to each coordinate label so it lies flat and reads from a
	// top-down view. With North=+X, a flat label cannot be both upright AND non-mirrored
	// from directly above, so this is exposed for one-time visual calibration in-editor.
	// Try Yaw 0/180 and Roll 0/180 if the default reads backwards or upside down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Text")
	FRotator CoordinateTextRotation = FRotator(90.f, 180.f, 0.f);

	// All text render components this visualizer has created, for cleanup.
	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> TextComponents;
#pragma endregion

#pragma region Delegates
	FOnCreateTextComponent  OnCreateTextComponent;
	FOnDestroyTextComponent OnDestroyTextComponent;
	FOnRedrawRequested      OnRedrawRequested;
#pragma endregion

#pragma region Toggle Buttons
	UFUNCTION(CallInEditor, Category = "Debug|Toggle")
	void ToggleGrid();

	UFUNCTION(CallInEditor, Category = "Debug|Toggle")
	void ToggleCoordinates();

	UFUNCTION(CallInEditor, Category = "Debug|Toggle")
	void ToggleCellStates();

	UFUNCTION(CallInEditor, Category = "Debug|Toggle")
	void ToggleForcedRegions();
#pragma endregion

#pragma region Room-level Draw (AMasterRoom)
	// High-level room draw. Derives the 6-type display state per cell from the
	// structural grid plus overlay cell lists (any may be empty).
	void DrawRoomGrid(
		FIntPoint GridSize,
		const TArray<ECellType>& StructuralCells,
		const TArray<FIntPoint>& DoorCells,
		const TArray<FIntPoint>& CustomCells,
		const TArray<FIntPoint>& VoidCells,
		const TArray<FIntPoint>& OccupiedCells,
		float CellSize,
		FVector OriginLocation);

	void DrawCellStates(
		FIntPoint GridSize,
		const TArray<ECellType>& StructuralCells,
		const TArray<FIntPoint>& DoorCells,
		const TArray<FIntPoint>& CustomCells,
		const TArray<FIntPoint>& VoidCells,
		const TArray<FIntPoint>& OccupiedCells,
		float CellSize,
		FVector OriginLocation);
#pragma endregion

#pragma region Forced Region Overlays
	void DrawForcedEmptyRegions(const TArray<FBGForcedEmptyRegion>& Regions, float CellSize, FVector OriginLocation);
	void DrawForcedEmptyCells(const TArray<FIntPoint>& Cells, float CellSize, FVector OriginLocation);
#pragma endregion

#pragma region Primitives (shared, e.g. AFloorManager)
	void DrawGridLines(FIntPoint GridSize, float CellSize, FVector OriginLocation);
	void DrawBoundaryBox(FIntPoint GridSize, float CellSize, FVector OriginLocation, float ZOffset);
	void DrawCellBox(FIntPoint GridCoord, FColor Color, float CellSize, FVector OriginLocation, float ZOffset);
	void DrawBox(FVector WorldCenter, FVector Extent, FColor Color, float Thickness);
	// Coordinate labels every CoordStep cells (1 = every cell).
	void DrawGridCoordinates(FIntPoint GridSize, float CellSize, FVector OriginLocation, int32 CoordStep = 1, float ZOffset = 0.f);
	// Creates a tracked world-space label via the owner's delegate. Returns null if unbound.
	UTextRenderComponent* CreateLabel(FVector WorldPosition, const FString& Text, FColor Color);
#pragma endregion

#pragma region Cleanup
	void ClearTextComponents();
	void ClearDebugDrawings();
#pragma endregion

private:
	EBGCellDisplayState DeriveDisplayState(
		ECellType Structural,
		const FIntPoint& Cell,
		const TSet<FIntPoint>& DoorCells,
		const TSet<FIntPoint>& CustomCells,
		const TSet<FIntPoint>& VoidCells,
		const TSet<FIntPoint>& OccupiedCells) const;

	FColor GetColorForDisplayState(EBGCellDisplayState State) const;
	FVector GridToWorldPosition(FIntPoint GridCoord, float CellSize, FVector OriginLocation) const;
};

#endif // WITH_EDITORONLY_DATA
