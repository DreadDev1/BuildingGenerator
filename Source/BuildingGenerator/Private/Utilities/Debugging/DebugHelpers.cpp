// Fill out your copyright notice in the Description page of Project Settings.

#include "Utilities/Debugging/DebugHelpers.h"
#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"
#include "Data/Generation/RoomGenerationTypes.h"
#include "Engine/Engine.h"

UDebugHelpers::UDebugHelpers()
{
	PrimaryComponentTick.bCanEverTick = false;
}

#pragma region Debug Drawing API
void UDebugHelpers:: DrawGrid(FIntPoint GridSize, const TArray<EGridCellType>& CellStates, float CellSize, FVector OriginLocation)
{
	if (!bEnableDebug) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Cache owner name for logging
	OwnerActorName = Owner->GetName();

	// Draw grid lines
	if (bShowGrid)
	{
		DrawGridLines(GridSize, CellSize, OriginLocation);
	}

	// Draw cell states (red/blue boxes)
	if (bShowCellStates)
	{
		DrawCellStates(GridSize, CellStates, CellSize, OriginLocation);
	}

	// Draw coordinates
	DrawGridCoordinatesWithTextComponents(GridSize, CellSize, OriginLocation);
}

void UDebugHelpers::DrawForcedEmptyRegions(const TArray<FForcedEmptyRegion>& Regions, FIntPoint GridSize, float CellSize, FVector OriginLocation)
{
	if (!bEnableDebug || ! bShowForcedEmptyRegions) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (const FForcedEmptyRegion& Region : Regions)
	{
		// Calculate bounding box (handles any corner order)
		int32 MinX = FMath::Min(Region.StartCell.X, Region.EndCell. X);
		int32 MaxX = FMath::Max(Region.StartCell.X, Region.EndCell.X);
		int32 MinY = FMath::Min(Region.StartCell.Y, Region.EndCell. Y);
		int32 MaxY = FMath::Max(Region.StartCell.Y, Region.EndCell.Y);

		// Clamp to valid grid bounds
		MinX = FMath::Clamp(MinX, 0, GridSize.X - 1);
		MaxX = FMath::Clamp(MaxX, 0, GridSize.X - 1);
		MinY = FMath:: Clamp(MinY, 0, GridSize.Y - 1);
		MaxY = FMath::Clamp(MaxY, 0, GridSize. Y - 1);

		// Draw each cell in the region
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				DrawCellBox(FIntPoint(X, Y), ForcedEmptyRegionColor, CellSize, OriginLocation, ForcedEmptyZOffset);
			}
		}
	}

	LogVerbose(FString::Printf(TEXT("Drew %d forced empty regions"), Regions.Num()));
}

void UDebugHelpers::DrawForcedEmptyCells(const TArray<FIntPoint>& Cells, FIntPoint GridSize, float CellSize, FVector OriginLocation)
{
	if (!bEnableDebug || !bShowForcedEmptyCells) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (const FIntPoint& Cell :  Cells)
	{
		// Validate cell is within grid bounds
		if (Cell.X >= 0 && Cell.X < GridSize.X && Cell.Y >= 0 && Cell.Y < GridSize.Y)
		{
			// Inner cyan box
			DrawCellBox(Cell, ForcedEmptyRegionColor, CellSize, OriginLocation, ForcedEmptyZOffset);

			// Outer orange border to distinguish from region cells
			FVector Center = GridToWorldPosition(Cell, CellSize, OriginLocation);
			Center.Z += ForcedEmptyZOffset;

			FVector OuterExtent(CellSize / 2.0f, CellSize / 2.0f, 27.0f);
			DrawDebugBox(World, Center, OuterExtent, FQuat::Identity, ForcedEmptyCellBorderColor, true, GridLineLifetime, 0, 2.0f);
		}
	}

	LogVerbose(FString::Printf(TEXT("Drew %d forced empty cells"), Cells.Num()));
}

