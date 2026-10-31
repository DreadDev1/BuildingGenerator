#include "MasterRoom.h"
#include "BuildingMathUtils.h"
#include "BGDevLog.h"
#include "RoomData.h"
#include "WallData.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Math/RandomStream.h"

#if WITH_EDITOR
#include "BGVisualizer.h"
#include "Components/TextRenderComponent.h"
#endif

// Defined further down; used by the floor pass for door thresholds.
static int32 GetDoorWidthCells(const UDoorData* DoorData);

AMasterRoom::AMasterRoom()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	FlavorISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FlavorISMC"));
	DevLog     = CreateDefaultSubobject<UBGDevLog>(TEXT("DevLog"));

	// FloorISMCPool, CeilingISMCPool, and WallISMCPool are populated dynamically — one ISMC per unique mesh.

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	FlavorISMC->SetupAttachment(GetRootComponent());

#if WITH_EDITOR
	Visualizer = CreateDefaultSubobject<UBGVisualizer>(TEXT("Visualizer"));
	Visualizer->OnCreateTextComponent.BindUObject(this, &AMasterRoom::CreateDebugTextComponent);
	Visualizer->OnDestroyTextComponent.BindUObject(this, &AMasterRoom::DestroyDebugTextComponent);
	Visualizer->OnRedrawRequested.BindUObject(this, &AMasterRoom::RedrawDebug);
#endif
}

// ============================================================
// Initialization
// ============================================================

void AMasterRoom::InitializeRoom(const FRoomPlacement& Placement, int32 Seed, int32 FloorRoomHeightCm)
{
	ActivePlacement = Placement;
	GenerationSeed  = Seed;
	RoomHeightCm    = FloorRoomHeightCm;

	GenerateRoomInterior();
}

void AMasterRoom::GenerateRoomInterior()
{
	DevLog->LogSectionHeader(TEXT("GenerateRoomInterior"));

	BuildGrid();
	ClassifyCells();
	PlaceFloorMeshes();
	PlaceCeilingMeshes();
	PlaceWallMeshStacks();
	ApplyForcedPlacements();
	PlaceFlavorMeshes();
	SpawnDoorTriggers();

#if WITH_EDITOR
	DrawDebugGrid();
#endif
}

// ============================================================
// Step 1 — BuildGrid
// ============================================================

void AMasterRoom::BuildGrid_Implementation()
{
	FRandomStream Stream(GenerationSeed ^ SeedOffset_BuildGrid);
	RoomGridSize = ResolveRoomGridSize(Stream);

	const int32 TotalCells = RoomGridSize.X * RoomGridSize.Y;
	CellGrid.SetNumUninitialized(TotalCells);
	for (ECellType& Cell : CellGrid)
	{
		Cell = ECellType::Floor;
	}

	DevLog->LogStatistic(TEXT("Grid size X"), RoomGridSize.X, EBGLogCategory::Grid);
	DevLog->LogStatistic(TEXT("Grid size Y"), RoomGridSize.Y, EBGLogCategory::Grid);
	DevLog->LogStatistic(TEXT("Total cells"), TotalCells, EBGLogCategory::Grid);
}

// ============================================================
// Step 2 — ClassifyCells
// ============================================================

void AMasterRoom::ClassifyCells_Implementation()
{
	const int32 MaxX = RoomGridSize.X - 1;
	const int32 MaxY = RoomGridSize.Y - 1;

	for (int32 Y = 0; Y < RoomGridSize.Y; ++Y)
	{
		for (int32 X = 0; X < RoomGridSize.X; ++X)
		{
			const bool bEdgeX = (X == 0 || X == MaxX);
			const bool bEdgeY = (Y == 0 || Y == MaxY);

			ECellType& Cell = CellGrid[CellIndex(X, Y)];

			if (bEdgeX && bEdgeY)
			{
				Cell = ECellType::Corner;
			}
			else if (bEdgeX || bEdgeY)
			{
				Cell = ECellType::Wall;
			}
			else
			{
				Cell = ECellType::Floor;
			}
		}
	}

	DevLog->LogImportant(TEXT("Cell classification complete"), EBGLogCategory::Grid);
}

// ============================================================
// Step 3 — PlaceFloorMeshes (Weighted random, multi-mesh, rotation support)
// ============================================================

