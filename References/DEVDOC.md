# Building Generator — Developer Reference

This document tracks what has been implemented, how each system works, and how the pieces
connect. It is written for developers joining the project mid-stream. Read CLAUDE.md first
for the full architectural spec, then use this file to understand what actually exists and why.

Update this document each time a step from CLAUDE.md Section 13 is completed.

---

## Table of Contents

1. [Project Setup and Conventions](#1-project-setup-and-conventions)
2. [Foundational Infrastructure](#2-foundational-infrastructure)
3. [Step 1 — Data Asset Schema](#3-step-1--data-asset-schema)
4. [Step 2 — AMasterRoom Grid and Cell Classification](#4-step-2--amasterroom-grid-and-cell-classification)
5. [Step 3 — Weighted Random Floor Fill](#5-step-3--weighted-random-floor-fill)
6. [Step 3b — Ceiling Mesh Fill](#6-step-3b--ceiling-mesh-fill)
7. [Step 4 — Wall Mesh Stack Placement](#7-step-4--wall-mesh-stack-placement)
8. [Step 5 — AFloorManager Details Panel](#8-step-5--afloormanager-details-panel)
9. [Step 6 — PreviewLayout Debug Visualization](#9-step-6--previewlayout-debug-visualization)
10. [Step 7 — ABuildingManager Spawning](#10-step-7--abuildingmanager-spawning)
11. [System Flow Diagram](#11-system-flow-diagram)
12. [Pending Investigations](#12-pending-investigations)
13. [Adding a New Developer Checklist](#13-adding-a-new-developer-checklist)

---

## 1. Project Setup and Conventions

**Engine**: Unreal Engine 5.8  
**Language**: C++ primary. Blueprints are for designer access only — no core logic in Blueprints.  
**IDE**: Rider  
**Module name**: `BuildingGenerator`  
**Source root**: `Source/BuildingGenerator/`  
**Headers**: `Source/BuildingGenerator/Public/`  
**Implementations**: `Source/BuildingGenerator/Private/`

### Build Module

`BuildingGenerator.Build.cs` declares all module dependencies.

| Dependency | Reason |
|---|---|
| `Core`, `CoreUObject`, `Engine`, `InputCore` | Standard UE5 baseline |
| `NetCore` | Required for multiplayer replication (seed broadcast, door state) |
| `UnrealEd` (editor only) | Required for `WITH_EDITOR` debug draw and CallInEditor buttons |

`EnhancedInput` was present in the project template and has been removed — this project does not use it.

### Cell Size Convention

The entire system is built on a **100 cm cell grid**. One cell = 100 cm x 100 cm in world space.
All mesh sizes, grid coordinates, and placement offsets are expressed in cell units unless
explicitly noted as world units (cm). Never mix the two without a conversion.

### Log Category

All project code logs through `LogBuildingGenerator` instead of `LogTemp`.

```cpp
UE_LOG(LogBuildingGenerator, Warning, TEXT("My message"));
```

Declared in `BuildingGenerator.h`, defined in `BuildingGenerator.cpp`. In the Output Log,
filter by `LogBuildingGenerator` to see only project output — no engine noise.

---

## 2. Foundational Infrastructure

These files were created before the numbered steps. They are shared utilities that every
subsequent system depends on.

---

### 2.1 CellTypes.h — `ECellType`

**File**: `Public/CellTypes.h`

Defines the three states a grid cell can be in after `ClassifyCells()` runs:

| Value | Meaning |
|---|---|
| `Floor` | Interior cell — receives floor mesh, ceiling mesh, flavor mesh |
| `Wall` | Edge cell (non-corner) — receives wall mesh stack or door mesh |
| `Corner` | Corner cell — receives corner piece (no door allowed here) |

This enum is shared between `AMasterRoom` (which classifies cells) and `UBGVisualizer`
(which colors cells for visualization). It lives in its own header to avoid circular
includes — both systems include `CellTypes.h` independently.

---

### 2.2 UBGDevLog + UBGVisualizer — Debug Infrastructure

**Files**:
- `Public/BGDevLog.h`, `Private/BGDevLog.cpp` — developer logging component
- `Public/BGVisualizer.h`, `Private/BGVisualizer.cpp` — editor visualization component

`UDebugLog` was refactored into two components. The split follows the principle that
logging (which must survive packaging) and visualization (editor-only) are separate concerns.
Both components are present on `AMasterRoom` and `AFloorManager`.

---

#### UBGDevLog — Developer Logging

`UBGDevLog` is an `ActorComponent` that provides filtered logging and performance profiling.
`UE_LOG` calls compile in all builds; the on-screen overlay is gated inside `ScreenLog()`
under `#if WITH_EDITOR`.

**Log levels** (`EBGLogLevel`):

| Level | Numeric | Shown for |
|---|---|---|
| `None` | 0 | Nothing |
| `Critical` | 1 | Errors that bypass all filters |
| `Important` | 2 | Key events (default) |
| `Verbose` | 3 | Per-cell detail |
| `Everything` | 4 | All output |

`LogCritical()` bypasses `bEnableDebug` and the level filter entirely.
Use it for conditions that should never happen (null asset pointers, missing data).

**Log categories** (`EBGLogCategory`):

| Category | Short tag | Use for |
|---|---|---|
| `Generation` | `[GEN]` | Overall generation sequence |
| `Grid` | `[GRID]` | Cell allocation and classification |
| `Mesh` | `[MESH]` | Floor, ceiling, flavor placement |
| `Wall` | `[WALL]` | Wall stack and door mesh placement |
| `Door` | `[DOOR]` | Door trigger spawning and state |
| `Replication` | `[NET]` | Seed broadcast, client regeneration |
| `Performance` | `[PERF]` | Timing results |
| `General` | `[GENERAL]` | Anything that doesn't fit above |

**Log methods:**

```cpp
DevLog->LogCritical(TEXT("RoomDataAsset is null"));
DevLog->LogImportant(TEXT("Grid built"), EBGLogCategory::Grid);
DevLog->LogVerbose(TEXT("Cell (2,3) classified as Wall"), EBGLogCategory::Grid);
DevLog->LogStatistic(TEXT("Total cells"), TotalCells, EBGLogCategory::Grid);
DevLog->LogSectionHeader(TEXT("GenerateRoomInterior"));
```

**Performance profiling** (`FBGPerformanceLog`):

```cpp
DevLog->BeginPerformanceLog(TEXT("BuildGrid"));
// ... work ...
DevLog->EndPerformanceLog(TEXT("BuildGrid")); // logs "[PERF] BuildGrid completed in 0.12ms"
```

Results are stored in `DevLog->GetPerformanceLogs()` as `TArray<FBGPerformanceLog>`.

**Screen logging**: In editor builds, all log calls appear as on-screen debug messages
via `GEngine->AddOnScreenDebugMessage()`, color-coded by level. The `UE_LOG` path always
compiles — screen display is the only editor-only part.

---

#### UBGVisualizer — Editor Visualization

`UBGVisualizer` is an editor-only `ActorComponent`. The entire class (header and .cpp)
is gated behind `#if WITH_EDITORONLY_DATA` — zero cost in packaged builds.

**6-state display classification** (`EBGCellDisplayState`):

| State | Default color | Meaning |
|---|---|---|
| `Empty` | Blue | Interior floor cell, no placed mesh |
| `Occupied` | Red | Floor cell with a placed mesh (flavor, etc.) |
| `Custom` | Yellow | Floor cell from a ForcedPlacement override |
| `Void` | Teal | Designer-reserved empty region |
| `Wall` | Black | Wall or Corner structural cell |
| `Door` | Green | Wall cell within a door span |

Display state is derived at draw time from the structural `ECellType` grid plus optional
overlay cell lists (`DoorCells`, `CustomCells`, `VoidCells`, `OccupiedCells`). It is
never stored on the grid itself.

**Delegates:**

| Delegate | Signature | Purpose |
|---|---|---|
| `OnCreateTextComponent` | `UTextRenderComponent*(FVector, FString, FColor, float)` | Owner creates and returns a text render component |
| `OnDestroyTextComponent` | `void(UTextRenderComponent*)` | Owner destroys a specific component |
| `OnRedrawRequested` | `void()` | Fired by toggle buttons; bound to `PreviewLayout` or `RedrawDebug` on the owner |

**Room-level draw API** (used by `AMasterRoom::DrawDebugGrid`):

```cpp
Visualizer->DrawRoomGrid(
    GridSize, StructuralCells,
    DoorCells, CustomCells, VoidCells, OccupiedCells,
    CellSize, OriginLocation);
```

**Primitive draw API** (used by `AFloorManager::PreviewLayout`):

```cpp
Visualizer->DrawBoundaryBox(FloorGridSize, CellSize, Origin, ZOffset);
Visualizer->DrawGridLines(FloorGridSize, CellSize, Origin);
Visualizer->DrawGridCoordinates(GridSize, CellSize, Origin, CoordStep, ZOffset);
Visualizer->DrawBox(WorldCenter, Extent, Color, Thickness);
Visualizer->DrawCellBox(GridCoord, Color, CellSize, Origin, ZOffset);
UTextRenderComponent* Label = Visualizer->CreateLabel(WorldPos, Text, Color);
```

**Toggle buttons** (Details panel, `Debug|Toggle` category):

| Button | Effect |
|---|---|
| `ToggleGrid` | Flip `bShowGrid`, fire `OnRedrawRequested` |
| `ToggleCoordinates` | Flip `bShowCoordinates`, fire `OnRedrawRequested` |
| `ToggleCellStates` | Flip `bShowCellStates`, fire `OnRedrawRequested` |
| `ToggleForcedRegions` | Flip both `bShowForcedEmptyRegions` and `bShowForcedEmptyCells` |

Pressing any toggle button fires `OnRedrawRequested`, which is bound to `PreviewLayout`
or `RedrawDebug` on the owning actor — the view updates immediately.

**Cleanup:**

```cpp
Visualizer->ClearDebugDrawings(); // FlushPersistentDebugLines + FlushDebugStrings + destroys all text components
Visualizer->ClearTextComponents(); // text components only
```

> `ClearDebugDrawings()` calls both `FlushPersistentDebugLines()` and `FlushDebugStrings()`.
> Both are global operations — all persistent debug draw in the scene is cleared, not just
> what this component drew. Do not mix `AFloorManager` and `AMasterRoom` previews at the
> same time if you need both visible simultaneously.

---

## 3. Step 1 — Data Asset Schema

**Status**: Complete  
**CLAUDE.md reference**: Section 3 (data hierarchy), Section 4 (key structs)

Step 1 creates all the data configuration classes that designers fill in via the
Content Browser. No gameplay code runs yet — these are pure data containers.

---

### Design Intent

The designer configures rooms through a layered asset system:

```
URoomData          ← one asset per room style (e.g. "Office", "Server Room")
  ├── UFloorData   ← which floor meshes to use and how to weight them
  ├── UCeilingData ← which ceiling meshes to use (same entry format as floor)
  ├── UWallData    ← the 4-piece wall stack meshes
  └── UDoorData    ← door mesh, frame, interaction type, sounds
```

Swapping one `UFloorData` asset on a `URoomData` changes the floor style for every room
that references that `URoomData`. Styles are composable — you could share a `UWallData`
between two room styles but give them different `UFloorData`.

---

### File Reference

| Class | Header | Notes |
|---|---|---|
| `FFloorMeshEntry` | `Public/FloorData.h` | Struct defining one mesh entry in a floor/ceiling set |
| `UFloorData` | `Public/FloorData.h` | Array of `FFloorMeshEntry` for floor generation |
| `UCeilingData` | `Public/CeilingData.h` | Array of `FFloorMeshEntry` for ceiling generation |
| `UWallData` | `Public/WallData.h` | 4-piece wall stack + permitted sizes + exterior flag |
| `EDoorInteraction` | `Public/DoorData.h` | `AutoTrigger` or `RequiresInput` |
| `UDoorData` | `Public/DoorData.h` | Door and frame meshes, collision extent, sounds |
| `ERoomGenerationType` | `Public/RoomData.h` | `Uniform` or `BSP` fill algorithm |
| `FForcedPlacement` | `Public/RoomData.h` | Override a specific cell with a specific mesh |
| `URoomData` | `Public/RoomData.h` | Root style asset — references all sub-assets |

---

### FFloorMeshEntry — Understanding CellsX / CellsY

Each entry in a `UFloorData` asset describes one mesh option. `CellsX` and `CellsY`
tell the floor fill algorithm how many grid cells this mesh occupies:

| CellsX | CellsY | World size |
|---|---|---|
| 1 | 1 | 100 × 100 cm |
| 2 | 2 | 200 × 200 cm |
| 4 | 4 | 400 × 400 cm |
| 2 | 4 | 200 × 400 cm |

`Weight` controls how often this mesh is chosen relative to others in the same asset.
`AllowRotation` permits the fill algorithm to rotate the mesh 90° when placing it —
useful for a `200×400` mesh that should also appear as `400×200`.

---

### UWallData — The 4-Piece Stack

Every wall cell in the grid receives a stack of 4 meshes placed at increasing Z offsets:

```
TopMesh       ← always present
MiddleMesh2   ← optional, used in taller rooms
MiddleMesh1   ← stacked on BaseMesh via socket
BaseMesh      ← always present, sits at floor level
```

`MiddleMesh1` has a socket that `MiddleMesh2` (if set) and `TopMesh` align to.
The number of middle pieces used depends on `AFloorManager::RoomHeightCm` — this
logic is implemented in Step 4.

All wall meshes must have their pivot set to **BottomBackCenter** in Blender before
import. This pivot convention allows the wall stack to be rotated 90° increments for
all four cardinal directions without any offset correction in code.

`WallSizes` lists which Y-axis widths this wall asset supports: `100`, `200`, or `400`.
The fill algorithm picks the largest wall segment that fits the remaining wall run.

`bIsExteriorCapable` signals to `ABuildingManager` that this wall type can be used
on the building's outer perimeter. Rooms whose perimeter walls face outside will have
their interior `WallISMC` instances suppressed and replaced by the exterior skin mesh.

---

### FForcedPlacement

`URoomData::ForcedPlacements` is an array of overrides applied after the base fill
(floor, ceiling, wall). Each entry targets a specific cell coordinate within the room's
local grid and places a specific mesh there, overriding whatever the fill algorithm chose.

Used by `AStaircaseRoom` to place the stair mesh at a fixed position within the room grid,
and by designers who want a specific prop at a guaranteed location.

---

### Include Chain

```
RoomData.h
  ├── FloorData.h     (for UFloorData*)
  │     └── Engine/StaticMesh.h
  ├── CeilingData.h   (for UCeilingData*, reuses FFloorMeshEntry from FloorData.h)
  ├── WallData.h      (for UWallData*)
  └── DoorData.h      (for UDoorData*)
        └── Sound/SoundBase.h
```

No circular includes. `CeilingData.h` includes `FloorData.h` to reuse `FFloorMeshEntry`
rather than duplicating the struct.

---

## 4. Step 2 — AMasterRoom Grid and Cell Classification

**Status**: Complete  
**CLAUDE.md reference**: Section 2 (AMasterRoom responsibilities), Section 5 (generation sequence steps 1–2)

Step 2 establishes `AMasterRoom` as the owning actor for one room's interior. After this
step, the class can allocate a cell grid, classify every cell, and draw a debug
visualization in the editor. No meshes are placed yet.

---

### File Reference

| File | Purpose |
|---|---|
| `Public/MasterRoom.h` | Class declaration — all fields, all 8 generation step signatures |
| `Private/MasterRoom.cpp` | `BuildGrid` and `ClassifyCells` implemented; steps 3–8 are stubs |

---

### How AMasterRoom Is Created

`AMasterRoom` is never placed in the level directly by a designer. It is always spawned
at runtime by `ABuildingManager::RequestRoomSpawn()` (Step 7), which is the sole actor
allowed to call `SpawnActor` for room classes. After spawn, `BuildingManager` calls:

```cpp
Room->InitializeRoom(Placement, GenerationSeed, FloorRoomHeightCm);
```

`InitializeRoom` stores those three inputs and immediately calls `GenerateRoomInterior()`,
which runs all 8 steps in sequence.

---

### Components

| Component | Purpose |
|---|---|
| `FloorISMCPool` | `TArray<UInstancedStaticMeshComponent*>` — one ISMC per unique static mesh encountered during floor generation; populated dynamically by `PlaceFloorMeshes` |
| `CeilingISMCPool` | `TArray<UInstancedStaticMeshComponent*>` — one ISMC per unique static mesh encountered during ceiling generation; populated dynamically by `PlaceCeilingMeshes` |
| `WallISMCPool` | `TArray<UInstancedStaticMeshComponent*>` — one ISMC per unique static mesh encountered during wall generation; populated dynamically by `PlaceWallMeshStacks` |
| `FlavorISMC` | Single Instanced Static Mesh for decoration meshes (stub until Step 12) |
| `UDebugLog` | Logging and editor visualization |

All four ISMCs are attached to a root `USceneComponent`. ISM geometry is **never
replicated** — clients receive the actor via normal replication, then regenerate
all geometry locally from `GenerationSeed`.

---

### The 8-Step Generation Sequence

`GenerateRoomInterior()` calls these steps in order. Each step is a `BlueprintNativeEvent`,
meaning it has a C++ default implementation but can be overridden in a Blueprint child class.
Subclasses call `Super` to run the base sequence, then add subclass-specific logic after.

| Step | Function | Status | Purpose |
|---|---|---|---|
| 1 | `BuildGrid()` | Implemented | Allocates `CellGrid` array, resolves `RoomGridSize` |
| 2 | `ClassifyCells()` | Implemented | Tags each cell as Floor, Wall, or Corner |
| 3 | `PlaceFloorMeshes()` | Implemented | Weighted random / Uniform fill, multi-mesh pool, AllowRotation (Step 3) |
| 4 | `PlaceCeilingMeshes()` | Implemented | Mirror of floor pass at `AMasterRoom::RoomHeightCm` Z offset (Step 3b) |
| 5 | `PlaceWallMeshStacks()` | Implemented | Bin-packed wall modules per face; corner pieces at corners; door cells skipped |
| 6 | `ApplyForcedPlacements()` | Stub | Override cells from URoomData::ForcedPlacements (Step 12) |
| 7 | `PlaceFlavorMeshes()` | Stub | Scatter flavor meshes at FlavorDensity (Step 12) |
| 8 | `SpawnDoorTriggers()` | Stub | Spawn replicated UBoxComponent triggers (Step 8) |

---

### Step 1 Detail — BuildGrid()

`BuildGrid_Implementation()` resolves the room's grid dimensions then allocates the cell array.

**Size resolution order**:
1. If `ActivePlacement.SizeOverride != (0,0)` — use the override directly.
2. Otherwise, use the `FRandomStream` seeded from `GenerationSeed` to pick a random size
   between `RoomData->MinRoomSize` and `RoomData->MaxRoomSize`.

The stream is seeded from `GenerationSeed` so every call with the same seed produces the
same size — this is what allows clients to regenerate identical geometry without network traffic.

**Cell array layout**: `CellGrid` is a flat `TArray<ECellType>` in row-major order.
To convert a 2D grid coordinate to an array index:

```cpp
int32 Index = Y * RoomGridSize.X + X;
// or use the helper:
int32 Index = CellIndex(X, Y);
```

After `BuildGrid()`, every cell is initialized to `ECellType::Floor`. `ClassifyCells()`
overwrites the edges in the next step.

---

### Step 2 Detail — ClassifyCells()

A single nested loop visits every cell. Classification rules (in priority order):

1. **Corner**: `X == 0 || X == MaxX` AND `Y == 0 || Y == MaxY` → `ECellType::Corner`
2. **Wall**: `X == 0 || X == MaxX` OR `Y == 0 || Y == MaxY` (but not both) → `ECellType::Wall`
3. **Floor**: everything else → `ECellType::Floor`

This means a 5×4 room produces:

```
C W W W C
W F F F W
W F F F W
C W W W C
```
(C = Corner, W = Wall, F = Floor)

For a minimum-size room of 2×2, every cell is a Corner — there are no Wall or Floor cells.
The smallest room that has interior floor cells is 3×3.

---

### IsDoorCell() Helper

`PlaceWallMeshStacks()` (Step 4) needs to know whether a given Wall cell has a door on it.
`IsDoorCell(X, Y)` checks the current cell against every `FDoorPlacement` in
`ActivePlacement.Doors`:

- `Face::North` → matches cells on the `Y == MaxY` edge at `X == CellOffset`
- `Face::South` → matches cells on the `Y == 0` edge at `X == CellOffset`
- `Face::East`  → matches cells on the `X == MaxX` edge at `Y == CellOffset`
- `Face::West`  → matches cells on the `X == 0` edge at `Y == CellOffset`

`CellOffset` is 0-based from the leftmost cell of that face as seen from inside the room.

---

### Editor Debug Visualization

After `GenerateRoomInterior()` finishes, `DrawDebugGrid()` is called inside `#if WITH_EDITOR`.
It passes `RoomGridSize`, `CellGrid`, `CellSize = 100.f`, and `GetActorLocation()` to
`Visualizer->DrawRoomGrid()`.

What appears in the viewport is controlled entirely by the toggles on the `Visualizer`
component in the Details panel. No recompile is needed to toggle visualization on or off —
set the properties and call `GenerateLayout` again from `AFloorManager`.

To see the grid in the editor:
1. Place an `AMasterRoom` (or let `AFloorManager::GenerateLayout` spawn one).
2. Select the actor, find the `Visualizer` component in the Details panel.
3. Enable `bShowGrid`, `bShowCellStates`, and/or `bShowCoordinates`.
4. Re-trigger generation.

---

### Replication Reminder

`AMasterRoom` has `bReplicates = true`. The actor itself replicates to clients.
`CellGrid` and all ISMC geometry are **not** replicated properties. When a late-joining
client receives the actor, it calls `InitializeRoom()` locally with the same
`GenerationSeed` it receives from `ABuildingManager` and regenerates the geometry
identically. This is load-bearing for the multiplayer design — never add `Replicated`
to `CellGrid` or any ISMC data.

---

## 5. Step 3 — Weighted Random Floor Fill

**Status**: Complete  
**CLAUDE.md reference**: Section 5 (generation step 3), Section 6 (Uniform algorithm), Section 8 (pivot conventions)

Step 3 fills every `ECellType::Floor` cell with an instanced static mesh drawn from a
weighted random pool. Multiple mesh sizes, weights, and rotations are all supported.
After this step every floor cell is covered and the room is fully tiled.

---

### FloorISMCPool Architecture

The original single `FloorISMC` component was replaced with `FloorISMCPool:
TArray<UInstancedStaticMeshComponent*>` — one ISMC per unique static mesh encountered
during generation. This is the same pattern used for `WallISMCPool` (Step 4).

`GetOrCreateFloorISMC(UStaticMesh* Mesh)` manages the pool:
- Linear scan for an existing pool entry whose `GetStaticMesh() == Mesh`
- If none found: creates a new component via `NewObject<>`, attaches to the actor root
  with `KeepRelativeTransform`, calls `RegisterComponent()`, and appends to `FloorISMCPool`

Neither `FloorISMCPool` nor `WallISMCPool` has a `CreateDefaultSubobject` entry in the
constructor — both are populated entirely at generation time.

**ClearPreview / PreviewRoom**: iterates all pool entries and calls `ClearInstances()`.
Components are not destroyed between previews — just cleared. On the next generation run
`GetOrCreateFloorISMC` finds the existing entries and reuses them.

---

### Per-Step Seed Isolation

Every generation step has its own `FRandomStream`, seeded by XOR-ing `GenerationSeed`
with a step-specific offset constant defined in `MasterRoom.h`:

```cpp
static constexpr int32 SeedOffset_BuildGrid   = 0x1000;
static constexpr int32 SeedOffset_FloorFill   = 0x2000;
static constexpr int32 SeedOffset_CeilingFill = 0x3000;
static constexpr int32 SeedOffset_WallFill    = 0x4000;
static constexpr int32 SeedOffset_FlavorFill  = 0x5000;
```

This guarantees that each step's stream is independent regardless of how many random
values the previous step consumed. Two steps sharing a stream would produce different
results if an earlier step's logic changed — the XOR offset eliminates that coupling.

---

### EFloorFillMode — Random vs Uniform

`UFloorData` and `UCeilingData` each expose a `FillMode` field of type `EFloorFillMode`
(defined in `FloorData.h`, reused by `CeilingData.h` via its include):

| Mode | Behavior |
|---|---|
| `Random` (default) | Per-cell pool-based exhaustive search — each cell draws independently from the full mesh array |
| `Uniform` | One mesh is selected from the weighted pool using the step's `FRandomStream` before the fill loop starts. That single entry is used for every cell — different seeds produce different uniform tiles |

**Uniform pre-selection logic** (runs once, before the cell loop):

```cpp
float TotalWeight = ...; // sum of valid entry weights
float Pick = Stream.FRand() * TotalWeight;
// scan entries, subtract weights, stop at first entry where Pick <= 0
// floating-point rounding fallback: take first valid entry if loop overshoots
```

Consuming one `FRand()` from the stream before the cell loop means that any seed change
affects both which mesh is chosen *and* how subsequent `AllowRotation` coin flips fall —
the entire room changes together, as expected for a seeded generator.

**In the cell loop**, Uniform mode builds a single-entry pool with the pre-selected mesh.
All existing orientation, rotation, and fallback logic runs unchanged — only the pool
construction differs. If the uniform mesh cannot fit at a cell, the `FallbackEntry` (1×1)
is used, then the cell is skipped if no fallback exists.

**Weights in Uniform mode**: higher weight = more likely to be the room's chosen tile.
A single entry with no other competitors is always chosen regardless of weight.

**Output Log**: in Uniform mode, `PlaceFloorMeshes` and `PlaceCeilingMeshes` emit
`[MESH] Uniform mode — mesh: <MeshName>` at `LogImportant` before the first cell is placed.

---

### The Fill Algorithm — Pool-Based Exhaustive Search

Scan order: Y outer (top-to-bottom), X inner (left-to-right). For each unclaimed Floor cell:

1. Build a per-cell candidate pool: `TArray<TPair<const FFloorMeshEntry*, float>>` from all
   entries with a valid mesh. Entries with `Weight <= 0` are assigned weight `1.0`. If the
   pool is empty (no valid meshes), generation bails with `LogCritical`.
2. While the pool is non-empty and no mesh has been placed:
   - Draw a weighted-random candidate via accumulated-weight scan, then `RemoveAtSwap` it
     from the pool so it cannot be selected again.
   - Attempt placement based on shape:

**Non-square mesh (CellsX ≠ CellsY):**
- Both orientations are **always** attempted, regardless of `AllowRotation`.
- `AllowRotation = true` randomizes which orientation is tried first (50/50 coin flip via
  `Stream.FRand()`). `AllowRotation = false` always tries the natural `CellsX×CellsY` first.
- If the first orientation fits → place and break. If not → try the swapped `CellsY×CellsX`.
- If neither fits → continue to the next pool candidate.

**Square mesh (CellsX == CellsY), `AllowRotation = true`:**
- Picks a random aesthetic yaw from {0, 90, 180, 270}° (`Stream.RandRange(0,3) * 90`).
- Footprint is unchanged; the rotation is purely visual.
- If `CanPlaceMesh` fails → continue to next pool candidate.

**Square mesh (CellsX == CellsY), `AllowRotation = false`:**
- Always placed at 0°.
- If `CanPlaceMesh` fails → continue to next pool candidate.

3. If the pool is exhausted with no placement, fall back to `FallbackEntry` (see below).

This exhaustive approach means a room can be fully covered with only 1×2 and 2×2 meshes —
no 1×1 required — as long as all cells can be tiled in some orientation. The 1×1 is only
reached if every pool candidate genuinely cannot fit at the current cell.

---

### FallbackEntry — Recommended 1×1 Entry

Before the main loop, the function scans `FloorData->FloorMeshes` for the first entry
with `CellsX == 1 && CellsY == 1`. This is stored as `FallbackEntry`.

The fallback path must use a 1×1 mesh. Placing a multi-cell mesh (e.g. 1×2) at a 1×1
transform puts the mesh center in the middle of a single cell, but the geometry extends
beyond it — a 200 cm mesh centered in a 100 cm cell bleeds 50 cm into each adjacent cell.
See `IssueTracker.md` Issue 6 for the full root-cause analysis.

If no 1×1 entry exists and a cell is truly unfillable by all pool candidates, `LogImportant`
fires and the cell is **skipped entirely** — no mesh is placed. This is not fatal; it
produces a visible gap only for cells that no pool mesh can cover in any orientation.
**Designer guidance**: include a 1×1 entry in any asset to guarantee complete coverage
without relying on the exhaustive pool alone. A `SkippedCount` statistic is logged at
the end of each fill pass when cells were skipped.

---

### Helper Functions

| Helper | Location | Purpose |
|---|---|---|
| `GetOrCreateFloorISMC(Mesh)` | AMasterRoom private | Pool lookup or new ISMC creation |
| `CanPlaceMesh(X, Y, CellsX, CellsY, Claimed)` | AMasterRoom private | Bounds + cell type + claim check |
| `ClaimCells(X, Y, CellsX, CellsY, Claimed)` | AMasterRoom private | Mark footprint cells as occupied |
| `MakeCellTransform(X, Y, CellsX, CellsY, Yaw)` | AMasterRoom private | Component-local FTransform (BottomCenter pivot) |

#### `CanPlaceMesh(X, Y, CellsX, CellsY, Claimed)`

Returns `true` if the entire `CellsX × CellsY` footprint fits at `(X, Y)`:
- Bounds: `X + CellsX <= RoomGridSize.X` AND `Y + CellsY <= RoomGridSize.Y`
- Every covered cell is `ECellType::Floor`
- Every covered cell is unclaimed

#### `MakeCellTransform(X, Y, CellsX, CellsY, Yaw)`

Returns a component-local `FTransform`. The position is the center of the mesh footprint:

```cpp
LocalPos.X = (X + CellsX * 0.5f) * 100.f
LocalPos.Y = (Y + CellsY * 0.5f) * 100.f
LocalPos.Z = 0.f
```

`ISMC::AddInstance()` defaults to `bWorldSpace = false`, so no actor-offset correction
is needed. The pivot convention for floor meshes is **BottomCenter** — the mesh's pivot
must be set in Blender at the bottom-center of the tile before import.

---

## 6. Step 3b — Ceiling Mesh Fill

**Status**: Complete  
**CLAUDE.md reference**: Section 5 (generation step 4), Section 3 (UCeilingData), Section 8 (pivot conventions)

Ceiling fill runs immediately after floor fill (generation sequence step 4). The algorithm
is identical to floor fill — the only differences are the data source (`UCeilingData` instead
of `UFloorData`) and the Z offset applied to every placed mesh.

---

### CeilingISMCPool Architecture

Mirrors `FloorISMCPool` exactly. `GetOrCreateCeilingISMC(UStaticMesh* Mesh)` follows the
same lookup-or-create pattern — one ISMC per unique ceiling mesh, populated dynamically
during `PlaceCeilingMeshes_Implementation`.

The ceiling fill uses the same pool-based exhaustive algorithm as floor fill, including
`EFloorFillMode` support. `GetOrCreateCeilingISMC` follows the same lookup-or-create
pattern as `GetOrCreateFloorISMC`. See Section 5 (Step 3) for the full algorithm and
`EFloorFillMode` semantics — they apply identically to `UCeilingData::FillMode`.

---

### Ceiling Z Offset

`AMasterRoom::RoomHeightCm` is the authoritative source for the ceiling Z position.
This value is passed in by `AFloorManager` via `InitializeRoom()` — all rooms on a floor
share the same height. The generated ceiling meshes are placed at exactly this Z value
in component-local space.

```cpp
const float CeilingZ = static_cast<float>(RoomHeightCm);
```

`UCeilingData` does **not** have a `RoomHeightCm` field — a per-asset ceiling height was
considered but removed because it would contradict the per-floor authority defined in
CLAUDE.md Section 12. Ceiling height is a floor-level property, not a room-style property.

**Editor preview**: `AMasterRoom` exposes `PreviewRoomHeightCm` (`WITH_EDITORONLY_DATA`,
category `"MasterRoom|Debug"`) as the substitute for `AFloorManager::RoomHeightCm` during
`PreviewRoom()`. Set this to match the visual top of your wall stack (Base + Middle + Top
mesh heights combined).

> **Designer note**: `PreviewRoomHeightCm` only affects the in-editor preview. At runtime,
> the value is always supplied by `AFloorManager::RoomHeightCm` via `InitializeRoom()`.
> If walls and ceiling appear misaligned in Play, check `AFloorManager::RoomHeightCm` —
> not the preview field.

---

### FallbackEntry — Same Rule as Floor

`PlaceCeilingMeshes_Implementation` uses the same `FallbackEntry` precompute pattern.
If no 1×1 entry exists and the exhaustive pool cannot cover a cell, that cell is **skipped**
(no mesh placed) and a `LogImportant` warning fires. See `IssueTracker.md` Issue 6 for the
original root-cause analysis and Issue 8 for the AllowRotation semantics fix that eliminated
the most common class of unfillable cells.

---

### Key Difference from Floor

The ceiling only covers `ECellType::Floor` cells — the same interior cells as the floor
pass. Wall and Corner cells do not receive ceiling meshes (the wall stack and corner mesh
form the border at ceiling height).

---

## 7. Step 4 — Wall Mesh Stack Placement

**Status**: Complete  
**CLAUDE.md reference**: Section 2 (AMasterRoom), Section 3 (UWallData / FWallModule), Section 5 (generation step 5), Section 8 (pivot conventions)

Step 4 fills every `ECellType::Wall` cell with a stack of 1–4 mesh pieces drawn from
`UWallData::WallModules`. After this step, all room walls are visible in the editor and
door cells are correctly left empty for future door mesh placement.

---

### Pre-requisite Files Created This Step

#### `BuildingMathUtils.h`

**File**: `Public/BuildingMathUtils.h`

Plain C++ utility struct `FBuildingMath`. No UHT reflection — no `.generated.h`.

| Method | Returns | Purpose |
|---|---|---|
| `ToIntPoint(ERoomFace)` | `FIntPoint` | Unit step in grid space for the face direction |
| `ToYawDegrees(ERoomFace)` | `float` | Yaw (degrees) to orient a BottomBackCenter wall mesh outward |
| `Opposite(ERoomFace)` | `ERoomFace` | The face on the other side of a shared wall boundary |

`ToYawDegrees` convention: 0° has the back face pointing West (-X). Each face rotates 90° CCW:

| Face | Yaw |
|---|---|
| West | 0° |
| South | 90° |
| East | 180° |
| North | 270° |

This is what makes BottomBackCenter-pivoted meshes orient correctly on all four faces with
no position offset — only a yaw rotation is needed.

#### `RoomPlacement.h` (created early — compile fix)

**File**: `Public/RoomPlacement.h`

Created during Step 3 compilation to resolve missing type errors (`FRoomPlacement`,
`FDoorPlacement`, `ERoomFace`). See `IssueTracker.md` Issue 1 for full context. Though
scheduled for Step 5, these types are prerequisites for any code that handles rooms.

Declares: `ERoomFace`, `FDoorPlacement`, `FRoomPlacement`.

`AMasterRoom` is forward-declared (not included) to avoid a circular dependency.

---

### Wall Generation Algorithm

`PlaceWallMeshStacks_Implementation()` iterates all four faces in order
(North → South → East → West). For each face it scans from `PosMin = 1` to
`PosMax = FaceLen - 2`, skipping the corner cells at positions 0 and `FaceLen-1`.

At each position:

1. **Door check** — if `IsDoorCell(X, Y)` is true, advance one cell and increment `DoorCellsSkipped`.
2. **Count available run** — scan forward from `Pos` to `PosMax`, stopping at the next door cell.
   This is the maximum module width that can fit without overlapping a door.
3. **Pick a module** — call `PickWallModule(WallData, Available, Stream)` for a weighted random
   selection filtered to modules whose `CellsWide <= Available`.
4. **Place the stack** — call `PlaceLayer` for each mesh piece (Base, Middle1, Middle2, Top).
   Each piece is placed via `GetOrCreateWallISMC(Mesh)->AddInstance(MakeWallTransform(...))`.
5. **Advance** — `Pos += Mod->CellsWide`. A 2-wide module consumes two cells in one step.

```
while (Pos <= PosMax)
  if IsDoorCell(Pos) → skip 1, ++DoorCellsSkipped
  else
    Available = run length to next door or PosMax
    Mod = PickWallModule(WallData, Available, Stream)
    PlaceLayer(BaseMesh,    Z = 0)
    PlaceLayer(MiddleMesh1, Z = BaseH)
    PlaceLayer(MiddleMesh2, Z = BaseH + M1H)
    PlaceLayer(TopMesh,     Z = BaseH + M1H + M2H)
    Pos += Mod->CellsWide
```

---

### WallISMCPool Architecture

Fixed per-layer ISMCs (`WallBaseISMC`, `WallTopISMC`, etc.) were rejected because
different modules have different static meshes per layer — a fixed ISMC can only hold
one mesh. Instead `WallISMCPool` is a `TArray<UInstancedStaticMeshComponent*>` where
each entry holds exactly one static mesh. `GetOrCreateWallISMC(Mesh)` does a linear scan
for an existing pool entry matching `Mesh`, and creates a new attached component if none exists.

This means the pool is empty before the first `PlaceWallMeshStacks` call and grows
dynamically as new unique meshes are encountered. The pool is visible in the Components
panel of the Details pane as separate ISMC entries after generation.

**Important**: `WallISMCPool` is **not** cleared between regenerations. `ClearPreview()`
calls `ISMC->ClearInstances()` on every pool member but does not destroy the components.
Components are only created once; instances are reset.

---

### Stack Height and PlacementOffset

**Stack height**: Each layer starts at Z = sum of all previous layers' heights. The height of
a layer is determined by `BaseMeshStackHeight` on the module (if > 0), otherwise by
`Mesh->GetBoundingBox().GetSize().Z`. Designers set the override when the bounding box
height differs from the visual top of the mesh (e.g., a mesh with embedded detail that
bloats the BB above the visible wall top).

**PlacementOffset**: In wall-forward space (X = into room, Y = along face, Z = up).
Corrects for mesh depth ≠ 100 cm. If a mesh is 135 cm deep, set `PlacementOffset.X = -35`
to pull it flush. `MakeWallTransform` rotates this local offset by the face yaw before
applying it, so the same module entry works on all four faces.

---

### Corner Placement

After the four-face wall loop, `PlaceWallMeshStacks_Implementation` checks `WallData->CornerMesh`.
If non-null, it places one instance at each of the four `ECellType::Corner` cells using
`MakeCellTransform(X, Y, 1, 1, Yaw)` — the same helper as floor tiles.

**Yaw convention** (mesh at 0° has its interior-facing corner pointing NE):

| Corner | Grid pos | Yaw |
|---|---|---|
| SW | (0, 0) | 0° |
| SE | (MaxX, 0) | 90° |
| NE | (MaxX, MaxY) | 180° |
| NW | (0, MaxY) | 270° |

Corner instances go into `WallISMCPool` via `GetOrCreateWallISMC(WallData->CornerMesh)` —
the same pool as wall modules. If `CornerMesh` is null the pass is silently skipped; no
warning is emitted since no-corner is a valid design choice.

**Output Log**: `[WALL] Corner pieces placed: 4` (or 0 if CornerMesh is null).

---

### Key Private Helpers (MasterRoom.cpp)

| Helper | Where | Purpose |
|---|---|---|
| `FaceToCell(Face, Pos, W, H, OutX, OutY)` | file-static | Converts face + position index to grid (X, Y) |
| `PickWallModule(WallData, Available, Stream)` | file-static | Weighted random module selection, filtered by Available width |
| `GetOrCreateWallISMC(Mesh)` | AMasterRoom private | Pool lookup or new ISMC creation (used by both wall modules and corner mesh) |
| `MakeWallTransform(FacePos, Face, CellsWide, Z, Offset)` | AMasterRoom protected | Component-local FTransform for a BottomBackCenter wall piece |
| `MakeCellTransform(X, Y, CellsX, CellsY, Yaw)` | AMasterRoom private | Component-local FTransform for BottomCenter floor/corner tiles |
| `GetFaceForWallCell(X, Y)` | AMasterRoom protected | Returns the face a Wall cell sits on (used by Step 11) |

---

## 8. Step 5 — AFloorManager Details Panel

**Status**: Complete  
**CLAUDE.md reference**: Section 2 (AFloorManager), Section 4 (FRoomPlacement, FDoorPlacement), Section 9 (WITH_EDITOR conventions)

Step 5 creates `AFloorManager` — the actor that holds a floor's room layout and drives
spawning for that floor. After this step, designers can set up `RoomPlacements` in the
Details panel and trigger layout operations from editor buttons.

---

### File Reference

| File | Purpose |
|---|---|
| `Public/FloorManager.h` | Class declaration — placements, editor buttons, visualizer, dev log |
| `Private/FloorManager.cpp` | Constructor, editor button stubs, ClearLayout implementation |
| `Public/RoomPlacement.h` | `ERoomFace`, `FDoorPlacement`, `FRoomPlacement` struct declarations |

`RoomPlacement.h` was factored out earlier (Step 3 compile fix) but formally belongs to
Step 5 in the generation roadmap.

---

### Key Properties

| Property | Type | Default | Purpose |
|---|---|---|---|
| `RoomPlacements` | `TArray<FRoomPlacement>` | empty | Designer-authored layout — one entry per room |
| `RoomHeightCm` | `int32` | 300 | All rooms on this floor share this wall/ceiling height |
| `FloorGridSize` | `FIntPoint` | (20, 20) | Total cell area for this floor |
| `FloorIndex` | `int32` | 0 | Floor number; drives Z offset during spawning |
| `OwningBuildingManager` | `ABuildingManager*` | nullptr | Must be set before `GenerateLayout` |
| `SpawnedRooms` | `TArray<AMasterRoom*>` | — | Runtime-only, tracks spawned actors for `ClearLayout` |
| `DevLog` | `UBGDevLog*` | — | Logging component, always present |
| `Visualizer` | `UBGVisualizer*` | — | Editor visualization, `WITH_EDITORONLY_DATA` |

---

### Editor Buttons

| Button | Implemented in | Behavior |
|---|---|---|
| `PreviewLayout` | Step 6 | Run validation, draw floor grid and room footprints |
| `GenerateLayout` | Step 7 | Spawn all rooms via `ABuildingManager::RequestRoomSpawn` |
| `ClearLayout` | Step 5 | Destroy all `SpawnedRooms`, empty the array |
| `ClearPreview` | Step 5 | Call `Visualizer->ClearDebugDrawings()` |

`ClearLayout` was the one fully implemented action in Step 5. The other three buttons
were stubs that received their full implementations in Steps 6 and 7.

---

### Visualizer Constructor Configuration

The `AFloorManager` constructor configures `Visualizer` with floor-appropriate defaults:

```cpp
Visualizer->GridColor         = FColor(80, 80, 80);   // thin gray lines
Visualizer->GridLineThickness = 1.f;
Visualizer->bShowGrid         = true;
Visualizer->bShowCoordinates  = true;
Visualizer->OnCreateTextComponent.BindUObject(this, &AFloorManager::CreateDebugTextComponent);
Visualizer->OnDestroyTextComponent.BindUObject(this, &AFloorManager::DestroyDebugTextComponent);
Visualizer->OnRedrawRequested.BindUObject(this, &AFloorManager::PreviewLayout);
```

Binding `OnRedrawRequested` to `PreviewLayout` means that any Visualizer toggle button
(Grid, Coordinates, CellStates) automatically fires `PreviewLayout` and the viewport
updates immediately — no manual refresh needed.

---

## 9. Step 6 — PreviewLayout Debug Visualization

**Status**: Complete  
**CLAUDE.md reference**: Section 9 (WITH_EDITOR conventions, PreviewLayout validation checks)

Step 6 implements `AFloorManager::PreviewLayout()` in full — the editor button that draws
the floor grid, room footprints, and runs five layout validation checks. No actors are
spawned; this is a pure visualization pass.

---

### What PreviewLayout Draws

| Draw element | When shown |
|---|---|
| Boundary box (outer perimeter of floor grid) | `Visualizer->bShowGrid` |
| Interior grid lines (cell subdivisions) | `Visualizer->bShowGrid` |
| Coordinate labels every 5 cells | `Visualizer->bShowCoordinates` |
| Green box per valid room footprint | Always |
| Red box per invalid room footprint | When the room fails any check |
| Room index + size label per footprint | Always |

All draws are **persistent** (`Duration = -1`). `ClearPreview` calls `Visualizer->ClearDebugDrawings()`
which flushes `FlushPersistentDebugLines()` and `FlushDebugStrings()` globally. `PreviewLayout`
calls `ClearPreview()` at its start to avoid stacking.

---

### Validation Checks

Five checks run each time `PreviewLayout` is called. Any failure colors the offending
room(s) red and logs a `LogCritical` message.

| Check | Failure condition |
|---|---|
| **Out of bounds** | Any footprint cell extends beyond `FloorGridSize` |
| **Room overlap** | Two footprint AABB intersect |
| **Orphan door** | Door face has no adjacent room and `bIsExteriorDoor = false` |
| **Exterior door on non-perimeter face** | `bIsExteriorDoor = true` but the face does not sit on the floor grid boundary |
| **Misaligned shared doors** | Two adjacent rooms have doors on the shared face but `WorldStart` or `Width` differ |

Staircase landing check (CLAUDE.md Section 9) is deferred to Step 10 when `AStaircaseRoom`
and `ConnectsToFloor` exist.

---

### Footprint Resolution

Before drawing or checking, every `FRoomPlacement` is resolved to a concrete `FIntPoint Size`:

1. If `SizeOverride != (0,0)` — use the override.
2. Else if `RoomDataAsset` is valid — use `RoomDataAsset->MinRoomSize`.
3. Else — fall back to (1, 1).

If `RotationDegrees` is 90° or 270°, X and Y are swapped after resolution.

---

### Door Width Resolution in PreviewLayout

`PreviewLayout` needs door widths for the misalignment check. It uses a local lambda
that mirrors `AMasterRoom::GetDoorWidthCells`:

```cpp
auto GetDoorWidthCells = [](const FRoomPlacement& P, const FDoorPlacement& Door) -> int32
{
    const UDoorData* DD = IsValid(Door.DoorDataOverride)
        ? Door.DoorDataOverride.Get()
        : (IsValid(P.RoomDataAsset) && IsValid(P.RoomDataAsset->DoorData)
           ? P.RoomDataAsset->DoorData.Get() : nullptr);
    return (IsValid(DD) && DD->DoorWidth == EDoorWidth::FourCell) ? 4 : 2;
};
```

---

## 10. Step 7 — ABuildingManager Spawning

**Status**: Complete  
**CLAUDE.md reference**: Section 2 (ABuildingManager, AFloorManager), Section 3 (UBuildingData / FFloorDefinition), Section 7 (SpawnActor pattern), Section 7 (replication rules)

Step 7 implements `ABuildingManager` as the root actor and sole authority for spawning both
`AFloorManager` and `AMasterRoom` instances. The entire building layout is now driven from
a single `UBuildingData` DataAsset assigned in the Details panel — no level-placed
`AFloorManager` actors are needed.

---

### File Reference

| File | Purpose |
|---|---|
| `Public/BuildingManager.h` | Class declaration — BuildingDataAsset, SpawnedFloorManagers, GenerationSeed, editor buttons |
| `Private/BuildingManager.cpp` | SpawnFloorManagersFromAsset, RequestRoomSpawn, GetLifetimeReplicatedProps, three editor buttons |
| `Public/BuildingData.h` | UBuildingData DataAsset + FFloorDefinition struct |
| `Private/BuildingData.cpp` | (generated boilerplate only) |

---

### Key Properties

| Property | Type | Set by | Notes |
|---|---|---|---|
| `BuildingDataAsset` | `UBuildingData*` | Designer | Single DataAsset that defines all floors and rooms |
| `GenerationSeed` | `int32` | Designer / Code | `UPROPERTY(Replicated)` — all geometry derives from this |
| `SpawnedFloorManagers` | `TArray<AFloorManager*>` | Runtime | Populated by `SpawnFloorManagersFromAsset()`, read-only |

`FloorManagers` (a level-actor reference array) has been **removed**. Designers no longer
place `AFloorManager` actors in the level or drag references into a Details panel array.

---

### UBuildingData and FFloorDefinition

`UBuildingData` is a `UDataAsset` subclass. One asset = one building type. It holds a
`TArray<FFloorDefinition> Floors` — one entry per floor.

`FFloorDefinition` fields:

| Field | Type | Notes |
|---|---|---|
| `FloorClass` | `TSubclassOf<AFloorManager>` | Which subclass to spawn; null = base AFloorManager |
| `RoomHeightCm` | `int32` | Height of all rooms on this floor (cm) |
| `FloorGridSize` | `FIntPoint` | Bounding cell area; PreviewLayout rejects rooms outside this |
| `RoomPlacements` | `TArray<FRoomPlacement>` | All rooms on this floor — same struct AFloorManager uses directly |

`BuildingData.h` includes `FloorManager.h` for `TSubclassOf<AFloorManager>`. `FloorManager.h`
does **not** include `BuildingData.h` — no circular dependency.

---

### SpawnFloorManagersFromAsset

`SpawnFloorManagersFromAsset()` is a `private` non-editor helper. Both `GenerateBuilding`
and `SpawnBuilding` call it first.

**Algorithm:**
1. Validate `BuildingDataAsset` is set and `Floors` is non-empty. Return `false` if not.
2. Destroy all existing `SpawnedFloorManagers` — calls `ClearLayout()` then `Destroy()` on each.
3. For each `FFloorDefinition` entry:
   - Resolve `FloorClass`: use `Def.FloorClass` if set, otherwise fall back to
     `TSubclassOf<AFloorManager>(AFloorManager::StaticClass())`.
   - Spawn actor at `BuildingOrigin + FVector(0, 0, CumulativeHeight)`.
   - Push `FloorIndex`, `RoomHeightCm`, `FloorGridSize`, `RoomPlacements` directly onto the
     spawned actor — no DataAsset reference kept on the floor manager.
   - Accumulate: `CumulativeHeight += Def.RoomHeightCm`.
4. Return `SpawnedFloorManagers.Num() > 0`.

**Cumulative Z**: floor N sits at the sum of `RoomHeightCm` for floors 0…N-1, not at
`FloorIndex * singleHeight`. This supports mixed-height floors (e.g. a taller ground floor).

**TSubclassOf ternary gotcha**: `TSubclassOf<T>` and `UClass*` are mutually convertible,
making a plain ternary expression ambiguous (C2445). The fallback branch must be explicitly
cast: `TSubclassOf<AFloorManager>(AFloorManager::StaticClass())`.

---

### AFloorManager Generation Paths

`OwningBuildingManager` (a UPROPERTY back-reference) has been **removed** from `AFloorManager`.
It was replaced by two generation paths:

| Method | When called | BuildingManager needed? |
|---|---|---|
| `GenerateLayoutWith(ABuildingManager*)` | By `ABuildingManager::SpawnBuilding` at runtime | Yes — BM passed as argument |
| `GenerateLayout()` (CallInEditor) | Editor button on AFloorManager directly | No — spawns with seed 0 |

`GenerateLayoutWith` is not editor-only — it is the server-side entry point for Step 8+
multiplayer generation. `ClearLayout()` is also not editor-only (`BlueprintCallable +
CallInEditor`) so it can be called from `SpawnFloorManagersFromAsset` and at runtime.

---

### RequestRoomSpawn

The only legal path for spawning an `AMasterRoom`. See the exact pattern in CLAUDE.md
Section 7. Key rules enforced here:

- `HasAuthority()` guard — returns `nullptr` immediately on clients
- `Params.SpawnCollisionHandlingOverride = AlwaysSpawn`
- `Params.Owner = this`
- After spawn: `Room->InitializeRoom(Placement, GenerationSeed, FloorRoomHeightCm)`

`FloorRoomHeightCm` is passed from `AFloorManager::GenerateLayoutWith` so `AMasterRoom`
receives the correct floor-specific height without querying upward.

---

### Replication

`ABuildingManager::GetLifetimeReplicatedProps` registers `GenerationSeed` via `DOREPLIFETIME`.
Clients receive the seed when they receive the actor. Late-joining clients receive the seed
and regenerate identical geometry locally from it — no mesh data travels over the network.

---

### Editor Buttons

| Button | Behavior |
|---|---|
| `GenerateBuilding` | `SpawnFloorManagersFromAsset()` + `PreviewLayout()` on each floor — debug grid only, no geometry |
| `SpawnBuilding` | `SpawnFloorManagersFromAsset()` + `GenerateLayoutWith(this)` on each floor — full geometry |
| `ClearBuilding` | `ClearLayout()` + `ClearPreview()` + `Destroy()` on each SpawnedFloorManager |

`GenerateBuilding` shows all floor grids simultaneously. Because debug lines have no depth
occlusion in the editor viewport, every floor's grid is visible at once regardless of Z offset —
this is the intended workflow for planning room layouts across multiple floors.

All three buttons are `#if WITH_EDITOR` and `CallInEditor`. In a packaged build they do not compile.

---

### Designer Workflow (Step 7)

1. Create a `UBuildingData` asset in the Content Browser.
2. Add one `FFloorDefinition` entry per floor. For each entry:
   - Set `FloorClass` (or leave null for base AFloorManager).
   - Set `RoomHeightCm` and `FloorGridSize`.
   - Add `FRoomPlacement` entries to `RoomPlacements` — same process as configuring a
     standalone `AFloorManager` in the Details panel.
3. Place one `ABuildingManager` actor in the level.
4. Assign the `UBuildingData` asset to `BuildingDataAsset` in the Details panel.
5. Click **GenerateBuilding** to see all floor grids simultaneously and plan room placements.
6. Click **SpawnBuilding** to generate full room geometry.
7. Click **ClearBuilding** to reset everything.

---

## 11. System Flow Diagram

```
[Designer creates UBuildingData asset]
  └── FFloorDefinition[] (FloorClass, RoomHeightCm, FloorGridSize, RoomPlacements[])
         │
         ▼
[Designer places ABuildingManager in world]
  └── Assigns BuildingDataAsset (UBuildingData*)
         │
         ├─ GenerateBuilding (editor)
         │    SpawnFloorManagersFromAsset()
         │    └── PreviewLayout() on each floor  ← debug grid only, no geometry
         │
         └─ SpawnBuilding (editor) / server at runtime
              SpawnFloorManagersFromAsset()
              └── GenerateLayoutWith(this) on each AFloorManager
                       │
                       ▼  HasAuthority() only
              [ABuildingManager::RequestRoomSpawn()]  ← Step 7
                       │
                       ▼
              [AMasterRoom spawned at world position]
                       │
                       ▼
              [InitializeRoom(Placement, Seed, HeightCm)]
                       │
                       ├─ BuildGrid()             ← Step 2
                       ├─ ClassifyCells()         ← Step 2
                       ├─ PlaceFloorMeshes()      ← Step 3
                       ├─ PlaceCeilingMeshes()    ← Step 3b
                       ├─ PlaceWallMeshStacks()   ← Step 4
                       ├─ ApplyForcedPlacements() ← Step 12
                       ├─ PlaceFlavorMeshes()     ← Step 12
                       └─ SpawnDoorTriggers()     ← Step 8
                                │
                                ▼
                       [ISMC geometry visible]
                       [Door triggers replicated]

[Client receives AMasterRoom via replication]
         │
         ▼
[Client calls InitializeRoom() locally]
         └─ Identical geometry regenerated from same seed (no mesh data over network)
```

---

## 12. Pending Investigations

This section records experiments, observations, and deferred feature work that emerged
during testing but were not implemented at the time. Review before starting any related system.

---

### 12.1 Hallway Rooms via MasterRoom SizeOverride

**Observation (Step 4 testing):** A `MasterRoom` with `PreviewPlacement.SizeOverride = (18, 6)`
generates a convincing narrow corridor — full floor, ceiling, and wall coverage with proper
corner and wall modules on the short ends, door placement working as expected.

**Implication:** The planned `AHallwayRoom` subclass (CLAUDE.md Section 2) may be simpler
to implement than anticipated. A hallway could be a `MasterRoom` configured with:
- A fixed narrow `SizeOverride` (e.g. 1–2 cells wide, variable length)
- `bSuppressFlavor = true` (already in the plan)
- `GenerationType` forced to Uniform regardless of `URoomData` setting (also planned)

**Before implementing `AHallwayRoom`:** Investigate whether the `FloorManager` can simply
spawn a `MasterRoom` with a narrow `SizeOverride` and a hallway-specific `URoomData` asset,
rather than requiring a dedicated subclass. A subclass adds maintenance overhead; a
well-configured `URoomData` may be sufficient for v1.

**Tag for follow-up:** Revisit during Step 10 (AHallwayRoom, AStaircaseRoom) and during
Step 5 (AFloorManager layout design) — hallway placement in the room grid will need to
be validated against the bin-packing / overlap logic.

---

### 12.2 Column Mesh Per-Side Yaw Override

**Observation (Step 4/5 door testing):** When a 2-cell door with columns is placed adjacent
to a corner cell, the left and right column meshes may not align with the corner mesh because
the same `ColumnMesh` is placed with the same transform on both sides of the opening. Corner
meshes are rotated per-corner; column meshes are not rotated at all.

**Note on bUseColumns (implemented):** The simpler problem — flanking cells always being
reserved as empty even when no `ColumnMesh` was wanted — was resolved by adding
`UDoorData::bUseColumns = false` (default). When false, flanking cells receive normal wall
modules from `WallData`. The per-side yaw issue is a *separate* concern affecting doors
that do use columns; it is not related to `bUseColumns`.

**Deferred feature:** Add per-side yaw overrides to `UDoorData`:

```cpp
// Proposed additions to UDoorData
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Placement")
float LeftColumnYaw = 0.f;   // applied to the cell at CellOffset - 1

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Placement")
float RightColumnYaw = 0.f;  // applied to the cell at CellOffset + Width
```

`MakeWallTransform` would receive this yaw in addition to the face yaw.

**Before implementing:** Confirm whether a separate `ColumnMesh` per side (two fields instead
of one shared mesh) would be cleaner than yaw-only. Different geometry on each side is
plausible for asymmetric doorways. Also verify the corner alignment issue is purely rotational
vs a pivot mismatch that a yaw alone cannot solve.

**Tag for follow-up:** Revisit when introducing new wall style assets with asymmetric corner
pieces, or when designers report consistent alignment problems at door-adjacent corners.

---

## 13. Adding a New Developer Checklist

If you are joining this project:

- [ ] Read `CLAUDE.md` in full — it is the architectural contract. Code that contradicts it
      is wrong, not the spec.
- [ ] Regenerate Rider project files: right-click `.uproject` → Generate Rider Project.
- [ ] Build the project once from Rider before making changes to ensure the baseline compiles.
- [ ] Understand the 100cm cell grid convention — every coordinate in the generation system
      is in cell units, not centimeters, unless explicitly named `*Cm`.
- [ ] Do not add `UE_LOG(LogTemp, ...)` calls. Use `DevLog->LogImportant(...)` or
      `UE_LOG(LogBuildingGenerator, ...)`.
- [ ] Do not place ISM instance data in any `UPROPERTY(Replicated)` field — geometry is
      always regenerated locally from seed.
- [ ] Do not call `SpawnActor` for `AMasterRoom` from anywhere except
      `ABuildingManager::RequestRoomSpawn()`. Do not call `SpawnActor` for `AFloorManager`
      from anywhere except `ABuildingManager::SpawnFloorManagersFromAsset()`.
- [ ] All debug visualization code goes inside `#if WITH_EDITOR / #endif`.
- [ ] When a step from `CLAUDE.md` Section 13 is completed, mark it done and update both
      `CLAUDE.md` Section 13 and this file's step sections.

---

*Last updated: Steps 5–7 complete plus two architectural refactors. AFloorManager Details panel, PreviewLayout (5 validation checks + full debug visualization), ABuildingManager DataAsset-driven spawning. OwningBuildingManager removed from AFloorManager — replaced by GenerateLayoutWith(ABuildingManager*) and standalone GenerateLayout(). ABuildingManager::FloorManagers (level-actor refs) replaced by BuildingDataAsset (UBuildingData*) + SpawnFloorManagersFromAsset(). Three editor buttons: GenerateBuilding (grid preview all floors), SpawnBuilding (full geometry), ClearBuilding. UBGDevLog + UBGVisualizer split from UDebugLog. UDoorData::bUseColumns added (default false). UE 5.7 → 5.8.*
