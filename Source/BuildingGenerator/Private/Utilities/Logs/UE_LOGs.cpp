// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/Logs/UE_LOGs.h"

#include "BuildingGenerator/BuildingGenerator.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "Engine/Engine.h"

UUE_LOGs::UUE_LOGs()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Default enabled categories
	EnabledCategories.Add(EDMLogCategory::Grid);
	EnabledCategories. Add(EDMLogCategory:: Mesh);
	EnabledCategories.Add(EDMLogCategory:: Wall);
	EnabledCategories.Add(EDMLogCategory:: Selection);
	EnabledCategories.Add(EDMLogCategory:: Performance);
}


// Called when the game starts
void UUE_LOGs::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableLogging)
	{
		LogDisplay(EDMLogCategory::General, TEXT("DMLogComponent initialized"));
	}
	
}

// ===========================
// Core Logging Functions
// ===========================

void UUE_LOGs:: LogMessage(EDMLogCategory Category, EDMLogVerbosity Verbosity, const FString& Message)
{
	LogInternal(Category, Verbosity, Message);
}

void UUE_LOGs::LogError(EDMLogCategory Category, const FString& Message)
{
	LogInternal(Category, EDMLogVerbosity::Error, Message);
}

void UUE_LOGs::LogWarning(EDMLogCategory Category, const FString& Message)
{
	LogInternal(Category, EDMLogVerbosity::Warning, Message);
}

void UUE_LOGs::LogDisplay(EDMLogCategory Category, const FString& Message)
{
	LogInternal(Category, EDMLogVerbosity::Display, Message);
}

void UUE_LOGs:: LogVerbose(EDMLogCategory Category, const FString& Message)
{
	LogInternal(Category, EDMLogVerbosity::Verbose, Message);
}

// ===========================
// Grid-Specific Logging
// ===========================

void UUE_LOGs:: LogGridInitialization(int32 SizeX, int32 SizeY, float CellSize)
{
	FString Message = FString::Printf(
		TEXT("Grid Initialized: %dx%d cells, Cell Size: %.2f"),
		SizeX, SizeY, CellSize
	);
	LogDisplay(EDMLogCategory::Grid, Message);
}

void UUE_LOGs::LogCellOccupancy(const FIntPoint& Cell, bool bOccupied)
{
	FString Message = FString::Printf(
		TEXT("Cell (%d, %d) marked as %s"),
		Cell.X, Cell.Y,
		bOccupied ?  TEXT("OCCUPIED") : TEXT("AVAILABLE")
	);
	LogVerbose(EDMLogCategory:: Grid, Message);
}

void UUE_LOGs::LogMeshPlacement(const FIntPoint& Cell, const FString& MeshName, const FTransform& Transform)
{
	FString Message = FString::Printf(
		TEXT("Mesh '%s' placed at Cell (%d, %d) | World Pos: %s"),
		*MeshName,
		Cell.X, Cell. Y,
		*Transform.GetLocation().ToString()
	);
	LogDisplay(EDMLogCategory::Mesh, Message);
}

// ===========================
// Performance Profiling
// ===========================

void UUE_LOGs::BeginPerformanceLog(const FString& OperationName)
{
	if (! bEnablePerformanceProfiling) return;

	double CurrentTime = FPlatformTime:: Seconds();
	ActivePerformanceTimers.Add(OperationName, CurrentTime);

	LogVerbose(EDMLogCategory:: Performance, FString::Printf(TEXT("Started:  %s"), *OperationName));
}