void AMasterRoom::PlaceFloorMeshes_Implementation()
{
	const URoomData* RoomData = ActivePlacement.RoomDataAsset;
	if (!IsValid(RoomData) || !IsValid(RoomData->FloorData))
	{
		DevLog->LogCritical(TEXT("PlaceFloorMeshes: RoomData or FloorData is null"));
		return;
	}

	const UFloorData* FloorData = RoomData->FloorData;
	if (FloorData->FloorMeshes.IsEmpty())
	{
		DevLog->LogCritical(TEXT("PlaceFloorMeshes: FloorData has no mesh entries"));
		return;
	}

	bool bHasValid = false;
	for (const FFloorMeshEntry& E : FloorData->FloorMeshes)
	{
		if (IsValid(E.Mesh)) { bHasValid = true; break; }
	}
	if (!bHasValid)
	{
		DevLog->LogCritical(TEXT("PlaceFloorMeshes: no valid static mesh found in FloorData"));
		return;
	}

	// 1x1 fallback — only placed when every weighted candidate has been tried and none fit.
	const FFloorMeshEntry* FallbackEntry = nullptr;
	for (const FFloorMeshEntry& E : FloorData->FloorMeshes)
	{
		if (IsValid(E.Mesh) && E.CellsX == 1 && E.CellsY == 1)
		{
			FallbackEntry = &E;
			break;
		}
	}

	FRandomStream Stream(GenerationSeed ^ SeedOffset_FloorFill);

	// Uniform mode: pick one entry from the weighted pool before the fill loop starts.
	// Every cell uses that single entry — different seeds produce different uniform meshes.
	const bool bUniform = (FloorData->FillMode == EFloorFillMode::Uniform);
	const FFloorMeshEntry* UniformEntry = nullptr;
	if (bUniform)
	{
		float TotalWeight = 0.f;
		for (const FFloorMeshEntry& E : FloorData->FloorMeshes)
		{
			if (IsValid(E.Mesh)) TotalWeight += (E.Weight > 0.f ? E.Weight : 1.f);
		}
		float Pick = Stream.FRand() * TotalWeight;
		for (const FFloorMeshEntry& E : FloorData->FloorMeshes)
		{
			if (!IsValid(E.Mesh)) continue;
			Pick -= (E.Weight > 0.f ? E.Weight : 1.f);
			if (Pick <= 0.f) { UniformEntry = &E; break; }
		}
		if (!UniformEntry)
		{
			for (const FFloorMeshEntry& E : FloorData->FloorMeshes)
			{
				if (IsValid(E.Mesh)) { UniformEntry = &E; break; }
			}
		}
		DevLog->LogImportant(
			FString::Printf(TEXT("PlaceFloorMeshes: Uniform mode — mesh: %s"), *GetNameSafe(UniformEntry ? UniformEntry->Mesh : nullptr)),
			EBGLogCategory::Mesh);
	}

	TArray<bool> Claimed;
	Claimed.Init(false, CellGrid.Num());

	// Designer-forced meshes go first; the fill below skips their cells.
	PlaceForcedFloorMeshes(Claimed);

	int32 PlacedCount   = 0;
	int32 FallbackCount = 0;
	int32 SkippedCount  = 0;

	for (int32 Y = 0; Y < RoomGridSize.Y; ++Y)
	{
		for (int32 X = 0; X < RoomGridSize.X; ++X)
		{
			if (CellGrid[CellIndex(X, Y)] != ECellType::Floor) continue;
			if (Claimed[CellIndex(X, Y)]) continue;

			// Uniform: single-entry pool (same mesh every cell).
			// Random: full weighted pool — all candidates tried in weighted-random order.
			TArray<TPair<const FFloorMeshEntry*, float>> Pool;
			if (bUniform && UniformEntry)
			{
				Pool.Emplace(UniformEntry, 1.f);
			}
			else
			{
				for (const FFloorMeshEntry& E : FloorData->FloorMeshes)
				{
					if (IsValid(E.Mesh)) Pool.Emplace(&E, E.Weight > 0.f ? E.Weight : 1.f);
				}
			}

			bool bPlaced = false;

			while (!bPlaced && Pool.Num() > 0)
			{
				float PoolTotal = 0.f;
				for (const TPair<const FFloorMeshEntry*, float>& P : Pool) PoolTotal += P.Value;

				float Pick = Stream.FRand() * PoolTotal;
				int32 PickedIdx = Pool.Num() - 1;
				for (int32 i = 0; i < Pool.Num(); ++i)
				{
					Pick -= Pool[i].Value;
					if (Pick <= 0.f) { PickedIdx = i; break; }
				}

				const FFloorMeshEntry* Entry = Pool[PickedIdx].Key;
				Pool.RemoveAtSwap(PickedIdx);

				const bool bCanRotate = Entry->AllowRotation;
				const bool bNonSquare = (Entry->CellsX != Entry->CellsY);

				if (bNonSquare)
				{
					// Both footprint orientations are always tried to guarantee coverage at edge
					// cells where only one row/column of space remains next to a wall boundary.
					// AllowRotation randomizes which orientation is preferred; both are attempted.
					int32 ACX = Entry->CellsX;
					int32 ACY = Entry->CellsY;
					float AYaw = 0.f;

					if (bCanRotate && Stream.FRand() < 0.5f) { Swap(ACX, ACY); AYaw = 90.f; }

					if (CanPlaceMesh(X, Y, ACX, ACY, Claimed))
					{
						GetOrCreateFloorISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(X, Y, ACX, ACY, AYaw));
						ClaimCells(X, Y, ACX, ACY, Claimed);
						++PlacedCount;
						bPlaced = true;
					}
					else
					{
						Swap(ACX, ACY);
						AYaw = (AYaw == 0.f) ? 90.f : 0.f;
						if (CanPlaceMesh(X, Y, ACX, ACY, Claimed))
						{
							GetOrCreateFloorISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(X, Y, ACX, ACY, AYaw));
							ClaimCells(X, Y, ACX, ACY, Claimed);
							++PlacedCount;
							bPlaced = true;
						}
					}
				}
				else
				{
					const float Yaw = bCanRotate
						? static_cast<float>(Stream.RandRange(0, 3)) * 90.f
						: 0.f;

					if (CanPlaceMesh(X, Y, Entry->CellsX, Entry->CellsY, Claimed))
					{
						GetOrCreateFloorISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(X, Y, Entry->CellsX, Entry->CellsY, Yaw));
						ClaimCells(X, Y, Entry->CellsX, Entry->CellsY, Claimed);
						++PlacedCount;
						bPlaced = true;
					}
				}
			}

			if (!bPlaced)
			{
				if (FallbackEntry)
				{
					GetOrCreateFloorISMC(FallbackEntry->Mesh)->AddInstance(FTransform(FallbackEntry->MeshRotation) * MakeCellTransform(X, Y, 1, 1, 0.f));
					Claimed[CellIndex(X, Y)] = true;
					++FallbackCount;
				}
				else
				{
					DevLog->LogImportant(
						FString::Printf(TEXT("PlaceFloorMeshes: cell (%d,%d) unfillable — add a 1x1 entry to FloorData"), X, Y),
						EBGLogCategory::Mesh);
					++SkippedCount;
				}
			}
		}
	}

	// Door thresholds. Doors sit in the wall ring, which the fill above skips, so a door shallower than
	// the wall band leaves bare ground in front of it. DoorBaseMesh (if set) is placed across the door's
	// cells in door space (X = into the room, Y = along the wall); otherwise bFillFloorUnderDoor fills
	// those cells with this FloorData's 1 x N tiles.
	int32 ThresholdCount = 0;
	for (const FDoorPlacement& Door : ActivePlacement.Doors)
	{
		const UDoorData* DData = GetEffectiveDoorData(Door);
		if (!IsValid(DData)) continue;
		if (!IsValid(DData->DoorBaseMesh) && !DData->bFillFloorUnderDoor) continue;

		const int32 Width   = GetDoorWidthCells(DData);
		const bool  bAlongY = (Door.Face == ERoomFace::South || Door.Face == ERoomFace::North);
		const int32 Fixed   = (Door.Face == ERoomFace::South) ? 0
		                    : (Door.Face == ERoomFace::North) ? RoomGridSize.X - 1
		                    : (Door.Face == ERoomFace::West)  ? 0
		                    :                                   RoomGridSize.Y - 1;
		auto CellAt = [&](int32 i) { return bAlongY ? FIntPoint(Fixed, Door.CellOffset + i) : FIntPoint(Door.CellOffset + i, Fixed); };

		if (IsValid(DData->DoorBaseMesh))
		{
			const FIntPoint C0 = CellAt(0);
			const FTransform DoorSpace = MakeCellTransform(C0.X, C0.Y, bAlongY ? 1 : Width, bAlongY ? Width : 1,
			                                               FBuildingMath::ToYawDegrees(Door.Face));
			GetOrCreateFloorISMC(DData->DoorBaseMesh)->AddInstance(FTransform(DData->DoorBaseRotation) * DoorSpace);
			++ThresholdCount;
			continue;
		}

		// Fill from FloorData: weighted pick among 1 x N tiles that fit the rest of the door.
		int32 i = 0;
		while (i < Width)
		{
			const int32 Remaining = Width - i;
			auto Fits = [&](const FFloorMeshEntry& E)
			{
				return IsValid(E.Mesh) && FMath::Min(E.CellsX, E.CellsY) == 1 && FMath::Max(E.CellsX, E.CellsY) <= Remaining;
			};
			TArray<const FFloorMeshEntry*> Candidates;
			if (bUniform && UniformEntry && Fits(*UniformEntry)) Candidates.Add(UniformEntry);
			else for (const FFloorMeshEntry& E : FloorData->FloorMeshes) { if (Fits(E)) Candidates.Add(&E); }
			if (Candidates.IsEmpty())
			{
				DevLog->LogImportant(TEXT("PlaceFloorMeshes: no FloorData tile fits under a door — add a 1x1 entry"), EBGLogCategory::Mesh);
				break;
			}

			float Total = 0.f;
			for (const FFloorMeshEntry* E : Candidates) Total += (E->Weight > 0.f ? E->Weight : 1.f);
			float Pick = Stream.FRand() * Total;
			const FFloorMeshEntry* Entry = Candidates.Last();
			for (const FFloorMeshEntry* E : Candidates)
			{
				Pick -= (E->Weight > 0.f ? E->Weight : 1.f);
				if (Pick <= 0.f) { Entry = E; break; }
			}

			const int32 Len = FMath::Max(Entry->CellsX, Entry->CellsY);
			const int32 AX  = bAlongY ? 1 : Len;
			const int32 AY  = bAlongY ? Len : 1;
			const float Yaw = (Entry->CellsX == AX && Entry->CellsY == AY) ? 0.f : 90.f;
			const FIntPoint C = CellAt(i);
			GetOrCreateFloorISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(C.X, C.Y, AX, AY, Yaw));
			++ThresholdCount;
			i += Len;
		}
	}
	DevLog->LogStatistic(TEXT("Floor instances placed"),    PlacedCount,   EBGLogCategory::Mesh);
	DevLog->LogStatistic(TEXT("Fallback 1x1 cells"),        FallbackCount, EBGLogCategory::Mesh);
	DevLog->LogStatistic(TEXT("Skipped unfillable cells"),  SkippedCount,  EBGLogCategory::Mesh);
	DevLog->LogStatistic(TEXT("Door threshold pieces"),     ThresholdCount, EBGLogCategory::Mesh);
}

