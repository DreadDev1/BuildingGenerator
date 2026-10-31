#include "FloorManager.h"
#include "BuildingManager.h"
#include "MasterRoom.h"
#include "RoomData.h"
#include "BGDevLog.h"

#if WITH_EDITOR
#include "BGVisualizer.h"
#include "Components/TextRenderComponent.h"
#endif

AFloorManager::AFloorManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	DevLog = CreateDefaultSubobject<UBGDevLog>(TEXT("DevLog"));

#if WITH_EDITOR
	Visualizer = CreateDefaultSubobject<UBGVisualizer>(TEXT("Visualizer"));
	// Floor-level grid: thin gray lines, on by default (PreviewLayout's main feature).
	Visualizer->GridColor          = FColor(80, 80, 80);
	Visualizer->GridLineThickness  = 1.f;
	Visualizer->bShowGrid          = true;
	Visualizer->bShowCoordinates   = true;
	Visualizer->OnCreateTextComponent.BindUObject(this, &AFloorManager::CreateDebugTextComponent);
	Visualizer->OnDestroyTextComponent.BindUObject(this, &AFloorManager::DestroyDebugTextComponent);
	Visualizer->OnRedrawRequested.BindUObject(this, &AFloorManager::PreviewLayout);
#endif
}

// ============================================================
// Generation
// ============================================================

