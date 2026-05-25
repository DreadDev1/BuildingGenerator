#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CellTypes.h"
#include "DebugLog.generated.h"

UENUM(BlueprintType)
enum class EDebugLogLevel : uint8
{
	None        = 0  UMETA(DisplayName = "None (No Logging)"),
	Critical    = 1  UMETA(DisplayName = "Critical Only"),
	Important   = 2  UMETA(DisplayName = "Important Messages"),
	Verbose     = 3  UMETA(DisplayName = "Verbose"),
	Everything  = 4  UMETA(DisplayName = "Everything")
};

UENUM(BlueprintType)
enum class EBGLogCategory : uint8
{
	Generation  UMETA(DisplayName = "Generation"),
	Grid        UMETA(DisplayName = "Grid"),
	Mesh        UMETA(DisplayName = "Mesh"),
	Wall        UMETA(DisplayName = "Wall"),
	Door        UMETA(DisplayName = "Door"),
	Replication UMETA(DisplayName = "Replication"),
	Performance UMETA(DisplayName = "Performance"),
	General     UMETA(DisplayName = "General")
};

USTRUCT(BlueprintType)
struct BUILDINGGENERATOR_API FBGPerformanceLog
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	FString OperationName;

	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	double StartTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	double EndTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	double DurationMs = 0.0;
};

#if WITH_EDITORONLY_DATA
class UTextRenderComponent;
#endif

#if WITH_EDITOR
DECLARE_DELEGATE_RetVal_FourParams(UTextRenderComponent*, FOnCreateTextComponent, FVector, FString, FColor, float);
DECLARE_DELEGATE_OneParam(FOnDestroyTextComponent, UTextRenderComponent*);
#endif

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BUILDINGGENERATOR_API UDebugLog : public UActorComponent
{
	GENERATED_BODY()

public:
	UDebugLog();

#pragma region Core Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Core")
	bool bEnableDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Logging")
	EDebugLogLevel CurrentLogLevel = EDebugLogLevel::Important;
#pragma endregion

#pragma region Screen Logging
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Screen")
	bool bEnableScreenLogging = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Screen", meta = (EditCondition = "bEnableScreenLogging"))
	float ScreenLogDuration = 5.f;
#pragma endregion

#pragma region Category Filtering
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Filtering")
	bool bEnableCategoryFiltering = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Filtering", meta = (EditCondition = "bEnableCategoryFiltering"))
	TSet<EBGLogCategory> EnabledCategories;
#pragma endregion

#pragma region Performance Profiling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance")
	bool bEnablePerformanceProfiling = true;

	UFUNCTION(BlueprintCallable, Category = "DebugLog|Performance")
	void BeginPerformanceLog(const FString& OperationName);

	UFUNCTION(BlueprintCallable, Category = "DebugLog|Performance")
	void EndPerformanceLog(const FString& OperationName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DebugLog|Performance")
	TArray<FBGPerformanceLog> GetPerformanceLogs() const { return PerformanceLogs; }

	UFUNCTION(BlueprintCallable, Category = "DebugLog|Performance")
	void ClearPerformanceLogs();
#pragma endregion

#pragma region Logging API
	void LogCritical(const FString& Message);
	void LogImportant(const FString& Message, EBGLogCategory Category = EBGLogCategory::General);
	void LogVerbose(const FString& Message, EBGLogCategory Category = EBGLogCategory::General);
	void LogStatistic(const FString& Label, const FString& Value, EBGLogCategory Category = EBGLogCategory::General);
	void LogStatistic(const FString& Label, int32 Value, EBGLogCategory Category = EBGLogCategory::General);
	void LogStatistic(const FString& Label, float Value, EBGLogCategory Category = EBGLogCategory::General);
	void LogSectionHeader(const FString& Title);
#pragma endregion

#if WITH_EDITORONLY_DATA
#pragma region Visualization Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bShowGrid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bShowCellStates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bShowCoordinates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	FColor GridColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float GridLineThickness = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float GridLineLifetime = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float CellBoxZOffset = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Appearance")
	float CellBoxThickness = 3.f;
#pragma endregion

#pragma region Cell Colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor FloorCellColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor WallCellColor = FColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor CornerCellColor = FColor(128, 0, 128);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor DoorCellColor = FColor::Green;
#pragma endregion

#pragma region Coordinate Text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Coordinates")
	FColor CoordinateTextColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Coordinates", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float CoordinateTextScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Coordinates", meta = (ClampMin = "0.0"))
	float CoordinateTextHeight = 30.f;

	UPROPERTY()
	TArray<UTextRenderComponent*> CoordinateTextComponents;
#pragma endregion
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	FOnCreateTextComponent OnCreateTextComponent;
	FOnDestroyTextComponent OnDestroyTextComponent;

#pragma region Visual Debug API
	void DrawGrid(FIntPoint GridSize, const TArray<ECellType>& CellStates, float CellSize, FVector OriginLocation);
	void DrawGridLines(FIntPoint GridSize, float CellSize, FVector OriginLocation);
	void DrawCellStates(FIntPoint GridSize, const TArray<ECellType>& CellStates, float CellSize, FVector OriginLocation);
	void DrawCellBox(FIntPoint GridCoord, FColor Color, float CellSize, FVector OriginLocation, float ZOffset);
	void DrawGridCoordinatesWithTextComponents(FIntPoint GridSize, float CellSize, FVector OriginLocation);
	void ClearCoordinateTextComponents();
	void ClearDebugDrawings();
#pragma endregion
#endif // WITH_EDITOR

private:
	FString OwnerActorName;
	TMap<FString, double> ActivePerformanceTimers;
	TArray<FBGPerformanceLog> PerformanceLogs;

	void LogInternal(EDebugLogLevel Level, EBGLogCategory Category, const FString& Message);
	bool ShouldLog(EDebugLogLevel Level, EBGLogCategory Category) const;
	FString GetCategoryString(EBGLogCategory Category) const;
	FString GetOwnerPrefix() const;
	FColor GetColorForLevel(EDebugLogLevel Level) const;

#if WITH_EDITOR
	FColor GetColorForCellType(ECellType CellType) const;
	FVector GridToWorldPosition(FIntPoint GridCoord, float CellSize, FVector OriginLocation) const;
#endif
};
