#include "BGVisualizer.h"

#if WITH_EDITORONLY_DATA

#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"

UBGVisualizer::UBGVisualizer()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================
// Toggle Buttons
// ============================================================

void UBGVisualizer::ToggleGrid()
{
	bShowGrid = !bShowGrid;
	OnRedrawRequested.ExecuteIfBound();
}

void UBGVisualizer::ToggleCoordinates()
{
	bShowCoordinates = !bShowCoordinates;
	OnRedrawRequested.ExecuteIfBound();
}

void UBGVisualizer::ToggleCellStates()
{
	bShowCellStates = !bShowCellStates;
	OnRedrawRequested.ExecuteIfBound();
}

void UBGVisualizer::ToggleForcedRegions()
{
	const bool bNew = !(bShowForcedEmptyRegions && bShowForcedEmptyCells);
	bShowForcedEmptyRegions = bNew;
	bShowForcedEmptyCells   = bNew;
	OnRedrawRequested.ExecuteIfBound();
}

// ============================================================
// Room-level Draw
// ============================================================

void UBGVisualizer::DrawRoomGrid(
	FIntPoint GridSize,
	const TArray<ECellType>& StructuralCells,
	const TArray<FIntPoint>& DoorCells,
	const TArray<FIntPoint>& CustomCells,
	const TArray<FIntPoint>& VoidCells,
	const TArray<FIntPoint>& OccupiedCells,
	float CellSize,
	FVector OriginLocation)
{
	if (!bEnableDebug) return;

	if (bShowGrid)
	{
		DrawGridLines(GridSize, CellSize, OriginLocation);
	}

	if (bShowCellStates)
	{
		DrawCellStates(GridSize, StructuralCells, DoorCells, CustomCells, VoidCells, OccupiedCells, CellSize, OriginLocation);
	}

	if (bShowCoordinates)
	{
		DrawGridCoordinates(GridSize, CellSize, OriginLocation, /*CoordStep*/ 1, CoordinateTextHeight);
	}
}

void UBGVisualizer::DrawCellStates(
	FIntPoint GridSize,
	const TArray<ECellType>& StructuralCells,
	const TArray<FIntPoint>& DoorCells,
	const TArray<FIntPoint>& CustomCells,
	const TArray<FIntPoint>& VoidCells,
	const TArray<FIntPoint>& OccupiedCells,
	float CellSize,
	FVector OriginLocation)
{
	const TSet<FIntPoint> DoorSet(DoorCells);
	const TSet<FIntPoint> CustomSet(CustomCells);
	const TSet<FIntPoint> VoidSet(VoidCells);
	const TSet<FIntPoint> OccupiedSet(OccupiedCells);

	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			const int32 Index = Y * GridSize.X + X;
			if (!StructuralCells.IsValidIndex(Index)) continue;

			const FIntPoint Cell(X, Y);
			const EBGCellDisplayState State =
				DeriveDisplayState(StructuralCells[Index], Cell, DoorSet, CustomSet, VoidSet, OccupiedSet);

			DrawCellBox(Cell, GetColorForDisplayState(State), CellSize, OriginLocation, CellBoxZOffset);
		}
	}
}

EBGCellDisplayState UBGVisualizer::DeriveDisplayState(
	ECellType Structural,
	const FIntPoint& Cell,
	const TSet<FIntPoint>& DoorCells,
	const TSet<FIntPoint>& CustomCells,
	const TSet<FIntPoint>& VoidCells,
	const TSet<FIntPoint>& OccupiedCells) const
{
	// Edge / corner cells are Wall unless they carry a door span.
	if (Structural == ECellType::Wall || Structural == ECellType::Corner)
	{
		return DoorCells.Contains(Cell) ? EBGCellDisplayState::Door : EBGCellDisplayState::Wall;
	}

	// Interior floor cells, in precedence order.
	if (VoidCells.Contains(Cell))     return EBGCellDisplayState::Void;
	if (CustomCells.Contains(Cell))   return EBGCellDisplayState::Custom;
	if (OccupiedCells.Contains(Cell)) return EBGCellDisplayState::Occupied;
	return EBGCellDisplayState::Empty;
}

// ============================================================
// Forced Region Overlays
// ============================================================

void UBGVisualizer::DrawForcedEmptyRegions(const TArray<FBGForcedEmptyRegion>& Regions, float CellSize, FVector OriginLocation)
{
	if (!bEnableDebug || !bShowForcedEmptyRegions) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (const FBGForcedEmptyRegion& Region : Regions)
	{
		const int32 MinX = FMath::Min(Region.Min.X, Region.Max.X);
		const int32 MinY = FMath::Min(Region.Min.Y, Region.Max.Y);
		const int32 MaxX = FMath::Max(Region.Min.X, Region.Max.X);
		const int32 MaxY = FMath::Max(Region.Min.Y, Region.Max.Y);

		const float SpanX = (MaxX - MinX + 1) * CellSize;
		const float SpanY = (MaxY - MinY + 1) * CellSize;

		FVector Center = OriginLocation + FVector(
			MinX * CellSize + SpanX * 0.5f,
			MinY * CellSize + SpanY * 0.5f,
			ForcedEmptyZOffset);

		const FVector Extent(SpanX * 0.5f, SpanY * 0.5f, ForcedEmptyZOffset * 0.8f);
		DrawDebugBox(World, Center, Extent, FQuat::Identity, ForcedEmptyRegionColor, true, GridLineLifetime, 0, CellBoxThickness);
	}
}