// ============================================================
// Step 4 — PlaceCeilingMeshes (Weighted random, multi-mesh, rotation support)
// ============================================================

void AMasterRoom::PlaceCeilingMeshes_Implementation()
{
	const URoomData* RoomData = ActivePlacement.RoomDataAsset;
	if (!IsValid(RoomData) || !IsValid(RoomData->CeilingData))
	{
		DevLog->LogCritical(TEXT("PlaceCeilingMeshes: RoomData or CeilingData is null"));
		return;
	}

	const UCeilingData* CeilingData = RoomData->CeilingData;
	if (CeilingData->CeilingMeshes.IsEmpty())
	{
		DevLog->LogCritical(TEXT("PlaceCeilingMeshes: CeilingData has no mesh entries"));
		return;
	}

	bool bHasValid = false;
	for (const FFloorMeshEntry& E : CeilingData->CeilingMeshes)
	{
		if (IsValid(E.Mesh)) { bHasValid = true; break; }
	}
	if (!bHasValid)
	{
		DevLog->LogCritical(TEXT("PlaceCeilingMeshes: no valid static mesh found in CeilingData"));
		return;
	}

	// 1x1 fallback — only placed when every weighted candidate has been tried and none fit.
	const FFloorMeshEntry* FallbackEntry = nullptr;
	for (const FFloorMeshEntry& E : CeilingData->CeilingMeshes)
	{
		if (IsValid(E.Mesh) && E.CellsX == 1 && E.CellsY == 1)
		{
			FallbackEntry = &E;
			break;
		}
	}

	// Z offset driven by AFloorManager::RoomHeightCm — passed into InitializeRoom().
	const float CeilingZ = static_cast<float>(RoomHeightCm);

	FRandomStream Stream(GenerationSeed ^ SeedOffset_CeilingFill);

	// Uniform mode: pick one entry from the weighted pool before the fill loop starts.
	const bool bUniform = (CeilingData->FillMode == EFloorFillMode::Uniform);
	const FFloorMeshEntry* UniformEntry = nullptr;
	if (bUniform)
	{
		float TotalWeight = 0.f;
		for (const FFloorMeshEntry& E : CeilingData->CeilingMeshes)
		{
			if (IsValid(E.Mesh)) TotalWeight += (E.Weight > 0.f ? E.Weight : 1.f);
		}
		float Pick = Stream.FRand() * TotalWeight;
		for (const FFloorMeshEntry& E : CeilingData->CeilingMeshes)
		{
			if (!IsValid(E.Mesh)) continue;
			Pick -= (E.Weight > 0.f ? E.Weight : 1.f);
			if (Pick <= 0.f) { UniformEntry = &E; break; }
		}
		if (!UniformEntry)
		{
			for (const FFloorMeshEntry& E : CeilingData->CeilingMeshes)
			{
				if (IsValid(E.Mesh)) { UniformEntry = &E; break; }
			}
		}
		DevLog->LogImportant(
			FString::Printf(TEXT("PlaceCeilingMeshes: Uniform mode — mesh: %s"), *GetNameSafe(UniformEntry ? UniformEntry->Mesh : nullptr)),
			EBGLogCategory::Mesh);
	}

	TArray<bool> Claimed;
	Claimed.Init(false, CellGrid.Num());

	int32 PlacedCount   = 0;
	int32 FallbackCount = 0;
	int32 SkippedCount  = 0;

	for (int32 Y = 0; Y < RoomGridSize.Y; ++Y)
	{
		for (int32 X = 0; X < RoomGridSize.X; ++X)
		{
			if (CellGrid[CellIndex(X, Y)] != ECellType::Floor) continue;
			if (Claimed[CellIndex(X, Y)]) continue;

			// Uniform: single-entry pool (same mesh every cell).
			// Random: full weighted pool — all candidates tried in weighted-random order.
			TArray<TPair<const FFloorMeshEntry*, float>> Pool;
			if (bUniform && UniformEntry)
			{
				Pool.Emplace(UniformEntry, 1.f);
			}
			else
			{
				for (const FFloorMeshEntry& E : CeilingData->CeilingMeshes)
				{
					if (IsValid(E.Mesh)) Pool.Emplace(&E, E.Weight > 0.f ? E.Weight : 1.f);
				}
			}

			bool bPlaced = false;

			while (!bPlaced && Pool.Num() > 0)
			{
				float PoolTotal = 0.f;
				for (const TPair<const FFloorMeshEntry*, float>& P : Pool) PoolTotal += P.Value;

				float Pick = Stream.FRand() * PoolTotal;
				int32 PickedIdx = Pool.Num() - 1;
				for (int32 i = 0; i < Pool.Num(); ++i)
				{
					Pick -= Pool[i].Value;
					if (Pick <= 0.f) { PickedIdx = i; break; }
				}

				const FFloorMeshEntry* Entry = Pool[PickedIdx].Key;
				Pool.RemoveAtSwap(PickedIdx);

				const bool bCanRotate = Entry->AllowRotation;
				const bool bNonSquare = (Entry->CellsX != Entry->CellsY);

				if (bNonSquare)
				{
					int32 ACX = Entry->CellsX;
					int32 ACY = Entry->CellsY;
					float AYaw = 0.f;

					if (bCanRotate && Stream.FRand() < 0.5f) { Swap(ACX, ACY); AYaw = 90.f; }

					if (CanPlaceMesh(X, Y, ACX, ACY, Claimed))
					{
						GetOrCreateCeilingISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(X, Y, ACX, ACY, AYaw, CeilingZ));
						ClaimCells(X, Y, ACX, ACY, Claimed);
						++PlacedCount;
						bPlaced = true;
					}
					else
					{
						Swap(ACX, ACY);
						AYaw = (AYaw == 0.f) ? 90.f : 0.f;
						if (CanPlaceMesh(X, Y, ACX, ACY, Claimed))
						{
							GetOrCreateCeilingISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(X, Y, ACX, ACY, AYaw, CeilingZ));
							ClaimCells(X, Y, ACX, ACY, Claimed);
							++PlacedCount;
							bPlaced = true;
						}
					}
				}
				else
				{
					const float Yaw = bCanRotate
						? static_cast<float>(Stream.RandRange(0, 3)) * 90.f
						: 0.f;

					if (CanPlaceMesh(X, Y, Entry->CellsX, Entry->CellsY, Claimed))
					{
						GetOrCreateCeilingISMC(Entry->Mesh)->AddInstance(FTransform(Entry->MeshRotation) * MakeCellTransform(X, Y, Entry->CellsX, Entry->CellsY, Yaw, CeilingZ));
						ClaimCells(X, Y, Entry->CellsX, Entry->CellsY, Claimed);
						++PlacedCount;
						bPlaced = true;
					}
				}
			}

			if (!bPlaced)
			{
				if (FallbackEntry)
				{
					GetOrCreateCeilingISMC(FallbackEntry->Mesh)->AddInstance(FTransform(FallbackEntry->MeshRotation) * MakeCellTransform(X, Y, 1, 1, 0.f, CeilingZ));
					Claimed[CellIndex(X, Y)] = true;
					++FallbackCount;
				}
				else
				{
					DevLog->LogImportant(
						FString::Printf(TEXT("PlaceCeilingMeshes: cell (%d,%d) unfillable — add a 1x1 entry to CeilingData"), X, Y),
						EBGLogCategory::Mesh);
					++SkippedCount;
				}
			}
		}
	}

	DevLog->LogStatistic(TEXT("Ceiling instances placed"),  PlacedCount,   EBGLogCategory::Mesh);
	DevLog->LogStatistic(TEXT("Fallback 1x1 cells"),        FallbackCount, EBGLogCategory::Mesh);
	DevLog->LogStatistic(TEXT("Skipped unfillable cells"),  SkippedCount,  EBGLogCategory::Mesh);
}
void AMasterRoom::ApplyForcedPlacements_Implementation(){}  // floors: see PlaceForcedFloorMeshes (Step 3)

