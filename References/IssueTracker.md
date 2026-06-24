# Building Generator — Issue Tracker

This document records compiler errors and build problems encountered during development,
their root causes, and the exact fixes applied. Read alongside DEVDOC.md for full context.

Each entry follows the same structure: **Symptom → Root Cause → Fix → Lesson**.

---

## Table of Contents

1. [Missing FRoomPlacement / FDoorPlacement / ERoomFace Definitions](#1-missing-froomplacement--fdoorplacement--eroomface-definitions)
2. [UPROPERTY Wrapped in WITH_EDITOR](#2-uproperty-wrapped-in-with_editor)
3. [UFUNCTION Wrapped in WITH_EDITORONLY_DATA](#3-ufunction-wrapped-in-with_editoronly_data)
4. [EDebugLogLevel Name Collision — Reference File in Source Tree](#4-edebogloglevel-name-collision--reference-file-in-source-tree)
5. [URoomData Name Collision — ProceduralDungeon Plugin](#5-uroomdata-name-collision--proceduraldungeon-plugin)
6. [PlaceFloorMeshes Fallback Overflow — Multi-Cell Mesh at 1×1 Transform](#6-placefloormmeshes-fallback-overflow--multi-cell-mesh-at-11-transform)
7. [Duplicate "Preview" Section in Details Panel](#7-duplicate-preview-section-in-details-panel)
8. [Floor Fill Holes at Last Interior Row — AllowRotation=false Gate](#8-floor-fill-holes-at-last-interior-row--allowrotationfalse-gate)
9. [Latent bNonSquare Gate in PlaceCeilingMeshes](#9-latent-bnonsquare-gate-in-placeceilingmeshes--ceiling-allowrotationfalse)
10. [UHT Duplicate Type Names — DebugLog.h and BGDevLog.h](#10-uht-duplicate-type-names--deblogh-and-bgdevlogh)
11. [Door Flanking Cell Gap — Wall Modules Missing Adjacent to Doorway](#11-door-flanking-cell-gap--wall-modules-missing-adjacent-to-doorway)

---

## 1. Missing FRoomPlacement / FDoorPlacement / ERoomFace Definitions

**Step context**: Attempting to compile after Step 3 was written.

### Symptom

```
error C2027: use of undefined type 'FRoomPlacement'
error C2027: use of undefined type 'FDoorPlacement'
error C2065: 'ERoomFace': undeclared identifier
```

### Root Cause

`MasterRoom.h` and `MasterRoom.cpp` referenced `FRoomPlacement`, `FDoorPlacement`, and
`ERoomFace` — all of which are specified in CLAUDE.md Section 4. However, Step 5 of the
implementation plan is where these structs were scheduled to be formally created. The Step 3
code was written with those types in mind but no header defined them yet.

### Fix

Created `Source/BuildingGenerator/Public/RoomPlacement.h` containing:

- `ERoomFace` — enum for the four cardinal wall faces (North, South, East, West)
- `FDoorPlacement` — struct describing a single door on a room face
- `FRoomPlacement` — struct describing a room's position, size, rotation, and doors

`AMasterRoom` is forward-declared in `RoomPlacement.h` (rather than included) to avoid a
circular dependency: `MasterRoom.h` includes `RoomPlacement.h`, so `RoomPlacement.h` cannot
include `MasterRoom.h`. A forward declaration is sufficient for `TSubclassOf<AMasterRoom>`
in a UPROPERTY.

`MasterRoom.h` was updated to replace `#include "RoomData.h"` with `#include "RoomPlacement.h"`.
`RoomData.h` is now pulled in transitively through `RoomPlacement.h`.

### Lesson

When writing code for a step that depends on types defined in a future step, the compiler
cannot distinguish "not yet written" from "missing." If Steps 1-3 are being compiled
together, all referenced types must exist. Either define prerequisite types early (as was
done here) or restructure the step order so dependencies are always satisfied before use.

---

## 2. UPROPERTY Wrapped in WITH_EDITOR

**Step context**: First compile attempt after adding visual debug properties to `DebugLog.h`.

### Symptom

```
DebugLog.h(116): Error: UProperties should not be wrapped by WITH_EDITOR,
use WITH_EDITORONLY_DATA instead.
```

### Root Cause

All editor-only `UPROPERTY` members in `DebugLog.h` (visualization toggles, colors,
coordinate text settings, `CoordinateTextComponents`) were wrapped in `#if WITH_EDITOR`.
Unreal Header Tool (UHT) enforces a distinction between the two guards:

| Guard | Purpose | Allowed contents |
|---|---|---|
| `WITH_EDITOR` | Code that runs only in editor builds | Functions, non-UPROPERTY members, implementations |
| `WITH_EDITORONLY_DATA` | Data that exists only in editor builds | `UPROPERTY` members |

UHT rejects `UPROPERTY` inside `WITH_EDITOR` because UHT needs to register reflected
properties independently of the editor build condition.

### Fix

Split the single `#if WITH_EDITOR` block in `DebugLog.h` into two separate blocks:

```cpp
// All UPROPERTY members (visualization toggles, colors, coordinate text, CoordinateTextComponents)
#if WITH_EDITORONLY_DATA
    UPROPERTY(...)
    bool bShowGrid = false;
    // ... all other UPROPERTYs
#endif // WITH_EDITORONLY_DATA

// Non-UPROPERTY members and function declarations
#if WITH_EDITOR
    FOnCreateTextComponent OnCreateTextComponent;
    FOnDestroyTextComponent OnDestroyTextComponent;
    void DrawGrid(...);
    // ... all function declarations
#endif // WITH_EDITOR
```

The forward declaration of `UTextRenderComponent` (needed by the `CoordinateTextComponents`
UPROPERTY) was also moved from `WITH_EDITOR` to `WITH_EDITORONLY_DATA`. The delegate type
declarations (`DECLARE_DELEGATE_*`) stayed in `WITH_EDITOR` since they are not UPROPERTYs.

The `.cpp` implementations remain in `#if WITH_EDITOR` — this is correct because
`WITH_EDITOR` implies `WITH_EDITORONLY_DATA`, so implementations in `WITH_EDITOR` can
freely access properties declared in `WITH_EDITORONLY_DATA`.

### Lesson

In any class that has both editor-only properties and editor-only functions, you will
always need two separate guards. `WITH_EDITOR` and `WITH_EDITORONLY_DATA` are not
interchangeable. Apply this split from the start whenever writing editor-only members:
properties go in `WITH_EDITORONLY_DATA`, everything else goes in `WITH_EDITOR`.

---

## 3. UFUNCTION Wrapped in WITH_EDITORONLY_DATA

**Step context**: Immediately after fixing Issue 2; a linter applied the same
`WITH_EDITOR` → `WITH_EDITORONLY_DATA` replacement to MasterRoom.h's preview section,
incorrectly placing `UFUNCTION` declarations inside `WITH_EDITORONLY_DATA`.

### Symptom

Two UHT errors, one per affected UFUNCTION:

```
MasterRoom.h: Error: UFunctions should not be wrapped by WITH_EDITORONLY_DATA,
use WITH_EDITOR instead.
```

### Root Cause

`MasterRoom.h` had a single block containing both `UPROPERTY` members (`PreviewPlacement`,
`PreviewSeed`) and `UFUNCTION(CallInEditor)` declarations (`PreviewRoom`, `ClearPreview`),
plus a plain function declaration (`DrawDebugGrid`). The block was changed wholesale from
`WITH_EDITOR` to `WITH_EDITORONLY_DATA` to fix the UPROPERTY error, but that incorrectly
wrapped the UFUNCTIONs as well.

UHT enforces the mirror rule: `UFUNCTION` must be in `WITH_EDITOR`, not
`WITH_EDITORONLY_DATA`.

### Fix

Split the block into the required two guards:

```cpp
// UPROPERTYs only
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Category = "MasterRoom|Preview")
    FRoomPlacement PreviewPlacement;

    UPROPERTY(EditAnywhere, Category = "MasterRoom|Preview")
    int32 PreviewSeed = 12345;
#endif // WITH_EDITORONLY_DATA

// UFUNCTIONs and plain function declarations
#if WITH_EDITOR
    void DrawDebugGrid() const;

    UFUNCTION(CallInEditor, Category = "MasterRoom|Preview")
    void PreviewRoom();

    UFUNCTION(CallInEditor, Category = "MasterRoom|Preview")
    void ClearPreview();
#endif // WITH_EDITOR
```

### Lesson

The split is always: `WITH_EDITORONLY_DATA` for `UPROPERTY`, `WITH_EDITOR` for `UFUNCTION`
and plain function declarations. A mixed block that contains both will always need to be
split. When a linter or automated tool applies a guard replacement, verify that it has not
grouped unlike constructs together.

---

## 4. EDebugLogLevel Name Collision — Reference File in Source Tree

**Step context**: First compile after `DebugLog.h` was added to the project.

### Symptom

```
DebugHelpers.h(12): Error: Enum 'EDebugLogLevel' shares engine name 'EDebugLogLevel'
with enum 'EDebugLogLevel' in DebugLog.h(8)
```

### Root Cause

The `References/` folder contains `DebugHelpers.h` and `DebugHelpers.cpp` — files ported
from the previous grid visualizer project for reference. Both files are physically inside
the module's `Source/` directory, which means UBT compiles them and UHT processes them as
part of the `BuildingGenerator` module.

`DebugHelpers.h` defines `EDebugLogLevel` with the same values and name as the new
`DebugLog.h`. UHT registers reflected types globally by their engine name (the class/enum
name without the U/E/F prefix). Two enums with the same name in the same module — even in
different folders — produce a hard collision.

### Fix

Renamed `EDebugLogLevel` to `EDebugHelpersLogLevel` in both `DebugHelpers.h` and
`DebugHelpers.cpp`. This is a safe change because `DebugHelpers` is a reference file: no
production code in this project includes or depends on it.

### Lesson

Any `.h` or `.cpp` file under the module `Source/` directory is compiled by UBT and
processed by UHT, regardless of what the containing folder is named. "References",
"Docs", "Archive" — the names carry no meaning to the build system. A file in
`References/Foo.h` that declares a `UCLASS` or `UENUM` is fully compiled.

If a folder is intended to hold reference files only, either:
- Use a file extension UBT ignores (e.g., rename `.h` to `.h.ref`)
- Move the folder outside the `Source/` tree entirely
- Ensure all reflected type names in reference files are unique (prefix them)

---

## 5. URoomData Name Collision — ProceduralDungeon Plugin

**Step context**: First compile after `RoomData.h` was added to the project.

### Symptom

```
PDRoomData.h(38): Error: Class 'URoomData' shares engine name 'RoomData'
with class 'URoomData' in RoomData.h(34)
```

### Root Cause

The ProceduralDungeon plugin (located at `Plugins/ProceduralDungeon/`) is present in the
project's `Plugins` folder as a reference for the Step 9 BSP port. Plugins in a project's
`Plugins/` folder are compiled automatically unless explicitly disabled. The plugin's
`PDRoomData.h` defines `URoomData` (an unprefixed class name, unusual for a plugin) which
collides with this project's `URoomData` in `RoomData.h`.

UHT registers UClasses by engine name — the class name minus the `U` prefix. Both classes
have engine name `RoomData`, which UHT treats as a conflict even though they are in
different modules.

### Fix

Disabled the ProceduralDungeon plugin in `BuildingGenerator.uproject`:

```json
{
    "Name": "ProceduralDungeon",
    "Enabled": false
}
```

The plugin files remain in `Plugins/ProceduralDungeon/` for reference. Setting
`"Enabled": false` in the `.uproject` tells UBT to skip compiling it entirely without
removing it from the project directory.

After editing `.uproject`, project files must be regenerated (right-click the `.uproject`
in Explorer → "Generate Visual Studio project files", or equivalent in Rider).

### Lesson

A plugin in the project's `Plugins/` folder is not automatically inert — it is compiled.
"In the folder for reference" and "disabled from the build" are two different things. If a
plugin is present only for code reference and not as an active dependency, disable it in
the `.uproject` immediately. Do not assume that keeping it in a subfolder prevents it from
being built.

When a name collision occurs with a plugin type, the options are:
1. Disable the plugin (preferred if it is not yet needed)
2. Rename your own type to avoid the conflict
3. Never change the plugin's source — that creates a maintenance burden

---

---

## 6. PlaceFloorMeshes Fallback Overflow — Multi-Cell Mesh at 1×1 Transform

**Step context**: First test run after weighted random floor fill was implemented.

### Symptom

Seed-dependent visual overlap between large floor tiles and adjacent 1×1 tiles. The
large tile appeared to bleed into cells that already had a separate mesh placed in them.
Only visible with certain seeds — not with every generation.

### Root Cause

When neither orientation of a multi-cell mesh (e.g., 1×2) can fit at a cell,
`PlaceFloorMeshes_Implementation` fell back to placing **the same mesh** (`Entry->Mesh`)
at a **1×1 transform** via `MakeCellTransform(X, Y, 1, 1, 0)`.

`MakeCellTransform` places the mesh center at `(X + 0.5) * 100, (Y + 0.5) * 100` — the
center of one grid cell (100 × 100 cm). A 1×2 mesh is 100 × 200 cm with a BottomCenter
pivot, so it extends ±100 cm in Y from that center:

```
Placed at Y center = (Y + 0.5) * 100
Mesh Y range: [(Y + 0.5)*100 - 100, (Y + 0.5)*100 + 100]
           = [(Y - 0.5)*100, (Y + 1.5)*100]
```

This bleeds 50 cm upward into cell Y-1 and 50 cm downward into cell Y+1. Cell Y-1 was
already processed (earlier scan row) and cell Y+1 was already claimed (which is why the
1×2 couldn't fit vertically — that's the exact condition that triggers the fallback).
Both adjacent cells have geometry; the oversized fallback mesh visually overlaps both.

The seed-dependency comes from which cells happen to be surrounded by claimed neighbors
— this varies by seed, which controls which mesh and orientation each earlier cell received.

### Fix

Before the main fill loop, both `PlaceFloorMeshes_Implementation` and
`PlaceCeilingMeshes_Implementation` scan their respective mesh arrays for the first entry
with `CellsX == 1 && CellsY == 1` and store it as `FallbackEntry`.

The fallback path uses `FallbackEntry->Mesh` instead of `Entry->Mesh`. If `FallbackEntry`
is null (no 1×1 entry in the pool), the cell is **skipped entirely** — no mesh is placed.
This avoids geometry overflow at the cost of a visible gap, which is far less destructive
than an oversized mesh bleeding into cells that already have geometry.

A `LogCritical` warning fires at startup if no 1×1 entry is found, so designers know
immediately that a fallback gap will occur.

```cpp
if (!bPlaced && FallbackEntry)
{
    GetOrCreateFloorISMC(FallbackEntry->Mesh)->AddInstance(MakeCellTransform(X, Y, 1, 1, 0.f));
    Claimed[CellIndex(X, Y)] = true;
    ++FallbackCount;
}
// Cell is left uncovered if FallbackEntry == nullptr.
```

### Lesson

A fallback that places "the same mesh at a smaller transform" is only safe if the mesh
physically fits at that smaller size. A 1×2 mesh at a 1×1 transform is geometrically
wrong — the mesh geometry is fixed at 100 × 200 cm regardless of the transform's scale.

When writing any fallback path that reduces a footprint to 1×1, always select a mesh
whose declared `CellsX == 1 && CellsY == 1`, not whichever mesh the main path tried.
The two are separate concerns: what mesh the weighted random chose, and what mesh
physically fits in the fallback space.

Any `FloorData` asset containing multi-cell meshes must include at least one 1×1 entry.
This is now enforced at generation time by the `LogCritical` check.

---

---

## 7. Duplicate "Preview" Section in Details Panel

**Step context**: Discovered during Step 3 testing after adding `PreviewRoomHeightCm` to `MasterRoom.h`.

### Symptom

The Details panel for `AMasterRoom` showed two separate "Preview" sections — one containing
the three `UPROPERTY` fields (`PreviewPlacement`, `PreviewSeed`, `PreviewRoomHeightCm`) and
another containing the two `CallInEditor` buttons (`PreviewRoom`, `ClearPreview`). Functionally
identical but visually confusing.

### Root Cause

Unreal Engine 5 renders `UPROPERTY` fields and `UFUNCTION(CallInEditor)` entries as separate
sub-panels when they share the same category string. Even though both blocks used
`Category = "MasterRoom|Preview"`, the engine creates a distinct collapsible section for
`UPROPERTY` members and another for `UFUNCTION` members in the same category.

This is intrinsic to the UE5 Details panel — UHT processes them through different pipelines,
and the rendered result is two identically-named sections stacked vertically.

### Fix

Renamed the three `UPROPERTY` categories from `"MasterRoom|Preview"` to `"MasterRoom|Debug"`,
grouping them with other editor-only debug data. The `UFUNCTION(CallInEditor)` declarations
kept `"MasterRoom|Preview"` — they are the action buttons, which belong in their own section.

```cpp
// UPROPERTYs → "MasterRoom|Debug"
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Category = "MasterRoom|Debug")
    FRoomPlacement PreviewPlacement;

    UPROPERTY(EditAnywhere, Category = "MasterRoom|Debug")
    int32 PreviewSeed = 12345;

    UPROPERTY(EditAnywhere, Category = "MasterRoom|Debug")
    int32 PreviewRoomHeightCm = 300;
#endif // WITH_EDITORONLY_DATA

// UFUNCTIONs stay in "MasterRoom|Preview"
#if WITH_EDITOR
    UFUNCTION(CallInEditor, Category = "MasterRoom|Preview")
    void PreviewRoom();

    UFUNCTION(CallInEditor, Category = "MasterRoom|Preview")
    void ClearPreview();
#endif // WITH_EDITOR
```

Result: one "Debug" section with the three fields, one "Preview" section with the two buttons.

### Lesson

In the UE5 Details panel, `UPROPERTY` and `UFUNCTION(CallInEditor)` entries in the same
category string always render as **two separate sections**, regardless of declaration order.
If you want them visually separated (e.g., config fields vs action buttons), put them in
different category strings — that is the only way to achieve a clean single-header grouping
for each. If you want them to appear as one unified block, this is not possible in the
standard UE5 Details panel without a custom Details Customization.

---

## 8. Floor Fill Holes at Last Interior Row — AllowRotation=false Gate

**Step context**: Discovered during Step 3 testing after ceiling fill was confirmed working.

### Symptom

```
LogBuildingGenerator: Warning: [MyMasterRoom_C_1] [MESH] PlaceFloorMeshes: cell (1,5) unfillable — add a 1x1 entry to FloorData
LogBuildingGenerator: Warning: [MyMasterRoom_C_1] [MESH] PlaceFloorMeshes: cell (2,5) unfillable — add a 1x1 entry to FloorData
LogBuildingGenerator: Warning: [MyMasterRoom_C_1] [MESH] PlaceFloorMeshes: cell (3,5) unfillable — add a 1x1 entry to FloorData
LogBuildingGenerator: Warning: [MyMasterRoom_C_1] [MESH] PlaceFloorMeshes: cell (4,5) unfillable — add a 1x1 entry to FloorData
LogBuildingGenerator: Warning: [MyMasterRoom_C_1] [MESH] Floor instances placed: 5
```

Cells in the last interior row were left empty. Ceiling fill with the same mesh set worked
correctly (no unfillable warnings). Floor fill had `AllowRotation = false` on its 1×2 entry;
ceiling fill had `AllowRotation = true`.

### Root Cause

The exhaustive pool algorithm tries both footprint orientations (`CellsX×CellsY` and
`CellsY×CellsX`) for non-square meshes to guarantee fill. However, the gate that enabled
the alternate orientation attempt read:

```cpp
if (bCanRotate && bNonSquare)
{
    // try the alternate orientation
}
```

`bCanRotate` was `Entry->AllowRotation`. With `AllowRotation = false` on the floor's 1×2
mesh, `bCanRotate` was `false`, so only the natural `CellsX=1, CellsY=2` orientation was
ever tried. The last interior row had only one free row of cells remaining — `1×2` needs
two rows and failed. No alternate orientation was tried, so the cell was reported unfillable.

Ceiling fill worked because its 1×2 entries had `AllowRotation = true`, enabling the
`CellsY×CellsX = 2×1` orientation attempt, which succeeded on the last row.

The intent of `AllowRotation` was understood to mean "attempt the rotated orientation."
The correct meaning is "randomize which orientation is tried *first*" — both should
always be tried for non-square meshes.

### Fix

Changed the gate from `if (bCanRotate && bNonSquare)` to `if (bNonSquare)` — the alternate
orientation is now **always attempted** for any non-square mesh, regardless of `AllowRotation`.
The `AllowRotation` flag was moved inside the block to control only the random starting
orientation:

```cpp
if (bNonSquare)
{
    // AllowRotation now means: randomize which orientation we attempt first.
    // Both are always tried — this flag only affects which comes first.
    const bool bFlipFirst = bCanRotate && Stream.FRand() < 0.5f;

    const int32 TryAX = bFlipFirst ? Entry->CellsY : Entry->CellsX;
    const int32 TryAY = bFlipFirst ? Entry->CellsX : Entry->CellsY;
    const int32 TryBX = bFlipFirst ? Entry->CellsX : Entry->CellsY;
    const int32 TryBY = bFlipFirst ? Entry->CellsY : Entry->CellsX;

    if (CanPlaceMesh(X, Y, TryAX, TryAY, Claimed)) { /* place A */ bPlaced = true; }
    else if (CanPlaceMesh(X, Y, TryBX, TryBY, Claimed)) { /* place B */ bPlaced = true; }
}
```

`AllowRotation = false` now means "always prefer the natural `CellsX×CellsY` orientation
first, but still attempt the alternate." The same fix was confirmed already present in
`PlaceCeilingMeshes_Implementation` (which never exhibited the bug because its assets used
`AllowRotation = true`).

`FFloorMeshEntry::AllowRotation` documentation in `FloorData.h` was updated to reflect
the new semantic.

### Lesson

A flag named `AllowRotation` creates an implicit assumption: "if false, do not rotate."
For fill algorithms where rotation is a *coverage* mechanism (not just an aesthetic one),
that assumption is wrong — it makes the flag a hidden prerequisite for fill correctness.

When a non-square mesh's alternate orientation is required for completeness (e.g., a 1×2
at the last row needs to be tried as 2×1), never gate that attempt on a designer-facing
flag. Reserve the flag for the order or aesthetic of the attempt, not its existence.

---

---

## 9. Latent bNonSquare Gate in PlaceCeilingMeshes — Ceiling AllowRotation=false

**Step context**: Found during FillMode implementation; ceiling tests passed only because all ceiling assets had `AllowRotation = true`.

### Symptom

No runtime failure at the time of discovery. The bug was latent — any `UCeilingData` asset
with a non-square mesh entry where `AllowRotation = false` would have produced fill holes
at edge rows identical to Issue 8.

### Root Cause

The Issue 8 fix changed `if (bCanRotate && bNonSquare)` to `if (bNonSquare)` in
`PlaceFloorMeshes_Implementation`, but the identical gate in `PlaceCeilingMeshes_Implementation`
was not updated at the same time:

```cpp
// PlaceCeilingMeshes still had the old gate:
if (bCanRotate && bNonSquare)  // ← bCanRotate = AllowRotation = false → block skipped
```

All ceiling test assets happened to have `AllowRotation = true`, so `bCanRotate` was
always `true` and the bug never triggered. The floor and ceiling functions diverged silently.

### Fix

Applied the same `if (bNonSquare)` gate to `PlaceCeilingMeshes_Implementation`, matching
the floor function exactly. `AllowRotation` still controls the starting-orientation coin flip
inside the block — the semantics are identical to the floor fix.

### Lesson

When the same logic bug is fixed in one of two near-identical functions (floor vs ceiling),
always fix both in the same commit. The fill functions are almost line-for-line identical —
any future divergence between them is a latent bug. A shared private helper (or a templated
fill function) would eliminate this class of problem entirely.

---

---

## 10. UHT Duplicate Type Names — DebugLog.h and BGDevLog.h

**Step context**: First compile after cloning the project on a second machine (Steps 5–7 already complete on the primary machine).

### Symptom

```
Error: Enum 'EBGLogCategory' shares engine name 'EBGLogCategory' with enum 'EBGLogCategory' in BGDevLog.h
Error: Struct 'FBGPerformanceLog' shares engine name 'FBGPerformanceLog' with struct 'FBGPerformanceLog' in BGDevLog.h
```

### Root Cause

`DebugLog.h` and `DebugLog.cpp` were deleted from the primary development machine as
part of a refactor that migrated all logging functionality to `BGDevLog` and all editor
visualization to `BGVisualizer`. However, those deletions were not committed to the
repository — the files survived in version control.

When the project was cloned on a second machine, both `DebugLog.h` (containing the
original `EBGLogCategory` and `FBGPerformanceLog` declarations) and `BGDevLog.h`
(containing the replacement declarations with the same reflected names) were present.
UHT processes every header in the module and registers reflected types globally — two
headers with the same `UENUM`/`USTRUCT` name are a hard collision.

### Diagnosis

Before deleting, confirmed that the migration was complete:
- All logging call sites used `DevLog->LogImportant(...)` (not `DebugLog->...`)
- `DebugLog.cpp` included only `DebugLog.h` — no other file included `DebugLog.h`
- `BGDevLog.h` and `BGVisualizer.h` fully replaced all functionality from `DebugLog.h`

### Fix

Deleted `Source/BuildingGenerator/Public/DebugLog.h` and
`Source/BuildingGenerator/Private/DebugLog.cpp` from the project.

### Lesson

File deletions must be committed. Deleting a file locally without staging the deletion
leaves it in version control, so anyone who clones or pulls gets the file back. After
deleting source files that are part of a refactor, always verify with `git status` that
the deletion is staged before committing.

---

## 11. Door Flanking Cell Gap — Wall Modules Missing Adjacent to Doorway

**Step context**: Discovered during Step 5–7 testing with a `UDoorData` asset that had no `ColumnMesh` assigned.

### Symptom

When a door was placed with no `ColumnMesh` set on the `UDoorData`, the two wall cells
immediately flanking the door opening (one cell to each side) were visibly empty. Wall
modules were placed on every other wall cell, but the cells at `CellOffset-1` and
`CellOffset+DoorWidth` always showed gaps.

### Root Cause

`PlaceWallMeshStacks_Implementation` uses a `FindColumnForPos` lambda to check whether
a wall position is reserved as a column cell. The original implementation returned a
`FDoorPlacement*` for **any** door at that flanking position, regardless of whether
`ColumnMesh` was set:

```cpp
// Original — always reserved flanking cells for any door
auto FindColumnForPos = [&](ERoomFace CurFace, int32 FacePos) -> const FDoorPlacement*
{
    for (const FDoorPlacement& Door : ActivePlacement.Doors)
    {
        if (Door.Face != CurFace) continue;
        const UDoorData* DData = GetEffectiveDoorData(Door);
        if (!IsValid(DData)) continue;
        const int32 Width = GetDoorWidthCells(DData);
        if (FacePos == Door.CellOffset - 1 || FacePos == Door.CellOffset + Width)
            return &Door;   // ← returned regardless of ColumnMesh
    }
    return nullptr;
};
```

When `FindColumnForPos` returned non-null, Branch 1 of the wall placement loop consumed
the cell and skipped it. But Branch 1 then checked `if (IsValid(DData->ColumnMesh))` before
placing — so the cell was consumed (preventing wall module placement) but nothing was
placed. The result: a permanently empty flanking cell for every door without a column mesh.

### Fix

Added `UDoorData::bUseColumns = false` (default). `FindColumnForPos` was changed to skip
any door where `!DData->bUseColumns`:

```cpp
auto FindColumnForPos = [&](ERoomFace CurFace, int32 FacePos) -> const FDoorPlacement*
{
    for (const FDoorPlacement& Door : ActivePlacement.Doors)
    {
        if (Door.Face != CurFace) continue;
        const UDoorData* DData = GetEffectiveDoorData(Door);
        if (!IsValid(DData) || !DData->bUseColumns) continue;  // ← opt-in guard added
        const int32 Width = GetDoorWidthCells(DData);
        if (FacePos == Door.CellOffset - 1 || FacePos == Door.CellOffset + Width)
            return &Door;
    }
    return nullptr;
};
```

Branch 1 retained its null guard for `ColumnMesh` (handles `bUseColumns=true` + null mesh
without crashing). The null guard was already present; this issue confirmed it must stay.

**Behavior table:**

| `bUseColumns` | `ColumnMesh` | Flanking cell result |
|---|---|---|
| `false` (default) | any | Normal wall module from WallData |
| `true` | set | ColumnMesh placed |
| `true` | null | Cell reserved, left empty |

### Lesson

"Reserved because something might go there" is not the same as "reserved because something
will go there." The original code reserved flanking cells unconditionally, which silently
made column placement mandatory for correct behavior. Any flag that changes a cell from
"passthrough" to "reserved" should be opt-in, not opt-out — default behavior should be
the most permissive case (fill the cell with wall modules) and the designer explicitly
enables the restriction (`bUseColumns = true`) when they need it.

---

*Last updated: Steps 5–7 complete. Issues 10 and 11 added — UHT DebugLog duplicate (resolved by deleting legacy files), door flanking cell gap (resolved by bUseColumns opt-in on UDoorData).*
