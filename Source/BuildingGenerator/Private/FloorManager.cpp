#include "FloorManager.h"
#include "BuildingManager.h"
#include "MasterRoom.h"
#include "RoomData.h"
#include "DrawDebugHelpers.h"

AFloorManager::AFloorManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

// ============================================================
// Editor
// ============================================================

#if WITH_EDITOR

static const TCHAR* FaceName(ERoomFace Face)
{
	switch (Face)
	{
	case ERoomFace::North: return TEXT("North");
	case ERoomFace::South: return TEXT("South");
	case ERoomFace::East:  return TEXT("East");
	case ERoomFace::West:  return TEXT("West");
	default:               return TEXT("?");
	}
}

static ERoomFace OppositeFace(ERoomFace Face)
{
	switch (Face)
	{
	case ERoomFace::North: return ERoomFace::South;
	case ERoomFace::South: return ERoomFace::North;
	case ERoomFace::East:  return ERoomFace::West;
	default:               return ERoomFace::East;
	}
}

void AFloorManager::PreviewLayout()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Clear previous preview draws before redrawing.
	ClearPreview();

	constexpr float CellSize = 100.f;
	constexpr float Duration = -1.f;  // persistent — cleared by ClearPreview
	constexpr float ZDraw    = 5.f;

	const FVector FloorOrigin = GetActorLocation();

	// ----------------------------------------------------------------
	// Resolved footprint for each placement
	// ----------------------------------------------------------------

	struct FFootprint
	{
		int32     Index;
		FIntPoint Origin;
		FIntPoint Size;
	};

	TArray<FFootprint> Footprints;
	Footprints.Reserve(RoomPlacements.Num());

	for (int32 i = 0; i < RoomPlacements.Num(); i++)
	{
		const FRoomPlacement& P = RoomPlacements[i];

		FIntPoint Size = (P.SizeOverride.X > 0 && P.SizeOverride.Y > 0)
			? P.SizeOverride
			: (IsValid(P.RoomDataAsset) ? P.RoomDataAsset->MinRoomSize : FIntPoint(1, 1));

		if (P.RotationDegrees == 90 || P.RotationDegrees == 270)
		{
			Swap(Size.X, Size.Y);
		}

		Footprints.Add({ i, P.GridOrigin, Size });
	}

	// ----------------------------------------------------------------
	// Validation helpers
	// ----------------------------------------------------------------

	auto DoorWorldStart = [](const FFootprint& F, const FDoorPlacement& Door) -> int32
	{
		return (Door.Face == ERoomFace::North || Door.Face == ERoomFace::South)
			? F.Origin.X + Door.CellOffset
			: F.Origin.Y + Door.CellOffset;
	};

	auto GetDoorWidthCells = [](const FRoomPlacement& P, const FDoorPlacement& Door) -> int32
	{
		const UDoorData* DD = IsValid(Door.DoorDataOverride)
			? Door.DoorDataOverride.Get()
			: (IsValid(P.RoomDataAsset) && IsValid(P.RoomDataAsset->DoorData) ? P.RoomDataAsset->DoorData.Get() : nullptr);
		return (IsValid(DD) && DD->DoorWidth == EDoorWidth::FourCell) ? 4 : 2;
	};

	auto FindAdjacentIndex = [&](const FFootprint& F, ERoomFace Face) -> int32
	{
		for (const FFootprint& B : Footprints)
		{
			if (B.Index == F.Index) continue;
			bool bTouching = false;
			bool bOverlap  = false;
			switch (Face)
			{
			case ERoomFace::North:
				bTouching = (B.Origin.Y == F.Origin.Y + F.Size.Y);
				bOverlap  = (B.Origin.X < F.Origin.X + F.Size.X) && (B.Origin.X + B.Size.X > F.Origin.X);
				break;
			case ERoomFace::South:
				bTouching = (B.Origin.Y + B.Size.Y == F.Origin.Y);
				bOverlap  = (B.Origin.X < F.Origin.X + F.Size.X) && (B.Origin.X + B.Size.X > F.Origin.X);
				break;
			case ERoomFace::East:
				bTouching = (B.Origin.X == F.Origin.X + F.Size.X);
				bOverlap  = (B.Origin.Y < F.Origin.Y + F.Size.Y) && (B.Origin.Y + B.Size.Y > F.Origin.Y);
				break;
			case ERoomFace::West:
				bTouching = (B.Origin.X + B.Size.X == F.Origin.X);
				bOverlap  = (B.Origin.Y < F.Origin.Y + F.Size.Y) && (B.Origin.Y + B.Size.Y > F.Origin.Y);
				break;
			}
			if (bTouching && bOverlap) return B.Index;
		}
		return -1;
	};

	// ----------------------------------------------------------------
	// Validation checks
	// ----------------------------------------------------------------

	TArray<FString> Errors;
	TSet<int32>     ErrorRooms;

	// Check 1: out of bounds
	for (const FFootprint& F : Footprints)
	{
		if (F.Origin.X < 0 || F.Origin.Y < 0
			|| F.Origin.X + F.Size.X > FloorGridSize.X
			|| F.Origin.Y + F.Size.Y > FloorGridSize.Y)
		{
			ErrorRooms.Add(F.Index);
			Errors.Add(FString::Printf(
				TEXT("Room %d at (%d,%d) size %dx%d extends beyond FloorGridSize %dx%d"),
				F.Index, F.Origin.X, F.Origin.Y, F.Size.X, F.Size.Y, FloorGridSize.X, FloorGridSize.Y));
		}
	}

	// Check 2: room overlap
	for (int32 i = 0; i < Footprints.Num(); i++)
	{
		for (int32 j = i + 1; j < Footprints.Num(); j++)
		{
			const FFootprint& A = Footprints[i];
			const FFootprint& B = Footprints[j];
			const bool bOverlap =
				A.Origin.X     < B.Origin.X + B.Size.X &&
				A.Origin.X + A.Size.X > B.Origin.X     &&
				A.Origin.Y     < B.Origin.Y + B.Size.Y &&
				A.Origin.Y + A.Size.Y > B.Origin.Y;
			if (bOverlap)
			{
				ErrorRooms.Add(A.Index);
				ErrorRooms.Add(B.Index);
				Errors.Add(FString::Printf(TEXT("Room %d and Room %d overlap"), A.Index, B.Index));
			}
		}
	}

	// Checks 3-5: per-door validation
	for (const FFootprint& F : Footprints)
	{
		const FRoomPlacement& P = RoomPlacements[F.Index];

		for (const FDoorPlacement& Door : P.Doors)
		{
			const int32 AdjIdx = FindAdjacentIndex(F, Door.Face);
			const int32 Width  = GetDoorWidthCells(P, Door);

			// Check 3: orphan door
			if (AdjIdx < 0 && !Door.bIsExteriorDoor)
			{
				ErrorRooms.Add(F.Index);
				Errors.Add(FString::Printf(
					TEXT("Room %d has orphan door on %s face (no adjacent room or hallway)"),
					F.Index, FaceName(Door.Face)));
			}

			// Check 4: exterior door on non-perimeter face
			if (Door.bIsExteriorDoor)
			{
				bool bOnPerimeter = false;
				switch (Door.Face)
				{
				case ERoomFace::South: bOnPerimeter = (F.Origin.Y == 0); break;
				case ERoomFace::North: bOnPerimeter = (F.Origin.Y + F.Size.Y == FloorGridSize.Y); break;
				case ERoomFace::West:  bOnPerimeter = (F.Origin.X == 0); break;
				case ERoomFace::East:  bOnPerimeter = (F.Origin.X + F.Size.X == FloorGridSize.X); break;
				}
				if (!bOnPerimeter)
				{
					ErrorRooms.Add(F.Index);
					Errors.Add(FString::Printf(
						TEXT("Room %d has exterior door on non-perimeter %s face"),
						F.Index, FaceName(Door.Face)));
				}
			}

			// Check 5: misaligned shared doors
			if (AdjIdx >= 0)
			{
				const FFootprint&     AdjF    = Footprints[AdjIdx];
				const FRoomPlacement& AdjP    = RoomPlacements[AdjIdx];
				const int32           MyStart = DoorWorldStart(F, Door);
				const ERoomFace       OppFace = OppositeFace(Door.Face);

				for (const FDoorPlacement& AdjDoor : AdjP.Doors)
				{
					if (AdjDoor.Face != OppFace) continue;
					const int32 AdjStart = DoorWorldStart(AdjF, AdjDoor);
					const int32 AdjWidth = GetDoorWidthCells(AdjP, AdjDoor);
					if (MyStart != AdjStart || Width != AdjWidth)
					{
						ErrorRooms.Add(F.Index);
						ErrorRooms.Add(AdjIdx);
						Errors.Add(FString::Printf(
							TEXT("Room %d door (%s, world start %d, width %d) misaligned with Room %d door (world start %d, width %d)"),
							F.Index, FaceName(Door.Face), MyStart, Width,
							AdjIdx, AdjStart, AdjWidth));
					}
				}
			}
		}
	}

	// ----------------------------------------------------------------
	// Draw floor grid
	// ----------------------------------------------------------------

	const float GridW = FloorGridSize.X * CellSize;
	const float GridH = FloorGridSize.Y * CellSize;

	DrawDebugBox(World,
		FloorOrigin + FVector(GridW * 0.5f, GridH * 0.5f, ZDraw),
		FVector(GridW * 0.5f, GridH * 0.5f, 5.f),
		FColor::White, true, Duration, 0, 5.f);

	for (int32 X = 1; X < FloorGridSize.X; X++)
	{
		DrawDebugLine(World,
			FloorOrigin + FVector(X * CellSize, 0.f, ZDraw),
			FloorOrigin + FVector(X * CellSize, GridH, ZDraw),
			FColor(80, 80, 80), true, Duration, 0, 1.f);
	}
	for (int32 Y = 1; Y < FloorGridSize.Y; Y++)
	{
		DrawDebugLine(World,
			FloorOrigin + FVector(0.f, Y * CellSize, ZDraw),
			FloorOrigin + FVector(GridW, Y * CellSize, ZDraw),
			FColor(80, 80, 80), true, Duration, 0, 1.f);
	}

	// Coordinate labels every 5 cells
	constexpr int32 CoordStep = 5;
	for (int32 X = 0; X < FloorGridSize.X; X += CoordStep)
	{
		for (int32 Y = 0; Y < FloorGridSize.Y; Y += CoordStep)
		{
			DrawDebugString(World,
				FloorOrigin + FVector((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, ZDraw + 10.f),
				FString::Printf(TEXT("%d,%d"), X, Y),
				nullptr, FColor::Yellow, Duration);
		}
	}

	// ----------------------------------------------------------------
	// Draw room footprints
	// ----------------------------------------------------------------

	for (const FFootprint& F : Footprints)
	{
		const bool   bError    = ErrorRooms.Contains(F.Index);
		const FColor RoomColor = bError ? FColor::Red : FColor::Green;
		const float  RW        = F.Size.X * CellSize;
		const float  RH        = F.Size.Y * CellSize;
		const FVector RCenter  = FloorOrigin + FVector(
			F.Origin.X * CellSize + RW * 0.5f,
			F.Origin.Y * CellSize + RH * 0.5f,
			ZDraw + 10.f);

		DrawDebugBox(World, RCenter, FVector(RW * 0.5f, RH * 0.5f, 8.f), RoomColor, true, Duration, 0, 6.f);
		DrawDebugString(World,
			RCenter + FVector(0.f, 0.f, 15.f),
			FString::Printf(TEXT("[%d] %dx%d"), F.Index, F.Size.X, F.Size.Y),
			nullptr, RoomColor, Duration);
	}

	// ----------------------------------------------------------------
	// Log results
	// ----------------------------------------------------------------

	if (Errors.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[FloorManager] PreviewLayout: Valid — %d room(s), FloorIndex %d"),
			RoomPlacements.Num(), FloorIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FloorManager] PreviewLayout: %d validation error(s):"), Errors.Num());
		for (const FString& Err : Errors)
		{
			UE_LOG(LogTemp, Warning, TEXT("  %s"), *Err);
		}
	}
}