void AFloorManager::GenerateLayoutWith(ABuildingManager* BuildingManager)
{
	if (!IsValid(BuildingManager))
	{
		DevLog->LogCritical(TEXT("GenerateLayoutWith: BuildingManager is null"));
		return;
	}

	ClearLayout();

	constexpr float CellSize = 100.f;
	const FVector   FloorOrigin = GetActorLocation();

	for (const FRoomPlacement& Placement : RoomPlacements)
	{
		if (!Placement.RoomClass)
		{
			DevLog->LogImportant(TEXT("GenerateLayoutWith: Placement has no RoomClass, skipping"), EBGLogCategory::Generation);
			continue;
		}

		const FVector    WorldPos      = FloorOrigin + FVector(Placement.GridOrigin.X * CellSize, Placement.GridOrigin.Y * CellSize, 0.f);
		const FTransform SpawnTransform(FRotator(0.f, static_cast<float>(Placement.RotationDegrees), 0.f), WorldPos);

		AMasterRoom* Room = BuildingManager->RequestRoomSpawn(
			Placement.RoomClass, SpawnTransform, Placement, RoomHeightCm);

		if (IsValid(Room))
		{
			SpawnedRooms.Add(Room);
		}
	}

	DevLog->LogImportant(
		FString::Printf(TEXT("GenerateLayoutWith: Spawned %d room(s) on FloorIndex %d"), SpawnedRooms.Num(), FloorIndex),
		EBGLogCategory::Generation);
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
		// North=+X / South=-X doors run along Y; East=+Y / West=-Y doors run along X.
		return (Door.Face == ERoomFace::North || Door.Face == ERoomFace::South)
			? F.Origin.Y + Door.CellOffset
			: F.Origin.X + Door.CellOffset;
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
			// North=+X / South=-X adjoin along X (overlap tested on Y);
			// East=+Y / West=-Y adjoin along Y (overlap tested on X).
			switch (Face)
			{
			case ERoomFace::North:
				bTouching = (B.Origin.X == F.Origin.X + F.Size.X);
				bOverlap  = (B.Origin.Y < F.Origin.Y + F.Size.Y) && (B.Origin.Y + B.Size.Y > F.Origin.Y);
				break;
			case ERoomFace::South:
				bTouching = (B.Origin.X + B.Size.X == F.Origin.X);
				bOverlap  = (B.Origin.Y < F.Origin.Y + F.Size.Y) && (B.Origin.Y + B.Size.Y > F.Origin.Y);
				break;
			case ERoomFace::East:
				bTouching = (B.Origin.Y == F.Origin.Y + F.Size.Y);
				bOverlap  = (B.Origin.X < F.Origin.X + F.Size.X) && (B.Origin.X + B.Size.X > F.Origin.X);
				break;
			case ERoomFace::West:
				bTouching = (B.Origin.Y + B.Size.Y == F.Origin.Y);
				bOverlap  = (B.Origin.X < F.Origin.X + F.Size.X) && (B.Origin.X + B.Size.X > F.Origin.X);
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
				// North=+X / South=-X bound the X edges; East=+Y / West=-Y bound the Y edges.
				switch (Door.Face)
				{
				case ERoomFace::South: bOnPerimeter = (F.Origin.X == 0); break;
				case ERoomFace::North: bOnPerimeter = (F.Origin.X + F.Size.X == FloorGridSize.X); break;
				case ERoomFace::West:  bOnPerimeter = (F.Origin.Y == 0); break;
				case ERoomFace::East:  bOnPerimeter = (F.Origin.Y + F.Size.Y == FloorGridSize.Y); break;
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

	if (Visualizer)
	{
		const FVector DrawOrigin = FloorOrigin + FVector(0.f, 0.f, ZDraw);
		if (Visualizer->bShowGrid)
		{
			Visualizer->DrawBoundaryBox(FloorGridSize, CellSize, FloorOrigin, ZDraw);
			Visualizer->DrawGridLines(FloorGridSize, CellSize, DrawOrigin);
		}
		if (Visualizer->bShowCoordinates)
		{
			Visualizer->DrawGridCoordinates(FloorGridSize, CellSize, DrawOrigin, /*CoordStep*/ 5, /*ZOffset*/ 10.f);
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

		if (Visualizer)
		{
			Visualizer->DrawBox(RCenter, FVector(RW * 0.5f, RH * 0.5f, 8.f), RoomColor, 6.f);
			Visualizer->CreateLabel(
				RCenter + FVector(0.f, 0.f, 15.f),
				FString::Printf(TEXT("[%d] %dx%d"), F.Index, F.Size.X, F.Size.Y),
				RoomColor);
		}
	}

	// ----------------------------------------------------------------
	// Log results
	// ----------------------------------------------------------------

	DevLog->LogSectionHeader(TEXT("PreviewLayout"));
	if (Errors.Num() == 0)
	{
		DevLog->LogImportant(
			FString::Printf(TEXT("Valid — %d room(s), FloorIndex %d"), RoomPlacements.Num(), FloorIndex),
			EBGLogCategory::Generation);
	}
	else
	{
		DevLog->LogCritical(FString::Printf(TEXT("PreviewLayout: %d validation error(s):"), Errors.Num()));
		for (const FString& Err : Errors)
		{
			DevLog->LogCritical(Err);
		}
	}
}

void AFloorManager::GenerateLayout()
{
	// Standalone editor path: spawns directly with seed 0 for layout testing.
	// For full building generation with the correct seed, use ABuildingManager::SpawnBuilding.
	UWorld* World = GetWorld();
	if (!World) return;

	ClearLayout();

	constexpr float CellSize = 100.f;
	const FVector   FloorOrigin = GetActorLocation();

	for (const FRoomPlacement& Placement : RoomPlacements)
	{
		if (!Placement.RoomClass)
		{
			DevLog->LogImportant(TEXT("GenerateLayout: Placement has no RoomClass, skipping"), EBGLogCategory::Generation);
			continue;
		}

		const FVector    WorldPos      = FloorOrigin + FVector(Placement.GridOrigin.X * CellSize, Placement.GridOrigin.Y * CellSize, 0.f);
		const FTransform SpawnTransform(FRotator(0.f, static_cast<float>(Placement.RotationDegrees), 0.f), WorldPos);

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AMasterRoom* Room = World->SpawnActor<AMasterRoom>(Placement.RoomClass, SpawnTransform, Params);
		if (IsValid(Room))
		{
			Room->InitializeRoom(Placement, 0 /* seed 0: standalone preview */, RoomHeightCm);
			SpawnedRooms.Add(Room);
		}
	}

	DevLog->LogImportant(
		FString::Printf(TEXT("GenerateLayout: Spawned %d room(s) on FloorIndex %d (standalone, seed=0)"), SpawnedRooms.Num(), FloorIndex),
		EBGLogCategory::Generation);
}

void AFloorManager::ClearPreview()
{
	if (Visualizer)
	{
		Visualizer->ClearDebugDrawings();
	}
}

UTextRenderComponent* AFloorManager::CreateDebugTextComponent(FVector WorldPosition, FString Text, FColor Color, float Scale)
{
	UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(this);
	if (!TextComp) return nullptr;

	TextComp->RegisterComponent();
	TextComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	TextComp->SetWorldLocation(WorldPosition);
	TextComp->SetWorldRotation(Visualizer ? Visualizer->CoordinateTextRotation : FRotator(90.f, 180.f, 0.f)); // face up so labels read from above
	TextComp->SetText(FText::FromString(Text));
	TextComp->SetTextRenderColor(Color);
	TextComp->SetWorldSize(Scale * 24.f);
	TextComp->SetHorizontalAlignment(EHTA_Center);
	TextComp->SetVerticalAlignment(EVRTA_TextCenter);
	return TextComp;
}

void AFloorManager::DestroyDebugTextComponent(UTextRenderComponent* TextComp)
{
	if (IsValid(TextComp))
	{
		TextComp->DestroyComponent();
	}
}

#endif // WITH_EDITOR
