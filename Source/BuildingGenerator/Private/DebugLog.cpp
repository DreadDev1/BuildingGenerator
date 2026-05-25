#include "DebugLog.h"

#include "BuildingGenerator/BuildingGenerator.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"
#endif

UDebugLog::UDebugLog()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================
// Internal Helpers
// ============================================================

void UDebugLog::LogInternal(EDebugLogLevel Level, EBGLogCategory Category, const FString& Message)
{
	FString FullMessage = FString::Printf(TEXT("%s [%s] %s"), *GetOwnerPrefix(), *GetCategoryString(Category), *Message);

	switch (Level)
	{
	case EDebugLogLevel::Critical:
		UE_LOG(LogBuildingGenerator, Error, TEXT("%s"), *FullMessage);
		break;
	case EDebugLogLevel::Important:
		UE_LOG(LogBuildingGenerator, Warning, TEXT("%s"), *FullMessage);
		break;
	default:
		UE_LOG(LogBuildingGenerator, Log, TEXT("%s"), *FullMessage)an I test 	}

	if (bEnableScreenLogging && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, ScreenLogDuration, GetColorForLevel(Level), FullMessage);
	}
}

bool UDebugLog::ShouldLog(EDebugLogLevel Level, EBGLogCategory Category) const
{
	if (!bEnableDebug) return false;
	if (static_cast<uint8>(Level) > static_cast<uint8>(CurrentLogLevel)) return false;
	if (bEnableCategoryFiltering && !EnabledCategories.Contains(Category)) return false;
	return true;
}

FString UDebugLog::GetOwnerPrefix() const
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

FString UDebugLog::GetCategoryString(EBGLogCategory Category) const
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

FColor UDebugLog::GetColorForLevel(EDebugLogLevel Level) const
{
	switch (Level)
	{
	case EDebugLogLevel::Critical:      return FColor::Red;
	case EDebugLogLevel::Important:     return FColor::Yellow;
	case EDebugLogLevel::Verbose:       return FColor::White;
	case EDebugLogLevel::Everything:    return FColor::Silver;
	default:                            return FColor::White;
	}
}

// ============================================================
// Public Logging API
// ============================================================

void UDebugLog::LogCritical(const FString& Message)
{
	// Bypasses all filters — always logs
	FString FullMessage = FString::Printf(TEXT("%s [CRITICAL] %s"), *GetOwnerPrefix(), *Message);
	UE_LOG(LogBuildingGenerator, Error, TEXT("%s"), *FullMessage);

	if (bEnableScreenLogging && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, ScreenLogDuration, FColor::Red, FullMessage);
	}
}

void UDebugLog::LogImportant(const FString& Message, EBGLogCategory Category)
{
	if (!ShouldLog(EDebugLogLevel::Important, Category)) return;
	LogInternal(EDebugLogLevel::Important, Category, Message);
}

void UDebugLog::LogVerbose(const FString& Message, EBGLogCategory Category)
{
	if (!ShouldLog(EDebugLogLevel::Verbose, Category)) return;
	LogInternal(EDebugLogLevel::Verbose, Category, Message);
}

void UDebugLog::LogStatistic(const FString& Label, const FString& Value, EBGLogCategory Category)
{
	if (!ShouldLog(EDebugLogLevel::Important, Category)) return;
	LogInternal(EDebugLogLevel::Important, Category, FString::Printf(TEXT("%s: %s"), *Label, *Value));
}

void UDebugLog::LogStatistic(const FString& Label, int32 Value, EBGLogCategory Category)
{
	LogStatistic(Label, FString::FromInt(Value), Category);
}

void UDebugLog::LogStatistic(const FString& Label, float Value, EBGLogCategory Category)
{
	LogStatistic(Label, FString::Printf(TEXT("%.2f"), Value), Category);
}

void UDebugLog::LogSectionHeader(const FString& Title)
{
	if (!ShouldLog(EDebugLogLevel::Important, EBGLogCategory::General)) return;

	FString FullMessage = FString::Printf(TEXT("%s === %s ==="), *GetOwnerPrefix(), *Title);
	UE_LOG(LogBuildingGenerator, Warning, TEXT("%s"), *FullMessage);

	if (bEnableScreenLogging && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, ScreenLogDuration, FColor::Cyan, FullMessage);
	}
}

// ============================================================
// Performance Profiling
// ============================================================

void UDebugLog::BeginPerformanceLog(const FString& OperationName)
{
	if (!bEnablePerformanceProfiling) return;
	ActivePerformanceTimers.Add(OperationName, FPlatformTime::Seconds());
	LogVerbose(FString::Printf(TEXT("Started: %s"), *OperationName), EBGLogCategory::Performance);
}

void UDebugLog::EndPerformanceLog(const FString& OperationName)
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

void UDebugLog::ClearPerformanceLogs()
{
	PerformanceLogs.Empty();
	ActivePerformanceTimers.Empty();
}

// ============================================================
// Visual Debug (Editor Only)
// ============================================================

