// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE_LOGs.generated.h"

/* Log categories for organized debugging */
UENUM(BlueprintType)
enum class EDMLogCategory : uint8
{
	Grid        UMETA(DisplayName = "Grid"),
	Mesh        UMETA(DisplayName = "Mesh"),
	Wall        UMETA(DisplayName = "Wall"),
	Selection   UMETA(DisplayName = "Selection"),
	Socket      UMETA(DisplayName = "Socket"),
	Data        UMETA(DisplayName = "Data"),
	Performance UMETA(DisplayName = "Performance"),
	General     UMETA(DisplayName = "General")
};

/* Log verbosity levels */
UENUM(BlueprintType)
enum class EDMLogVerbosity : uint8
{
	Error   UMETA(DisplayName = "Error"),
	Warning UMETA(DisplayName = "Warning"),
	Display UMETA(DisplayName = "Display"),
	Log     UMETA(DisplayName = "Log"),
	Verbose UMETA(DisplayName = "Verbose")
};

/* Performance timing data */
USTRUCT(BlueprintType)
struct FDMPerformanceLog
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FString OperationName;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	double StartTime;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	double EndTime;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	double DurationMs;

	FDMPerformanceLog()
		: StartTime(0.0)
		, EndTime(0.0)
		, DurationMs(0.0)
	{}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BUILDINGGENERATOR_API UUE_LOGs : public UActorComponent
{
	GENERATED_BODY()
	
public:
	// Sets default values for this component's properties
	UUE_LOGs();

#pragma region Logging Configuration
	/** Enable/disable all logging from this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	bool bEnableLogging = true;

	/** Minimum verbosity level to display logs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	EDMLogVerbosity MinimumVerbosity = EDMLogVerbosity::Display;

	/** Enable screen logging (on-screen debug messages) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging|Screen")
	bool bEnableScreenLogging = true;

	/** Duration for screen messages */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging|Screen", meta=(EditCondition="bEnableScreenLogging"))
	float ScreenLogDuration = 5.0f;

	/** Enable category filtering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging|Filters")
	bool bEnableCategoryFiltering = false;

	/** Categories to log (if filtering enabled) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging|Filters", meta=(EditCondition="bEnableCategoryFiltering"))
	TSet<EDMLogCategory> EnabledCategories;

	/** Enable performance profiling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging|Performance")
	bool bEnablePerformanceProfiling = true;
#pragma endregion

#pragma region Logging Functions
	/** Log a message with category and verbosity */
	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogMessage(EDMLogCategory Category, EDMLogVerbosity Verbosity, const FString& Message);

	/** Quick log functions for common verbosity levels */
	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogError(EDMLogCategory Category, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogWarning(EDMLogCategory Category, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogDisplay(EDMLogCategory Category, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogVerbose(EDMLogCategory Category, const FString& Message);
#pragma endregion

#pragma region Grid Specific Logging
	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogGridInitialization(int32 SizeX, int32 SizeY, float CellSize);

	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogCellOccupancy(const FIntPoint& Cell, bool bOccupied);

	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Logging")
	void LogMeshPlacement(const FIntPoint& Cell, const FString& MeshName, const FTransform& Transform);
#pragma endregion

#pragma region Performance Profiling Functions
	/** Begin a performance measurement */
	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Performance")
	void BeginPerformanceLog(const FString& OperationName);

	/** End a performance measurement and log the result */
	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Performance")
	void EndPerformanceLog(const FString& OperationName);

	/** Get all performance logs */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonManager|Performance")
	TArray<FDMPerformanceLog> GetPerformanceLogs() const { return PerformanceLogs; }

	/** Clear all performance logs */
	UFUNCTION(BlueprintCallable, Category = "DungeonManager|Performance")
	void ClearPerformanceLogs();
#pragma endregion
protected:
	virtual void BeginPlay() override;

private:
	// Internal logging
	void LogInternal(EDMLogCategory Category, EDMLogVerbosity Verbosity, const FString& Message);
	bool ShouldLog(EDMLogCategory Category, EDMLogVerbosity Verbosity) const;
	FColor GetColorForVerbosity(EDMLogVerbosity Verbosity) const;
	FString GetCategoryString(EDMLogCategory Category) const;

	// Performance tracking
	TMap<FString, double> ActivePerformanceTimers;
	TArray<FDMPerformanceLog> PerformanceLogs;
};