void AMasterRoom::GatherForcedPlacements(TArray<const FForcedPlacement*>& Out) const
{
	Out.Reset();
	for (const FForcedPlacement& FP : ActivePlacement.ForcedPlacements) Out.Add(&FP);
	if (IsValid(ActivePlacement.RoomDataAsset))
	{
		for (const FForcedPlacement& FP : ActivePlacement.RoomDataAsset->ForcedPlacements) Out.Add(&FP);
	}
}

void AMasterRoom::PlaceForcedFloorMeshes(TArray<bool>& Claimed)
{
	TArray<const FForcedPlacement*> List;
	GatherForcedPlacements(List);

	int32 Placed = 0;
	for (int32 i = 0; i < List.Num(); ++i)
	{
		const FForcedPlacement& FP = *List[i];
		const int32 CX = FMath::Max(1, FP.CellsX);
		const int32 CY = FMath::Max(1, FP.CellsY);
		const FIntPoint C = FP.CellPosition;

		if (!IsValid(FP.Mesh))
		{
			DevLog->LogImportant(FString::Printf(TEXT("ForcedPlacement %d at (%d,%d): no mesh set — skipped"), i, C.X, C.Y), EBGLogCategory::Mesh);
			continue;
		}
		if (C.X < 0 || C.Y < 0 || !CanPlaceMesh(C.X, C.Y, CX, CY, Claimed))
		{
			DevLog->LogImportant(FString::Printf(
				TEXT("ForcedPlacement %d (%s) at (%d,%d) size %dx%d does not fit: it must lie on free floor cells inside the wall ring (floor cells are 1..%d, 1..%d) — skipped"),
				i, *GetNameSafe(FP.Mesh), C.X, C.Y, CX, CY, RoomGridSize.X - 2, RoomGridSize.Y - 2), EBGLogCategory::Mesh);
			continue;
		}

		GetOrCreateFloorISMC(FP.Mesh)->AddInstance(MakeCellTransform(C.X, C.Y, CX, CY, FP.RotationYaw));
		ClaimCells(C.X, C.Y, CX, CY, Claimed);
		++Placed;
	}

	if (List.Num() > 0)
	{
		DevLog->LogStatistic(TEXT("Forced floor placements"), Placed, EBGLogCategory::Mesh);
	}
}
void AMasterRoom::PlaceFlavorMeshes_Implementation()    {}
void AMasterRoom::SpawnDoorTriggers_Implementation()    {}

// ============================================================
// Step 5 — PlaceWallMeshStacks
// ============================================================

// Converts a face + position-along-face to a grid (X, Y) cell coordinate.
static void FaceToCell(ERoomFace Face, int32 FacePos, int32 GridW, int32 GridH, int32& OutX, int32& OutY)
{
	// North=+X / South=-X run along Y; East=+Y / West=-Y run along X.
	switch (Face)
	{
	case ERoomFace::North: OutX = GridW - 1; OutY = FacePos; break;
	case ERoomFace::South: OutX = 0;         OutY = FacePos; break;
	case ERoomFace::East:  OutX = FacePos;   OutY = GridH - 1; break;
	case ERoomFace::West:  OutX = FacePos;   OutY = 0;       break;
	default:               OutX = 0;         OutY = 0;        break;
	}
}

// Returns the door opening width in cells from UDoorData. Defaults to 2 (TwoCell) when null.
static int32 GetDoorWidthCells(const UDoorData* DoorData)
{
	if (!IsValid(DoorData)) return 2;
	switch (DoorData->DoorWidth)
	{
	case EDoorWidth::FourCell:  return 4;
	case EDoorWidth::ThreeCell: return 3;
	default:                    return 2;
	}
}