void AFloorManager::GenerateLayout()
{
	if (!IsValid(OwningBuildingManager))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FloorManager] GenerateLayout: OwningBuildingManager is not set — assign it in the Details panel"));
		return;
	}

	ClearLayout();

	constexpr float CellSize = 100.f;
	const FVector   FloorOrigin = GetActorLocation();

	for (const FRoomPlacement& Placement : RoomPlacements)
	{
		if (!Placement.RoomClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FloorManager] GenerateLayout: Placement has no RoomClass, skipping"));
			continue;
		}

		const FVector    WorldPos      = FloorOrigin + FVector(Placement.GridOrigin.X * CellSize, Placement.GridOrigin.Y * CellSize, 0.f);
		const FTransform SpawnTransform(FRotator(0.f, static_cast<float>(Placement.RotationDegrees), 0.f), WorldPos);

		AMasterRoom* Room = OwningBuildingManager->RequestRoomSpawn(
			Placement.RoomClass, SpawnTransform, Placement, RoomHeightCm);

		if (IsValid(Room))
		{
			SpawnedRooms.Add(Room);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[FloorManager] GenerateLayout: Spawned %d room(s) on FloorIndex %d"),
		SpawnedRooms.Num(), FloorIndex);
}

void AFloorManager::ClearLayout()
{
	for (TObjectPtr<AMasterRoom>& Room : SpawnedRooms)
	{
		if (IsValid(Room))
		{
			Room->Destroy();
		}
	}
	SpawnedRooms.Empty();
}

void AFloorManager::ClearPreview()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FlushPersistentDebugLines(World);
	FlushDebugStrings(World);
}

#endif // WITH_EDITOR