void UBGVisualizer::DrawForcedEmptyCells(const TArray<FIntPoint>& Cells, float CellSize, FVector OriginLocation)
{
	if (!bEnableDebug || !bShowForcedEmptyCells) return;

	for (const FIntPoint& Cell : Cells)
	{
		DrawCellBox(Cell, ForcedEmptyCellBorderColor, CellSize, OriginLocation, ForcedEmptyZOffset);
	}
}

// ============================================================
// Primitives
// ============================================================

void UBGVisualizer::DrawGridLines(FIntPoint GridSize, float CellSize, FVector OriginLocation)
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

void UBGVisualizer::DrawBoundaryBox(FIntPoint GridSize, float CellSize, FVector OriginLocation, float ZOffset)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const float GridW = GridSize.X * CellSize;
	const float GridH = GridSize.Y * CellSize;

	const FVector Center = OriginLocation + FVector(GridW * 0.5f, GridH * 0.5f, ZOffset);
	const FVector Extent(GridW * 0.5f, GridH * 0.5f, 5.f);
	DrawDebugBox(World, Center, Extent, FQuat::Identity, BoundaryColor, true, GridLineLifetime, 0, GridLineThickness * 2.f);
}

void UBGVisualizer::DrawCellBox(FIntPoint GridCoord, FColor Color, float CellSize, FVector OriginLocation, float ZOffset)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Center = GridToWorldPosition(GridCoord, CellSize, OriginLocation);
	Center.Z += ZOffset;

	const FVector Extent(CellSize / 2.2f, CellSize / 2.2f, FMath::Max(ZOffset * 0.8f, 1.f));
	DrawDebugBox(World, Center, Extent, FQuat::Identity, Color, true, GridLineLifetime, 0, CellBoxThickness);
}

void UBGVisualizer::DrawBox(FVector WorldCenter, FVector Extent, FColor Color, float Thickness)
{
	UWorld* World = GetWorld();
	if (!World) return;

	DrawDebugBox(World, WorldCenter, Extent, FQuat::Identity, Color, true, GridLineLifetime, 0, Thickness);
}

void UBGVisualizer::DrawGridCoordinates(FIntPoint GridSize, float CellSize, FVector OriginLocation, int32 CoordStep, float ZOffset)
{
	if (CoordStep < 1) CoordStep = 1;

	for (int32 X = 0; X < GridSize.X; X += CoordStep)
	{
		for (int32 Y = 0; Y < GridSize.Y; Y += CoordStep)
		{
			FVector CellCenter = GridToWorldPosition(FIntPoint(X, Y), CellSize, OriginLocation);
			CellCenter.Z += ZOffset;

			CreateLabel(CellCenter, FString::Printf(TEXT("(%d,%d)"), X, Y), CoordinateTextColor);
		}
	}
}

UTextRenderComponent* UBGVisualizer::CreateLabel(FVector WorldPosition, const FString& Text, FColor Color)
{
	if (!OnCreateTextComponent.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBGVisualizer] CreateLabel: OnCreateTextComponent delegate not bound"));
		return nullptr;
	}

	UTextRenderComponent* TextComp = OnCreateTextComponent.Execute(WorldPosition, Text, Color, CoordinateTextScale);
	if (TextComp)
	{
		TextComponents.Add(TextComp);
	}
	return TextComp;
}

// ============================================================
// Cleanup
// ============================================================

void UBGVisualizer::ClearTextComponents()
{
	for (UTextRenderComponent* TextComp : TextComponents)
	{
		if (!IsValid(TextComp)) continue;

		if (OnDestroyTextComponent.IsBound())
		{
			OnDestroyTextComponent.Execute(TextComp);
		}
		else
		{
			TextComp->DestroyComponent();
		}
	}

	TextComponents.Empty();
}

void UBGVisualizer::ClearDebugDrawings()
{
	if (UWorld* World = GetWorld())
	{
		FlushPersistentDebugLines(World);
		FlushDebugStrings(World);
	}

	ClearTextComponents();
}

// ============================================================
// Internal Helpers
// ============================================================

FColor UBGVisualizer::GetColorForDisplayState(EBGCellDisplayState State) const
{
	switch (State)
	{
	case EBGCellDisplayState::Empty:    return EmptyCellColor;
	case EBGCellDisplayState::Occupied: return OccupiedCellColor;
	case EBGCellDisplayState::Custom:   return CustomCellColor;
	case EBGCellDisplayState::Void:     return VoidCellColor;
	case EBGCellDisplayState::Wall:     return WallCellColor;
	case EBGCellDisplayState::Door:     return DoorCellColor;
	default:                            return FColor::White;
	}
}

FVector UBGVisualizer::GridToWorldPosition(FIntPoint GridCoord, float CellSize, FVector OriginLocation) const
{
	return OriginLocation + FVector(
		GridCoord.X * CellSize + (CellSize * 0.5f),
		GridCoord.Y * CellSize + (CellSize * 0.5f),
		0.f
	);
}

#endif // WITH_EDITORONLY_DATA