// A module needs a BaseMesh, plus a TopMesh unless it is a single-piece (bUseBaseMeshOnly) module.
static bool IsWallModuleUsable(const FWallModule& Mod)
{
	return IsValid(Mod.BaseMesh) && (Mod.bUseBaseMeshOnly || IsValid(Mod.TopMesh));
}

// Weighted random module selection. Returns the first valid module if no candidate fits.
static const FWallModule* PickWallModule(const UWallData* WallData, int32 Available, FRandomStream& Stream)
{
	float TotalWeight = 0.f;
	const FWallModule* Fallback = nullptr;

	for (const FWallModule& Mod : WallData->WallModules)
	{
		if (!IsWallModuleUsable(Mod)) continue;
		if (!Fallback) Fallback = &Mod;
		if (Mod.CellsWide > Available) continue;
		TotalWeight += Mod.Weight;
	}

	if (TotalWeight <= 0.f) return Fallback;

	float Pick = Stream.FRand() * TotalWeight;
	for (const FWallModule& Mod : WallData->WallModules)
	{
		if (!IsWallModuleUsable(Mod)) continue;
		if (Mod.CellsWide > Available) continue;
		Pick -= Mod.Weight;
		if (Pick <= 0.f) return &Mod;
	}

	return Fallback;
}

// How an adjusted slot is re-seated (see FWallMeshAdjust::bAutoAlign).
enum class EModuleAlign : uint8 { Wall, Corner };

// Mesh-local transform for one slot of a wall / corner module, plus the slot's bounds after it.
// A slot without rotation or scale only gets its Offset, so meshes are placed exactly as authored.
static FTransform MakeSlotTransform(const UStaticMesh* Mesh, const FWallMeshAdjust& Adjust, EModuleAlign Align, FBox& OutBounds)
{
	const FBox MeshBox = Mesh->GetBoundingBox();
	if (!Adjust.HasRotationOrScale())
	{
		OutBounds = MeshBox.ShiftBy(Adjust.Offset);
		return FTransform(Adjust.Offset);
	}

	const FBox Adjusted = MeshBox.TransformBy(FTransform(Adjust.Rotation, FVector::ZeroVector, Adjust.Scale));
	FVector Shift = Adjust.Offset;
	if (Adjust.bAutoAlign)
	{
		const FVector Center = Adjusted.GetCenter();
		Shift += (Align == EModuleAlign::Wall)
			? FVector(-Adjusted.Min.X, -Center.Y, -Adjusted.Min.Z)   // back face on X=0, centred along the face
			: FVector(-Center.X,       -Center.Y, -Adjusted.Min.Z);  // centred on the corner cell
	}
	OutBounds = Adjusted.ShiftBy(Shift);
	return FTransform(Adjust.Rotation, Shift, Adjust.Scale);
}