#if WITH_EDITOR

void UDebugLog::DrawGrid(FIntPoint GridSize, const TArray<ECellType>& CellStates, float CellSize, FVector OriginLocation)
{
	if (!bEnableDebug) return;

	const AActor* Owner = GetOwner();
	if (!Owner) return;

	OwnerActorName = Owner->GetName();

	if (bShowGrid)
	{
		DrawGridLines(GridSize, CellSize, OriginLocation);
	}

	if (bShowCellStates)
	{
		DrawCellStates(GridSize, CellStates, CellSize, OriginLocation);
	}

	DrawGridCoordinatesWithTextComponents(GridSize, CellSize, OriginLocation);
}

void UDebugLog::DrawGridLines(FIntPoint GridSize, float CellSize, FVector OriginLocation)
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 X = 0; X <= GridSize.X; ++X)
	{
		const FVector Start = OriginLocation + FVector(X * CellSize, 0.f, 0.f);
		const FVector End   = OriginLocation + FVector(X * CellSize, GridSize.Y * CellSize, 0.f);
		DrawDebugLine(World, Start, End, GridColor, true, GridLineLifetime, 0, GridLineThickness);
	}

	for (int32 Y = 0; Y <= GridSize.Y; ++Y)
	{
		const FVector Start = OriginLocation + FVector(0.f, Y * CellSize, 0.f);
		const FVector End   = OriginLocation + FVector(GridSize.X * CellSize, Y * CellSize, 0.f);
		DrawDebugLine(World, Start, End, GridColor, true, GridLineLifetime, 0, GridLineThickness);
	}
}

void UDebugLog::DrawCellStates(FIntPoint GridSize, const TArray<ECellType>& CellStates, float CellSize, FVector OriginLocation)
{
	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			const int32 Index = Y * GridSize.X + X;
			if (CellStates.IsValidIndex(Index))
			{
				DrawCellBox(FIntPoint(X, Y), GetColorForCellType(CellStates[Index]), CellSize, OriginLocation, CellBoxZOffset);
			}
		}
	}
}

void UDebugLog::DrawCellBox(FIntPoint GridCoord, FColor Color, float CellSize, FVector OriginLocation, float ZOffset)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Center = GridToWorldPosition(GridCoord, CellSize, OriginLocation);
	Center.Z += ZOffset;

	const FVector Extent(CellSize / 2.2f, CellSize / 2.2f, ZOffset * 0.8f);
	DrawDebugBox(World, Center, Extent, FQuat::Identity, Color, true, GridLineLifetime, 0, CellBoxThickness);
}

void UDebugLog::DrawGridCoordinatesWithTextComponents(FIntPoint GridSize, float CellSize, FVector OriginLocation)
{
	ClearCoordinateTextComponents();

	if (!bShowCoordinates) return;

	if (!OnCreateTextComponent.IsBound())
	{
		LogCritical(TEXT("DrawGridCoordinatesWithTextComponents: OnCreateTextComponent delegate not bound"));
		return;
	}

	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FVector CellCenter = GridToWorldPosition(FIntPoint(X, Y), CellSize, OriginLocation);
			CellCenter.Z += CoordinateTextHeight;

			UTextRenderComponent* TextComp = OnCreateTextComponent.Execute(
				CellCenter,
				FString::Printf(TEXT("(%d,%d)"), X, Y),
				CoordinateTextColor,
				CoordinateTextScale
			);

			if (TextComp)
			{
				CoordinateTextComponents.Add(TextComp);
			}
		}
	}
}

void UDebugLog::ClearCoordinateTextComponents()
{
	if (OnDestroyTextComponent.IsBound())
	{
		for (UTextRenderComponent* TextComp : CoordinateTextComponents)
		{
			if (IsValid(TextComp))
			{
				OnDestroyTextComponent.Execute(TextComp);
			}
		}
	}
	else
	{
		for (UTextRenderComponent* TextComp : CoordinateTextComponents)
		{
			if (IsValid(TextComp))
			{
				TextComp->DestroyComponent();
			}
		}
	}

	CoordinateTextComponents.Empty();
}

void UDebugLog::ClearDebugDrawings()
{
	UWorld* World = GetWorld();
	if (World)
	{
		FlushPersistentDebugLines(World);
	}
}

FColor UDebugLog::GetColorForCellType(ECellType CellType) const
{
	switch (CellType)
	{
	case ECellType::Floor:  return FloorCellColor;
	case ECellType::Wall:   return WallCellColor;
	case ECellType::Corner: return CornerCellColor;
	default:                return FColor::White;
	}
}

FVector UDebugLog::GridToWorldPosition(FIntPoint GridCoord, float CellSize, FVector OriginLocation) const
{
	return OriginLocation + FVector(
		GridCoord.X * CellSize + (CellSize * 0.5f),
		GridCoord.Y * CellSize + (CellSize * 0.5f),
		0.f
	);
}

#endif // WITH_EDITOR