void UDebugHelpers::DrawGridLines(FIntPoint GridSize, float CellSize, FVector OriginLocation)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Draw X-axis lines (vertical lines in world)
	for (int32 X = 0; X <= GridSize.X; ++X)
	{
		FVector Start = OriginLocation + FVector(X * CellSize, 0.0f, 0.0f);
		FVector End = OriginLocation + FVector(X * CellSize, GridSize.Y * CellSize, 0.0f);
		DrawDebugLine(World, Start, End, GridColor, true, GridLineLifetime, 0, GridLineThickness);
	}

	// Draw Y-axis lines (horizontal lines in world)
	for (int32 Y = 0; Y <= GridSize.Y; ++Y)
	{
		FVector Start = OriginLocation + FVector(0.0f, Y * CellSize, 0.0f);
		FVector End = OriginLocation + FVector(GridSize.X * CellSize, Y * CellSize, 0.0f);
		DrawDebugLine(World, Start, End, GridColor, true, GridLineLifetime, 0, GridLineThickness);
	}
}

void UDebugHelpers::DrawCellStates(FIntPoint GridSize, const TArray<EGridCellType>& CellStates, float CellSize, FVector OriginLocation)
{
	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			int32 Index = Y * GridSize.X + X;
			if (CellStates. IsValidIndex(Index))
			{
				FColor BoxColor = GetColorForCellType(CellStates[Index]);
				DrawCellBox(FIntPoint(X, Y), BoxColor, CellSize, OriginLocation, CellBoxZOffset);
			}
		}
	}
}

void UDebugHelpers::DrawCellBox(FIntPoint GridCoord, FColor Color, float CellSize, FVector OriginLocation, float ZOffset)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Center of the cell
	FVector Center = GridToWorldPosition(GridCoord, CellSize, OriginLocation);
	Center.Z += ZOffset;

	// Size of the box (half extent)
	FVector Extent(CellSize / 2.2f, CellSize / 2.2f, ZOffset * 0.8f);

	DrawDebugBox(World, Center, Extent, FQuat::Identity, Color, true, GridLineLifetime, 0, CellBoxThickness);
}

void UDebugHelpers::DrawGridCoordinatesWithTextComponents(FIntPoint GridSize, float CellSize, FVector OriginLocation)
{
	// ✅ ALWAYS clear existing text components first (moved BEFORE the check)
	ClearCoordinateTextComponents();

	// NOW check if we should create new ones
	if (! bShowCoordinates)
	{
		// Coordinates are hidden - don't create new ones
		return;
	}

	if (! OnCreateTextComponent. IsBound())
	{
		LogCritical(TEXT("DrawGridCoordinatesWithTextComponents:   OnCreateTextComponent delegate not bound!"));
		return;
	}

	LogVerbose(FString::Printf(TEXT("Creating coordinate text components for %dx%d grid"), GridSize.X, GridSize.Y));

	// Create text component for each grid cell
	for (int32 X = 0; X < GridSize.X; ++X)
	{
		for (int32 Y = 0; Y < GridSize.Y; ++Y)
		{
			FIntPoint GridCoord(X, Y);
			FVector CellCenter = GridToWorldPosition(GridCoord, CellSize, OriginLocation);

			// Offset text above the grid using CoordinateTextHeight
			CellCenter.Z += CoordinateTextHeight;

			// Format coordinate string
			FString CoordText = FString::Printf(TEXT("(%d,%d)"), X, Y);

			// Request text component from owner via delegate
			UTextRenderComponent* TextComp = OnCreateTextComponent.Execute(
				CellCenter,
				CoordText,
				CoordinateTextColor,
				CoordinateTextScale
			);

			if (TextComp)
			{
				CoordinateTextComponents.Add(TextComp);
			}
		}
	}

	LogImportant(FString::Printf(TEXT("Created %d coordinate text components"), CoordinateTextComponents.Num()));
}

FColor UDebugHelpers:: GetColorForCellType(EGridCellType CellType) const
{
	switch (CellType)
	{
	case EGridCellType::ECT_Empty:
		return EmptyCellColor;  // Blue
	case EGridCellType:: ECT_FloorMesh:
		return OccupiedCellColor;  // Red
	case EGridCellType::ECT_WallMesh:
		return FColor::Purple;  
	case EGridCellType::ECT_Doorway:
		return FColor:: Yellow;  
	case EGridCellType::ECT_Custom:
		return CustomCellColor;       
	case EGridCellType::ECT_Void:
		return VoidCellColor;
	default:
		return FColor::White;
	}
}

