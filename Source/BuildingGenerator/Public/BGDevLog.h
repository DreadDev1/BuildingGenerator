#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BGDevLog.generated.h"

// Developer-facing logging component.
// The UE_LOG path always compiles (survives packaging). Only the on-screen
// overlay is gated behind #if WITH_EDITOR. Visual grid debugging lives in the
// separate, editor-only UBGVisualizer component.

UENUM(BlueprintType)
enum class EBGLogLevel : uint8
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

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BUILDINGGENERATOR_API UBGDevLog : public UActorComponent
{
	GENERATED_BODY()

public:
	UBGDevLog();

#pragma region Core Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Logging")
	bool bEnableDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Logging")
	EBGLogLevel CurrentLogLevel = EBGLogLevel::Important;
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

	UFUNCTION(BlueprintCallable, Category = "Debug|Performance")
	void BeginPerformanceLog(const FString& OperationName);

	UFUNCTION(BlueprintCallable, Category = "Debug|Performance")
	void EndPerformanceLog(const FString& OperationName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Debug|Performance")
	TArray<FBGPerformanceLog> GetPerformanceLogs() const { return PerformanceLogs; }

	UFUNCTION(BlueprintCallable, Category = "Debug|Performance")
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

private:
	FString OwnerActorName;
	TMap<FString, double> ActivePerformanceTimers;
	TArray<FBGPerformanceLog> PerformanceLogs;

	void LogInternal(EBGLogLevel Level, EBGLogCategory Category, const FString& Message);
	bool ShouldLog(EBGLogLevel Level, EBGLogCategory Category) const;
	FString GetCategoryString(EBGLogCategory Category) const;
	FString GetOwnerPrefix() const;
	FColor GetColorForLevel(EBGLogLevel Level) const;

	// Pushes a message to the on-screen overlay. No-op outside the editor.
	void ScreenLog(const FColor& Color, const FString& Message) const;
};