void UUE_LOGs::EndPerformanceLog(const FString& OperationName)
{
	if (!bEnablePerformanceProfiling) return;

	double* StartTimePtr = ActivePerformanceTimers.Find(OperationName);
	if (! StartTimePtr)
	{
		LogWarning(EDMLogCategory::Performance, FString::Printf(TEXT("No start time found for: %s"), *OperationName));
		return;
	}

	double EndTime = FPlatformTime:: Seconds();
	double DurationMs = (EndTime - *StartTimePtr) * 1000.0;

	FDMPerformanceLog PerfLog;
	PerfLog.OperationName = OperationName;
	PerfLog.StartTime = *StartTimePtr;
	PerfLog.EndTime = EndTime;
	PerfLog.DurationMs = DurationMs;

	PerformanceLogs.Add(PerfLog);
	ActivePerformanceTimers.Remove(OperationName);

	/*FString Message = FString::Printf(TEXT("Completed: %s | Duration: %. 2fms"), *OperationName, DurationMs);
	LogDisplay(EDMLogCategory::Performance, Message);*/
}

void UUE_LOGs::ClearPerformanceLogs()
{
	PerformanceLogs.Empty();
	ActivePerformanceTimers.Empty();
	LogDisplay(EDMLogCategory::Performance, TEXT("Performance logs cleared"));
}

// ===========================
// Internal Helpers
// ===========================

void UUE_LOGs::LogInternal(EDMLogCategory Category, EDMLogVerbosity Verbosity, const FString& Message)
{
	if (!ShouldLog(Category, Verbosity)) return;

	FString CategoryStr = GetCategoryString(Category);
	FString FullMessage = FString::Printf(TEXT("[%s] %s"), *CategoryStr, *Message);

	// Output Log
	switch (Verbosity)
	{
		case EDMLogVerbosity::Error:
			UE_LOG(BuildingManagerLog, Error, TEXT("%s"), *FullMessage);
			break;
		case EDMLogVerbosity::Warning:
			UE_LOG(BuildingManagerLog, Warning, TEXT("%s"), *FullMessage);
			break;
		case EDMLogVerbosity::Display:
			UE_LOG(BuildingManagerLog, Display, TEXT("%s"), *FullMessage);
			break;
		case EDMLogVerbosity::Log:
			UE_LOG(BuildingManagerLog, Log, TEXT("%s"), *FullMessage);
			break;
		case EDMLogVerbosity::Verbose:
			UE_LOG(BuildingManagerLog, Verbose, TEXT("%s"), *FullMessage);
			break;
	}

	// Screen Log
	if (bEnableScreenLogging && GEngine)
	{
		FColor Color = GetColorForVerbosity(Verbosity);
		GEngine->AddOnScreenDebugMessage(-1, ScreenLogDuration, Color, FullMessage);
	}
}

bool UUE_LOGs::ShouldLog(EDMLogCategory Category, EDMLogVerbosity Verbosity) const
{
	if (! bEnableLogging) return false;

	// Check verbosity level
	if (Verbosity > MinimumVerbosity) return false;

	// Check category filtering
	if (bEnableCategoryFiltering && ! EnabledCategories.Contains(Category)) return false;

	return true;
}

FColor UUE_LOGs::GetColorForVerbosity(EDMLogVerbosity Verbosity) const
{
	switch (Verbosity)
	{
		case EDMLogVerbosity::Error:   return FColor::Red;
		case EDMLogVerbosity:: Warning: return FColor::Yellow;
		case EDMLogVerbosity::Display: return FColor:: Cyan;
		case EDMLogVerbosity::Log:     return FColor::White;
		case EDMLogVerbosity::Verbose: return FColor::Silver;
		default:                        return FColor::White;
	}
}

FString UUE_LOGs::GetCategoryString(EDMLogCategory Category) const
{
	switch (Category)
	{
		case EDMLogCategory::Grid:         return TEXT("GRID");
		case EDMLogCategory::Mesh:        return TEXT("MESH");
		case EDMLogCategory::Wall:        return TEXT("WALL");
		case EDMLogCategory::Selection:   return TEXT("SELECT");
		case EDMLogCategory::Socket:      return TEXT("SOCKET");
		case EDMLogCategory:: Data:        return TEXT("DATA");
		case EDMLogCategory:: Performance: return TEXT("PERF");
		case EDMLogCategory::General:     return TEXT("GENERAL");
		default:                          return TEXT("UNKNOWN");
	}
}