void AMasterRoom::PlaceWallMeshStacks_Implementation()
{
	const URoomData* RoomData = ActivePlacement.RoomDataAsset;
	if (!IsValid(RoomData) || !IsValid(RoomData->WallData))
	{
		DevLog->LogCritical(TEXT("PlaceWallMeshStacks: RoomData or WallData is null"));
		return;
	}

	const UWallData* WallData = RoomData->WallData;
	if (WallData->WallModules.IsEmpty())
	{
		DevLog->LogCritical(TEXT("PlaceWallMeshStacks: WallData has no modules"));
		return;
	}

	FRandomStream Stream(GenerationSeed ^ SeedOffset_WallFill);

	// Returns the FDoorPlacement* for the door whose column falls on FacePos
	// (CellOffset-1 or CellOffset+Width), but only when bUseColumns is true on that door's DoorData.
	// When bUseColumns is false, flanking cells are not reserved and receive normal wall modules.
	auto FindColumnForPos = [&](ERoomFace CurFace, int32 FacePos) -> const FDoorPlacement*
	{
		for (const FDoorPlacement& Door : ActivePlacement.Doors)
		{
			if (Door.Face != CurFace) continue;
			const UDoorData* DData = GetEffectiveDoorData(Door);
			if (!IsValid(DData) || !DData->bUseColumns) continue;
			const int32 Width = GetDoorWidthCells(DData);
			if (FacePos == Door.CellOffset - 1 || FacePos == Door.CellOffset + Width) return &Door;
		}
		return nullptr;
	};

	// Returns the FDoorPlacement* if FacePos is the start cell of a door span on CurFace.
	auto FindDoorAtStart = [&](ERoomFace CurFace, int32 FacePos) -> const FDoorPlacement*
	{
		for (const FDoorPlacement& Door : ActivePlacement.Doors)
		{
			if (Door.Face == CurFace && Door.CellOffset == FacePos) return &Door;
		}
		return nullptr;
	};

	// Returns true if FacePos falls inside any door span (columns not counted — spans only).
	auto IsDoorSpanPos = [&](ERoomFace CurFace, int32 FacePos) -> bool
	{
		for (const FDoorPlacement& Door : ActivePlacement.Doors)
		{
			if (Door.Face != CurFace) continue;
			const int32 Width = GetDoorWidthCells(GetEffectiveDoorData(Door));
			if (FacePos >= Door.CellOffset && FacePos < Door.CellOffset + Width) return true;
		}
		return false;
	};

	// Returns true if FacePos should not receive a regular wall module.
	// Column positions are only blocked when ColumnMesh is set; otherwise they get a wall module.
	auto IsBlockedPos = [&](ERoomFace CurFace, int32 FacePos) -> bool
	{
		if (FindColumnForPos(CurFace, FacePos)) return true;  // only matches when ColumnMesh is valid
		if (IsDoorSpanPos(CurFace, FacePos)) return true;
		return false;
	};

	// Places one module's layers (Base, Middle1, Middle2, Top) on top of each other.
	// MakeBase(Z) returns the module's placement transform for a layer starting at height Z.
	// Layer heights come from the StackHeight overrides, else from each slot's (adjusted) bounds.
	auto PlaceModuleStack = [&](const FWallModule& Mod, EModuleAlign Align, TFunctionRef<FTransform(float)> MakeBase)
	{
		struct FSlot { UStaticMesh* Mesh; const FWallMeshAdjust* Adjust; float Override; };
		const FSlot Slots[] = {
			{ Mod.BaseMesh,    &Mod.BaseMeshAdjust,    Mod.BaseMeshStackHeight    },
			{ Mod.MiddleMesh1, &Mod.MiddleMesh1Adjust, Mod.MiddleMesh1StackHeight },
			{ Mod.MiddleMesh2, &Mod.MiddleMesh2Adjust, Mod.MiddleMesh2StackHeight },
			{ Mod.TopMesh,     &Mod.TopMeshAdjust,     0.f                        },
		};
		const int32 NumSlots = Mod.bUseBaseMeshOnly ? 1 : UE_ARRAY_COUNT(Slots);

		float Z = 0.f;
		for (int32 i = 0; i < NumSlots; ++i)
		{
			const FSlot& S = Slots[i];
			if (!IsValid(S.Mesh)) continue;

			FBox Bounds;
			const FTransform Local = MakeSlotTransform(S.Mesh, *S.Adjust, Align, Bounds);
			GetOrCreateWallISMC(S.Mesh)->AddInstance(Local * MakeBase(Z));
			Z += (S.Override > 0.f) ? S.Override : Bounds.GetSize().Z;
		}
	};
	int32 StacksPlaced     = 0;
	int32 DoorsPlaced      = 0;
	int32 ColumnsPlaced    = 0;
	int32 DoorCellsSkipped = 0;

	// Process each of the four faces. For North/South, FacePos runs along Y.
	// For East/West, FacePos runs along X. Corner cells at 0 and FaceLen-1 are excluded.
	const ERoomFace Faces[] = { ERoomFace::North, ERoomFace::South, ERoomFace::East, ERoomFace::West };

	for (ERoomFace Face : Faces)
	{
		const bool  bNS     = (Face == ERoomFace::North || Face == ERoomFace::South);
		const int32 FaceLen = bNS ? RoomGridSize.Y : RoomGridSize.X;
		const int32 PosMin  = 1;
		const int32 PosMax  = FaceLen - 2;  // inclusive; corners at 0 and FaceLen-1 are Corner cells

		if (PosMax < PosMin) continue;

		int32 Pos = PosMin;
		while (Pos <= PosMax)
		{
			// 1. Column position — only reached when bUseColumns is true on the door's DoorData.
			//    If ColumnMesh is set, it is placed. If null, the cell is reserved but left empty.
			//    When bUseColumns is false, FindColumnForPos returns nullptr and the flanking
			//    cell falls through to the regular wall module bin-pack instead.
			if (const FDoorPlacement* ColDoor = FindColumnForPos(Face, Pos))
			{
				const UDoorData* DData = GetEffectiveDoorData(*ColDoor);
				if (IsValid(DData) && IsValid(DData->ColumnMesh))
				{
					GetOrCreateWallISMC(DData->ColumnMesh)->AddInstance(
						MakeWallTransform(Pos, Face, 1, 0.f, DData->ColumnPlacementOffset));
					++ColumnsPlaced;
				}
				++Pos;
				continue;
			}

			// 2. Door span start — place DoorMesh (and FrameMesh, if set) spanning Width cells.
			if (const FDoorPlacement* DoorP = FindDoorAtStart(Face, Pos))
			{
				const UDoorData* DData = GetEffectiveDoorData(*DoorP);
				const int32 Width = GetDoorWidthCells(DData);
				if (IsValid(DData) && IsValid(DData->DoorMesh))
				{
					GetOrCreateWallISMC(DData->DoorMesh)->AddInstance(
						MakeWallTransform(Pos, Face, Width, 0.f, DData->DoorPlacementOffset));
					// FrameMesh (optional) shares the door's transform, so a frame authored in the door mesh's
					// space lands seated in its opening.
					if (IsValid(DData->FrameMesh))
					{
						GetOrCreateWallISMC(DData->FrameMesh)->AddInstance(
							MakeWallTransform(Pos, Face, Width, 0.f, DData->DoorPlacementOffset));
					}
					// TopMesh (optional) stacks above the door, using the wall-slot adjustment and alignment.
					if (IsValid(DData->TopMesh))
					{
						const float TopZ = (DData->TopMeshZ > 0.f) ? DData->TopMeshZ : DData->DoorMesh->GetBoundingBox().Max.Z;
						FBox TopBounds;
						const FTransform TopLocal = MakeSlotTransform(DData->TopMesh, DData->TopMeshAdjust, EModuleAlign::Wall, TopBounds);
						GetOrCreateWallISMC(DData->TopMesh)->AddInstance(
							TopLocal * MakeWallTransform(Pos, Face, Width, TopZ, DData->DoorPlacementOffset));
					}
					++DoorsPlaced;
				}
				else
				{
					DoorCellsSkipped += Width;
				}
				Pos += Width;
				continue;
			}

			// 3. Interior door span cell that wasn't caught by a start (designer error — skip gracefully).
			if (IsDoorSpanPos(Face, Pos))
			{
				++DoorCellsSkipped;
				++Pos;
				continue;
			}

			// 4. Regular wall module — count the available run before the next blocked position.
			int32 Available = 0;
			for (int32 P = Pos; P <= PosMax; ++P)
			{
				if (IsBlockedPos(Face, P)) break;
				++Available;
			}

			if (Available == 0)
			{
				++Pos;
				continue;
			}

			const FWallModule* Mod = PickWallModule(WallData, Available, Stream);
			if (!Mod)
			{
				DevLog->LogCritical(TEXT("PlaceWallMeshStacks: no valid module found — check WallData"));
				break;
			}

			PlaceModuleStack(*Mod, EModuleAlign::Wall, [&](float Z)
			{
				return MakeWallTransform(Pos, Face, Mod->CellsWide, Z, Mod->PlacementOffset);
			});

			Pos += Mod->CellsWide;
			++StacksPlaced;
		}
	}

	// Place corner pieces at all four ECellType::Corner cells.
	// CornerModule (a stacked corner) wins when its BaseMesh is set; otherwise the legacy CornerMesh
	// is used. Both are optional — neither set means this wall style has no corner geometry.
	int32 CornersPlaced = 0;
	const FWallModule& CornerModule = WallData->CornerModule;
	const bool bUseCornerModule = IsValid(CornerModule.BaseMesh);
	if (bUseCornerModule || IsValid(WallData->CornerMesh))
	{
		const int32 MaxX = RoomGridSize.X - 1;
		const int32 MaxY = RoomGridSize.Y - 1;

		// Yaw convention: mesh at 0° has its interior-facing corner pointing NE (+X,+Y).
		// Each 90° CCW rotation moves to the next corner around the room.
		// With North=+X / East=+Y: minX=South, maxX=North, minY=West, maxY=East.
		struct FCornerDef { int32 X; int32 Y; float Yaw; };
		const FCornerDef Corners[] = {
			{ 0,    0,    0.f   },   // SW — interior corner faces NE
			{ MaxX, 0,    90.f  },   // NW — interior corner faces SE
			{ MaxX, MaxY, 180.f },   // NE — interior corner faces SW
			{ 0,    MaxY, 270.f },   // SE — interior corner faces NW
		};

		for (const FCornerDef& C : Corners)
		{
			if (bUseCornerModule)
			{
				PlaceModuleStack(CornerModule, EModuleAlign::Corner, [&](float Z)
				{
					// PlacementOffset is in corner space, so it turns with the corner.
					return FTransform(CornerModule.PlacementOffset) * MakeCellTransform(C.X, C.Y, 1, 1, C.Yaw, Z);
				});
			}
			else
			{
				GetOrCreateWallISMC(WallData->CornerMesh)->AddInstance(MakeCellTransform(C.X, C.Y, 1, 1, C.Yaw));
			}
			++CornersPlaced;
		}
	}

	DevLog->LogStatistic(TEXT("Wall stacks placed"),   StacksPlaced,     EBGLogCategory::Wall);
	DevLog->LogStatistic(TEXT("Door meshes placed"),   DoorsPlaced,      EBGLogCategory::Wall);
	DevLog->LogStatistic(TEXT("Column meshes placed"), ColumnsPlaced,    EBGLogCategory::Wall);
	DevLog->LogStatistic(TEXT("Corner pieces placed"), CornersPlaced,    EBGLogCategory::Wall);
	DevLog->LogStatistic(TEXT("Door cells skipped"),   DoorCellsSkipped, EBGLogCategory::Wall);
}

