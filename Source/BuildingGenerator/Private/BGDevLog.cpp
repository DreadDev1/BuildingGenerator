#include "BGDevLog.h"

#include "BuildingGenerator/BuildingGenerator.h"
#include "Engine/Engine.h"

UBGDevLog::UBGDevLog()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================
// Internal Helpers
// ============================================================

void UBGDevLog::ScreenLog(const FColor& Color, const FString& Message) const
{
#if WITH_EDITOR
	if (bEnableScreenLogging && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, ScreenLogDuration, Color, Message);
	}
#endif
}

void UBGDevLog::LogInternal(EBGLogLevel Level, EBGLogCategory Category, const FString& Message)
{
	const FString FullMessage = FString::Printf(TEXT("%s [%s] %s"), *GetOwnerPrefix(), *GetCategoryString(Category), *Message);

	switch (Level)
	{
	case EBGLogLevel::Critical:
		UE_LOG(LogBuildingGenerator, Error, TEXT("%s"), *FullMessage);
		break;
	case EBGLogLevel::Important:
		UE_LOG(LogBuildingGenerator, Warning, TEXT("%s"), *FullMessage);
		break;
	default:
		UE_LOG(LogBuildingGenerator, Log, TEXT("%s"), *FullMessage);
		break;
	}

	ScreenLog(GetColorForLevel(Level), FullMessage);
}

bool UBGDevLog::ShouldLog(EBGLogLevel Level, EBGLogCategory Category) const
{
	if (!bEnableDebug) return false;
	if (static_cast<uint8>(Level) > static_cast<uint8>(CurrentLogLevel)) return false;
	if (bEnableCategoryFiltering && !EnabledCategories.Contains(Category)) return false;
	return true;
}

FString UBGDevLog::GetOwnerPrefix() const
{
	if (!OwnerActorName.IsEmpty())
	{
		return FString::Printf(TEXT("[%s]"), *OwnerActorName);
	}

	const AActor* Owner = GetOwner();
	return Owner
		? FString::Printf(TEXT("[%s]"), *Owner->GetName())
		: FString(TEXT("[Unknown]"));
}

FString UBGDevLog::GetCategoryString(EBGLogCategory Category) const
{
	switch (Category)
	{
	case EBGLogCategory::Generation:    return TEXT("GEN");
	case EBGLogCategory::Grid:          return TEXT("GRID");
	case EBGLogCategory::Mesh:          return TEXT("MESH");
	case EBGLogCategory::Wall:          return TEXT("WALL");
	case EBGLogCategory::Door:          return TEXT("DOOR");
	case EBGLogCategory::Replication:   return TEXT("NET");
	case EBGLogCategory::Performance:   return TEXT("PERF");
	case EBGLogCategory::General:       return TEXT("GENERAL");
	default:                            return TEXT("UNKNOWN");
	}
}

FColor UBGDevLog::GetColorForLevel(EBGLogLevel Level) const
{
	switch (Level)
	{
	case EBGLogLevel::Critical:      return FColor::Red;
	case EBGLogLevel::Important:     return FColor::Yellow;
	case EBGLogLevel::Verbose:       return FColor::White;
	case EBGLogLevel::Everything:    return FColor::Silver;
	default:                         return FColor::White;
	}
}

// ============================================================
// Public Logging API
// ============================================================

void UBGDevLog::LogCritical(const FString& Message)
{
	// Bypasses all filters — always logs.
	const FString FullMessage = FString::Printf(TEXT("%s [CRITICAL] %s"), *GetOwnerPrefix(), *Message);
	UE_LOG(LogBuildingGenerator, Error, TEXT("%s"), *FullMessage);

	ScreenLog(FColor::Red, FullMessage);
}

void UBGDevLog::LogImportant(const FString& Message, EBGLogCategory Category)
{
	if (!ShouldLog(EBGLogLevel::Important, Category)) return;
	LogInternal(EBGLogLevel::Important, Category, Message);
}

void UBGDevLog::LogVerbose(const FString& Message, EBGLogCategory Category)
{
	if (!ShouldLog(EBGLogLevel::Verbose, Category)) return;
	LogInternal(EBGLogLevel::Verbose, Category, Message);
}

void UBGDevLog::LogStatistic(const FString& Label, const FString& Value, EBGLogCategory Category)
{
	if (!ShouldLog(EBGLogLevel::Important, Category)) return;
	LogInternal(EBGLogLevel::Important, Category, FString::Printf(TEXT("%s: %s"), *Label, *Value));
}

void UBGDevLog::LogStatistic(const FString& Label, int32 Value, EBGLogCategory Category)
{
	LogStatistic(Label, FString::FromInt(Value), Category);
}

void UBGDevLog::LogStatistic(const FString& Label, float Value, EBGLogCategory Category)
{
	LogStatistic(Label, FString::Printf(TEXT("%.2f"), Value), Category);
}

void UBGDevLog::LogSectionHeader(const FString& Title)
{
	if (!ShouldLog(EBGLogLevel::Important, EBGLogCategory::General)) return;

	const FString FullMessage = FString::Printf(TEXT("%s === %s ==="), *GetOwnerPrefix(), *Title);
	UE_LOG(LogBuildingGenerator, Warning, TEXT("%s"), *FullMessage);

	ScreenLog(FColor::Cyan, FullMessage);
}

// ============================================================
// Performance Profiling
// ============================================================

void UBGDevLog::BeginPerformanceLog(const FString& OperationName)
{
	if (!bEnablePerformanceProfiling) return;
	ActivePerformanceTimers.Add(OperationName, FPlatformTime::Seconds());
	LogVerbose(FString::Printf(TEXT("Started: %s"), *OperationName), EBGLogCategory::Performance);
}

void UBGDevLog::EndPerformanceLog(const FString& OperationName)
{
	if (!bEnablePerformanceProfiling) return;

	const double* StartTimePtr = ActivePerformanceTimers.Find(OperationName);
	if (!StartTimePtr)
	{
		LogCritical(FString::Printf(TEXT("EndPerformanceLog: no start time found for '%s'"), *OperationName));
		return;
	}

	const double EndTime = FPlatformTime::Seconds();
	const double DurationMs = (EndTime - *StartTimePtr) * 1000.0;

	FBGPerformanceLog Entry;
	Entry.OperationName = OperationName;
	Entry.StartTime     = *StartTimePtr;
	Entry.EndTime       = EndTime;
	Entry.DurationMs    = DurationMs;
	PerformanceLogs.Add(Entry);
	ActivePerformanceTimers.Remove(OperationName);

	LogImportant(
		FString::Printf(TEXT("%s completed in %.2fms"), *OperationName, DurationMs),
		EBGLogCategory::Performance
	);
}

void UBGDevLog::ClearPerformanceLogs()
{
	PerformanceLogs.Empty();
	ActivePerformanceTimers.Empty();
}
