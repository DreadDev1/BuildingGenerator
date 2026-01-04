// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Grid/GridData.h"
#include "DebugHelpers.generated.h"

struct FForcedEmptyRegion;
// Log verbosity levels
UENUM(BlueprintType)
enum class EDebugLogLevel : uint8
{
	None = 0       UMETA(DisplayName = "None (No Logging)"),
	Critical = 1   UMETA(DisplayName = "Critical Only"),
	Important = 2  UMETA(DisplayName = "Important Messages"),
	Verbose = 3    UMETA(DisplayName = "Verbose"),
	Everything = 4 UMETA(DisplayName = "Everything")
};

class UTextRenderComponent;

// Delegate for requesting text render components from owner
DECLARE_DELEGATE_RetVal_FourParams(UTextRenderComponent*, FOnCreateTextComponent, FVector /*WorldPosition*/, FString /*Text*/, FColor /*Color*/, float /*Scale*/);
// Delegate for destroying text render components
DECLARE_DELEGATE_OneParam(FOnDestroyTextComponent, UTextRenderComponent*);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BUILDINGGENERATOR_API UDebugHelpers : public UActorComponent
{
	GENERATED_BODY()

public: 
	// Sets default values for this component's properties
	UDebugHelpers();

	// Master switch for all debug functionality
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
	bool bEnableDebug = true;
	
#pragma region Debug Settings|Visualization
	// Show grid outline
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Visualization")
	bool bShowGrid = false;

	// Show cell states (occupied/empty)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Visualization")
	bool bShowCellStates = false;

	// Show coordinate text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Visualization")
	bool bShowCoordinates = false;

	// Show forced empty regions (designer overrides)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Visualization")
	bool bShowForcedEmptyRegions = false;

	// Show forced empty individual cells
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Visualization")
	bool bShowForcedEmptyCells = false;
	
	// Show forced placement meshes (designer overrides)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Visualization")
	bool bShowForcedPlacements = false;
#pragma endregion
	
#pragma region Visual Settings
	// Grid line color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor GridColor = FColor::Green;

	// Grid line thickness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Appearance")
	float GridLineThickness = 5.0f;

	// Grid line lifetime (negative = persistent in editor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Appearance")
	float GridLineLifetime = -1.0f;

	// Empty cell color (blue in MasterRoom)
	UPROPERTY(BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor EmptyCellColor = FColor::Blue;

	// Occupied cell color (red in MasterRoom)
	UPROPERTY(BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor OccupiedCellColor = FColor::Red;
	
	// Custom cell color (red in MasterRoom)
	UPROPERTY(BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor CustomCellColor = FColor::Yellow;

	// Custom cell color (red in MasterRoom)
	UPROPERTY(BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor VoidCellColor = FColor(0, 150, 150);
	
	// Forced empty region color (cyan in MasterRoom)
	UPROPERTY(BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor ForcedEmptyRegionColor = FColor:: Cyan;

	// Forced empty cell border color (orange in MasterRoom)
	UPROPERTY(BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor ForcedEmptyCellBorderColor = FColor::Orange;

	// Cell box Z-offset (height above ground)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Appearance", meta = (ClampMin = "0.0"))
	float CellBoxZOffset = 20.0f;

	// Forced empty region Z-offset (higher for visibility)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Appearance", meta = (ClampMin = "0.0"))
	float ForcedEmptyZOffset = 40.0f;
	
	// Wall boundary cell color (for Phase 3 - wall generation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor WallCellColor = FColor::Black;

	// Doorway cell color (for Phase 4 - door placement)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor DoorCellColor = FColor:: Green;

	// Forced placement mesh color (for designer overrides)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Colors")
	FColor ForcedPlacementColor = FColor:: Magenta;

	// Cell box thickness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Appearance")
	float CellBoxThickness = 3.0f;
#pragma endregion
	
#pragma region Logging Settings
	// Current debug log level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Logging")
	EDebugLogLevel CurrentLogLevel = EDebugLogLevel::Important;

	// Prefix for all log messages (automatically includes owner actor name)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug Settings|Logging")
	FString LogCategoryPrefix;
#pragma endregion

#pragma region Debug Drawing
	/* Draw complete grid visualization with all enabled features */
	void DrawGrid(FIntPoint GridSize, const TArray<EGridCellType>& GridState, float CellSize, FVector OriginLocation);

	/* Draw forced empty regions (rectangular areas) */
	void DrawForcedEmptyRegions(const TArray<FForcedEmptyRegion>& Regions, FIntPoint GridSize, float CellSize, FVector OriginLocation);

	/* Draw forced empty individual cells */
	void DrawForcedEmptyCells(const TArray<FIntPoint>& Cells, FIntPoint GridSize, float CellSize, FVector OriginLocation);

	/* Draw a single cell box at grid coordinate */
	void DrawCellBox(FIntPoint GridCoord, FColor Color, float CellSize, FVector OriginLocation, float ZOffset);

	/* Draw grid lines (X and Y axis) */
	void DrawGridLines(FIntPoint GridSize, float CellSize, FVector OriginLocation);

	/* Draw cell state boxes (red/blue for occupied/empty) */
	void DrawCellStates(FIntPoint GridSize, const TArray<EGridCellType>& GridState, float CellSize, FVector OriginLocation);
#pragma endregion
	
#pragma region Debuging Cleanup
	/* Clear all debug drawings (lines and boxes) */
	void ClearDebugDrawings();
#pragma endregion
	
#pragma region Debug Logging
	// Log a critical error (always shown, red)
	void LogCritical(const FString& Message);

	// Log an important message (warnings, yellow)
	void LogImportant(const FString& Message);

	// Log a statistic (formatted with label)
	void LogStatistic(const FString& Label, const FString& Value);
	void LogStatistic(const FString& Label, int32 Value);
	void LogStatistic(const FString& Label, float Value);

	// Log a verbose/debug message (only if log level allows)
	void LogVerbose(const FString& Message);

	// Log a section header (bookend style for major operations)
	void LogSectionHeader(const FString& Title);
#pragma endregion
	
#pragma region Grid Coordinate Text Rendering
	// Delegate for requesting text component creation from owner
	FOnCreateTextComponent OnCreateTextComponent;

	// Delegate for requesting text component destruction from owner
	FOnDestroyTextComponent OnDestroyTextComponent;
	
	// Coordinate text color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Text")
	FColor CoordinateTextColor = FColor::Orange;

	// Coordinate text scale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Text", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float CoordinateTextScale = 1.0f;

	// Height offset for coordinate text above grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings|Text", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CoordinateTextHeight = 30.0f;
	
	UPROPERTY()
	TArray<UTextRenderComponent*> CoordinateTextComponents;
	
	/*Draw coordinate text using text render components */
	void DrawGridCoordinatesWithTextComponents(FIntPoint GridSize, float CellSize, FVector OriginLocation);
	
	/* Clear coordinate text components */
	void ClearCoordinateTextComponents();
#pragma endregion
	
private:
#pragma region Internal Data
	// Owner actor name for logging
	FString OwnerActorName;
#pragma endregion
	
#pragma region Internal Helpers
	/* Get color for a specific cell type */
	FColor GetColorForCellType(EGridCellType CellType) const;

	/* Convert grid coordinate to world position (cell center) */
	FVector GridToWorldPosition(FIntPoint GridCoord, float CellSize, FVector OriginLocation) const;

	/* Get formatted log prefix with category and owner name */
	FString GetCategoryPrefix() const;

	/* Check if message should be logged based on current log level */
	bool ShouldLog(EDebugLogLevel MessageLevel) const;
#pragma endregion
};