// ============================================================
// Wall helpers
// ============================================================

ERoomFace AMasterRoom::GetFaceForWallCell(int32 X, int32 Y) const
{
	// North=+X / South=-X on the X edges; East=+Y / West=-Y on the Y edges.
	if (X == 0)                  return ERoomFace::South;
	if (X == RoomGridSize.X - 1) return ERoomFace::North;
	if (Y == 0)                  return ERoomFace::West;
	return ERoomFace::East;
}

FTransform AMasterRoom::MakeWallTransform(int32 FacePos, ERoomFace Face, int32 CellsWide, float Z, const FVector& PlacementOffset) const
{
	// Pivot convention: BottomBackCenter — placed at the exterior face of the room,
	// centered along the module's width. The mesh extends inward from that point.
	constexpr float CellSize   = 100.f;
	const float     FaceCenter = (FacePos + CellsWide * 0.5f) * CellSize;
	const float     Yaw        = FBuildingMath::ToYawDegrees(Face);

	// North=+X / South=-X sit on the X-extent planes (FaceCenter runs along Y);
	// East=+Y / West=-Y sit on the Y-extent planes (FaceCenter runs along X).
	FVector BasePos;
	switch (Face)
	{
	case ERoomFace::South: BasePos = FVector(0.f,                        FaceCenter, Z); break;
	case ERoomFace::North: BasePos = FVector(RoomGridSize.X * CellSize,  FaceCenter, Z); break;
	case ERoomFace::West:  BasePos = FVector(FaceCenter,                 0.f,        Z); break;
	case ERoomFace::East:  BasePos = FVector(FaceCenter,  RoomGridSize.Y * CellSize, Z); break;
	default:               BasePos = FVector::ZeroVector;                                break;
	}

	// Rotate the module's local offset (X=into-room, Y=along-face) into component space.
	const FVector WorldOffset = FRotator(0.f, Yaw, 0.f).RotateVector(PlacementOffset);

	return FTransform(FRotator(0.f, Yaw, 0.f), BasePos + WorldOffset);
}

UInstancedStaticMeshComponent* AMasterRoom::GetOrCreateFloorISMC(UStaticMesh* Mesh)
{
	for (UInstancedStaticMeshComponent* ISMC : FloorISMCPool)
	{
		if (IsValid(ISMC) && ISMC->GetStaticMesh() == Mesh)
		{
			return ISMC;
		}
	}

	UInstancedStaticMeshComponent* NewISMC = NewObject<UInstancedStaticMeshComponent>(this);
	NewISMC->SetStaticMesh(Mesh);
	NewISMC->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	NewISMC->RegisterComponent();
	FloorISMCPool.Add(NewISMC);
	return NewISMC;
}

UInstancedStaticMeshComponent* AMasterRoom::GetOrCreateCeilingISMC(UStaticMesh* Mesh)
{
	for (UInstancedStaticMeshComponent* ISMC : CeilingISMCPool)
	{
		if (IsValid(ISMC) && ISMC->GetStaticMesh() == Mesh)
		{
			return ISMC;
		}
	}

	UInstancedStaticMeshComponent* NewISMC = NewObject<UInstancedStaticMeshComponent>(this);
	NewISMC->SetStaticMesh(Mesh);
	NewISMC->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	NewISMC->RegisterComponent();
	CeilingISMCPool.Add(NewISMC);
	return NewISMC;
}

UInstancedStaticMeshComponent* AMasterRoom::GetOrCreateWallISMC(UStaticMesh* Mesh)
{
	for (UInstancedStaticMeshComponent* ISMC : WallISMCPool)
	{
		if (IsValid(ISMC) && ISMC->GetStaticMesh() == Mesh)
		{
			return ISMC;
		}
	}

	UInstancedStaticMeshComponent* NewISMC = NewObject<UInstancedStaticMeshComponent>(this);
	NewISMC->SetStaticMesh(Mesh);
	NewISMC->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	NewISMC->RegisterComponent();
	WallISMCPool.Add(NewISMC);
	return NewISMC;
}

// ============================================================
// Exterior wall suppression (Step 11)
// ============================================================

void AMasterRoom::SuppressExteriorWallISM(ERoomFace Face)
{
	// Stub — implemented in Step 11
}

// ============================================================
// Mesh Fill Helpers
// ============================================================

bool AMasterRoom::CanPlaceMesh(int32 X, int32 Y, int32 CellsX, int32 CellsY, const TArray<bool>& Claimed) const
{
	// Reject if footprint would exceed grid bounds
	if (X + CellsX > RoomGridSize.X || Y + CellsY > RoomGridSize.Y) return false;

	for (int32 DY = 0; DY < CellsY; ++DY)
	{
		for (int32 DX = 0; DX < CellsX; ++DX)
		{
			const int32 Idx = CellIndex(X + DX, Y + DY);
			if (CellGrid[Idx] != ECellType::Floor) return false;
			if (Claimed[Idx])                      return false;
		}
	}
	return true;
}

void AMasterRoom::ClaimCells(int32 X, int32 Y, int32 CellsX, int32 CellsY, TArray<bool>& Claimed)
{
	for (int32 DY = 0; DY < CellsY; ++DY)
	{
		for (int32 DX = 0; DX < CellsX; ++DX)
		{
			Claimed[CellIndex(X + DX, Y + DY)] = true;
		}
	}
}

FTransform AMasterRoom::MakeCellTransform(int32 X, int32 Y, int32 CellsX, int32 CellsY, float Yaw, float Z) const
{
	// Transforms are in component-local space (relative to the actor root).
	// ISMC::AddInstance() uses bWorldSpace=false by default, so no actor offset is added here.
	// Pivot convention: BottomCenter — position the center of the mesh footprint at height Z.
	constexpr float CellSize = 100.f;
	const FVector LocalPos(
		(X + CellsX * 0.5f) * CellSize,
		(Y + CellsY * 0.5f) * CellSize,
		Z
	);
	return FTransform(FRotator(0.f, Yaw, 0.f), LocalPos);
}