FVector UDebugHelpers::GridToWorldPosition(FIntPoint GridCoord, float CellSize, FVector OriginLocation) const
{
	// Calculate center of cell
	float CenterOffsetX = GridCoord.X * CellSize + (CellSize * 0.5f);
	float CenterOffsetY = GridCoord.Y * CellSize + (CellSize * 0.5f);
	
	return OriginLocation + FVector(CenterOffsetX, CenterOffsetY, 0.0f);
}
#pragma endregion

#pragma region Debuging Cleanup
void UDebugHelpers::ClearDebugDrawings()
{
	UWorld* World = GetWorld();
	if (World)
	{
		FlushPersistentDebugLines(World);
		LogVerbose(TEXT("Cleared debug drawings"));
	}
}

void UDebugHelpers::ClearCoordinateTextComponents()
{
	// ✅ Use delegate to let owner destroy components (if bound)
	if (OnDestroyTextComponent.IsBound())
	{
		for (UTextRenderComponent* TextComp : CoordinateTextComponents)
		{
			if (TextComp && TextComp->IsValidLowLevel())
			{
				// Request owner to destroy this component
				OnDestroyTextComponent.Execute(TextComp);
			}
		}
	}
	else
	{
		// Fallback: Try to destroy directly (less reliable)
		for (UTextRenderComponent* TextComp : CoordinateTextComponents)
		{
			if (TextComp && TextComp->IsValidLowLevel())
			{
				TextComp->DestroyComponent();
			}
		}
	}

	CoordinateTextComponents.Empty();
	LogVerbose(TEXT("Cleared coordinate text components"));
}
#pragma endregion

#pragma region Debug Logging API

FString UDebugHelpers::GetCategoryPrefix() const
{
	if (OwnerActorName. IsEmpty())
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			return FString::Printf(TEXT("[%s]"), *Owner->GetName());
		}
		return TEXT("[Unknown]");
	}

	return FString::Printf(TEXT("[%s]"), *OwnerActorName);
}

bool UDebugHelpers::ShouldLog(EDebugLogLevel MessageLevel) const
{
	return bEnableDebug && (MessageLevel <= CurrentLogLevel);
}

void UDebugHelpers::LogCritical(const FString& Message)
{
	// Critical messages always show (even if debug disabled)
	FString FullMessage = FString::Printf(TEXT("%s %s"), *GetCategoryPrefix(), *Message);
	UE_LOG(LogTemp, Error, TEXT("%s"), *FullMessage);
}

void UDebugHelpers::LogImportant(const FString& Message)
{
	if (! ShouldLog(EDebugLogLevel::Important)) return;

	FString FullMessage = FString::Printf(TEXT("%s %s"), *GetCategoryPrefix(), *Message);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *FullMessage);
}

void UDebugHelpers::LogStatistic(const FString& Label, const FString& Value)
{
	if (!ShouldLog(EDebugLogLevel::Important)) return;

	FString FullMessage = FString::Printf(TEXT("%s %s:  %s"), *GetCategoryPrefix(), *Label, *Value);
	UE_LOG(LogTemp, Log, TEXT("%s"), *FullMessage);
}

void UDebugHelpers::LogStatistic(const FString& Label, int32 Value)
{
	LogStatistic(Label, FString:: FromInt(Value));
}

void UDebugHelpers::LogStatistic(const FString& Label, float Value)
{
	LogStatistic(Label, FString:: Printf(TEXT("%.2f"), Value));
}

void UDebugHelpers::LogVerbose(const FString& Message)
{
	if (!ShouldLog(EDebugLogLevel::Verbose)) return;

	FString FullMessage = FString::Printf(TEXT("%s %s"), *GetCategoryPrefix(), *Message);
	UE_LOG(LogTemp, Log, TEXT("%s"), *FullMessage);
}

void UDebugHelpers::LogSectionHeader(const FString& Title)
{
	if (!ShouldLog(EDebugLogLevel::Important)) return;

	// Just log the title without separator lines (as per your preference)
	FString FullMessage = FString::Printf(TEXT("%s %s"), *GetCategoryPrefix(), *Title);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *FullMessage);
}
#pragma endregion