// ============================================================
// Helpers
// ============================================================

FIntPoint AMasterRoom::ResolveRoomGridSize(FRandomStream& Stream) const
{
	if (ActivePlacement.SizeOverride != FIntPoint::ZeroValue)
	{
		return ActivePlacement.SizeOverride;
	}

	const URoomData* RoomData = ActivePlacement.RoomDataAsset;
	if (!RoomData)
	{
		DevLog->LogCritical(TEXT("ResolveRoomGridSize: RoomDataAsset is null — using fallback 4x4"));
		return FIntPoint(4, 4);
	}

	const int32 X = Stream.RandRange(RoomData->MinRoomSize.X, RoomData->MaxRoomSize.X);
	const int32 Y = Stream.RandRange(RoomData->MinRoomSize.Y, RoomData->MaxRoomSize.Y);
	return FIntPoint(X, Y);
}

const UDoorData* AMasterRoom::GetEffectiveDoorData(const FDoorPlacement& Door) const
{
	if (IsValid(Door.DoorDataOverride)) return Door.DoorDataOverride;
	if (IsValid(ActivePlacement.RoomDataAsset)) return ActivePlacement.RoomDataAsset->DoorData;
	return nullptr;
}

bool AMasterRoom::IsDoorCell(int32 X, int32 Y) const
{
	const int32 MaxX = RoomGridSize.X - 1;
	const int32 MaxY = RoomGridSize.Y - 1;

	for (const FDoorPlacement& Door : ActivePlacement.Doors)
	{
		const int32 Width = GetDoorWidthCells(GetEffectiveDoorData(Door));

		// North=+X / South=-X edges span along Y; East=+Y / West=-Y edges span along X.
		switch (Door.Face)
		{
		case ERoomFace::North:
			if (X == MaxX && Y >= Door.CellOffset && Y < Door.CellOffset + Width) return true;
			break;
		case ERoomFace::South:
			if (X == 0 && Y >= Door.CellOffset && Y < Door.CellOffset + Width) return true;
			break;
		case ERoomFace::East:
			if (Y == MaxY && X >= Door.CellOffset && X < Door.CellOffset + Width) return true;
			break;
		case ERoomFace::West:
			if (Y == 0 && X >= Door.CellOffset && X < Door.CellOffset + Width) return true;
			break;
		}
	}
	return false;
}

// ============================================================
// Editor Debug Draw
// ============================================================

#if WITH_EDITOR

void AMasterRoom::PreviewRoom()
{
	for (UInstancedStaticMeshComponent* ISMC : FloorISMCPool)
	{
		if (IsValid(ISMC)) ISMC->ClearInstances();
	}
	for (UInstancedStaticMeshComponent* ISMC : CeilingISMCPool)
	{
		if (IsValid(ISMC)) ISMC->ClearInstances();
	}
	for (UInstancedStaticMeshComponent* ISMC : WallISMCPool)
	{
		if (IsValid(ISMC)) ISMC->ClearInstances();
	}
	FlavorISMC->ClearInstances();
	if (Visualizer) Visualizer->ClearDebugDrawings();

	InitializeRoom(PreviewPlacement, PreviewSeed, PreviewRoomHeightCm);
}

void AMasterRoom::ClearPreview()
{
	for (UInstancedStaticMeshComponent* ISMC : FloorISMCPool)
	{
		if (IsValid(ISMC)) ISMC->ClearInstances();
	}
	for (UInstancedStaticMeshComponent* ISMC : CeilingISMCPool)
	{
		if (IsValid(ISMC)) ISMC->ClearInstances();
	}
	for (UInstancedStaticMeshComponent* ISMC : WallISMCPool)
	{
		if (IsValid(ISMC)) ISMC->ClearInstances();
	}
	FlavorISMC->ClearInstances();
	if (Visualizer) Visualizer->ClearDebugDrawings();
}

void AMasterRoom::RedrawDebug()
{
	if (!Visualizer) return;
	Visualizer->ClearDebugDrawings();
	DrawDebugGrid();
}

void AMasterRoom::DrawDebugGrid() const
{
	if (!Visualizer) return;

	constexpr float CellSize = 100.f;
	const FVector Origin = GetActorLocation();

	// Derive the editor-only overlay cell lists from current room state.
	// Void / Occupied have no backing data until Step 12 — empty for now.
	TArray<FIntPoint> DoorCells;
	TArray<FIntPoint> CustomCells;
	const TArray<FIntPoint> VoidCells;
	const TArray<FIntPoint> OccupiedCells;
	BuildDoorCells(DoorCells);
	BuildCustomCells(CustomCells);

	Visualizer->DrawRoomGrid(RoomGridSize, CellGrid, DoorCells, CustomCells, VoidCells, OccupiedCells, CellSize, Origin);

	// Forced-empty overlays (data arrives in Step 12; exercised with empty arrays).
	const TArray<FBGForcedEmptyRegion> ForcedEmptyRegions;
	Visualizer->DrawForcedEmptyRegions(ForcedEmptyRegions, CellSize, Origin);
	Visualizer->DrawForcedEmptyCells(VoidCells, CellSize, Origin);
}

void AMasterRoom::BuildDoorCells(TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	for (int32 Y = 0; Y < RoomGridSize.Y; ++Y)
	{
		for (int32 X = 0; X < RoomGridSize.X; ++X)
		{
			if (IsDoorCell(X, Y))
			{
				OutCells.Emplace(X, Y);
			}
		}
	}
}

void AMasterRoom::BuildCustomCells(TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	TArray<const FForcedPlacement*> List;
	GatherForcedPlacements(List);

	// Every cell of each footprint, so the whole forced area shows yellow.
	for (const FForcedPlacement* FP : List)
	{
		for (int32 DY = 0; DY < FMath::Max(1, FP->CellsY); ++DY)
		{
			for (int32 DX = 0; DX < FMath::Max(1, FP->CellsX); ++DX)
			{
				const FIntPoint C(FP->CellPosition.X + DX, FP->CellPosition.Y + DY);
				if (C.X >= 0 && C.Y >= 0 && C.X < RoomGridSize.X && C.Y < RoomGridSize.Y) OutCells.AddUnique(C);
			}
		}
	}
}

UTextRenderComponent* AMasterRoom::CreateDebugTextComponent(FVector WorldPosition, FString Text, FColor Color, float Scale)
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

void AMasterRoom::DestroyDebugTextComponent(UTextRenderComponent* TextComp)
{
	if (IsValid(TextComp))
	{
		TextComp->DestroyComponent();
	}
}
#endif
