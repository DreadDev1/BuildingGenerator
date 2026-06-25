# Building Generator — Designer Guide

This document is the designer's instruction manual for the Building Generator system.
It is updated at the end of each implementation step to reflect what is currently
available to test and configure in the Unreal Editor.

For the full technical architecture, see DEVDOC.md.
For the implementation roadmap, see CLAUDE.md (Section 13).

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Current Status — What Is Available to Test](#2-current-status--what-is-available-to-test)
3. [Step 3 — Testing the Room Grid and Floor Fill](#3-step-3--testing-the-room-grid-and-floor-fill)
   - [Test A: Grid Visualization Only (No Data Assets Required)](#test-a-grid-visualization-only-no-data-assets-required)
   - [Test B: Floor Mesh Instances](#test-b-floor-mesh-instances)
4. [Step 3b — Testing Ceiling Mesh Fill](#4-step-3b--testing-ceiling-mesh-fill)
5. [Step 4 — Testing Wall Mesh Stacks](#5-step-4--testing-wall-mesh-stacks)
6. [Step 5 — Setting Up AFloorManager](#6-step-5--setting-up-afloormanager)
7. [Step 6 — Using PreviewLayout](#7-step-6--using-previewlayout)
8. [Step 7 — Generating a Full Building via ABuildingManager](#8-step-7--generating-a-full-building-via-abuildingmanager)
9. [Tips and Experiments](#9-tips-and-experiments)
10. [Debug Components Reference](#10-debug-components-reference)

---

## 1. System Overview

The Building Generator places fully interior-accessible buildings at any world location.
Each building is composed of floors, and each floor is composed of rooms. All geometry
is generated procedurally from a seed value, meaning the same seed always produces
the same building.

**Key actors you will work with:**

| Actor | Purpose |
|---|---|
| `AMasterRoom` | A single room. Owns all floor, ceiling, wall, and decoration geometry. |
| `AFloorManager` | One per floor. Holds the room layout for that floor. Place in the level and assign rooms via the `RoomPlacements` array in the Details panel. |
| `ABuildingManager` | The root actor placed in the world. Drives the full building. Add `AFloorManager` actors to its `FloorManagers` array, then press **GenerateBuilding**. |

**Key data assets you will configure:**

| Asset | Purpose |
|---|---|
| `URoomData` | Defines a room style — references all sub-assets below. |
| `UFloorData` | The set of floor meshes for a room style, with sizes and weights. |
| `UCeilingData` | The set of ceiling meshes. Same format as FloorData. Ceiling height is controlled by `PreviewRoomHeightCm` on the `AMasterRoom` actor in the editor; at runtime it is driven by `AFloorManager::RoomHeightCm`. |
| `UWallData` | A pool of wall modules, each with its own mesh stack and footprint. |
| `UDoorData` | Door mesh, interaction type, and sounds. *(Used in a later step)* |

---

## 2. Current Status — What Is Available to Test

| Step | Feature | Status |
|---|---|---|
| Step 1 | Data asset classes (URoomData, UFloorData, UWallData, UDoorData, UCeilingData) | Available — assets can be created |
| Step 2 | Room grid allocation and cell classification | Available — debug visualization works |
| Step 3 | Weighted random floor mesh fill | Available — Random or Uniform fill mode; multi-mesh pool; AllowRotation |
| Step 3b | Weighted random ceiling mesh fill | Available — same modes as floor; configure via UCeilingData |
| Step 4 | Wall mesh stacks + corner pieces + door mesh placement | Available — WallData with BaseMesh/TopMesh required; CornerMesh optional; bUseColumns controls flanking cell reservation |
| Step 5 | AFloorManager Details panel and room layout | Available — place AFloorManager, fill RoomPlacements, use ClearLayout |
| Step 6 | PreviewLayout debug visualization in AFloorManager | Available — draws floor grid, room footprints; 5 validation checks |
| Step 7 | ABuildingManager spawning | Available — GenerateBuilding/ClearBuilding editor buttons |

---

## 3. Step 3 — Testing the Room Grid and Floor Fill

`AMasterRoom` has a built-in editor preview that lets you generate and inspect a single
room without running Play In Editor. Two levels of testing are available depending on
whether you have mesh assets ready.

---

### Test A: Grid Visualization Only (No Data Assets Required)

Use this to verify cell classification (Floor, Wall, Corner) and the debug overlay.
No mesh assets are needed.

**Setup:**

1. Place an `AMasterRoom` actor anywhere in the level.
2. Select it and open the **Details** panel.
3. Under **MasterRoom | Debug**, find `PreviewPlacement`:
   - Set `SizeOverride` to `(6, 6)` (or any non-zero value).
   - Leave `RoomDataAsset` empty.
4. Find the **Visualizer** component in the Details panel (listed under Components).
5. Enable the following:
   - `bShowGrid` — draws green lines at every cell boundary.
   - `bShowCellStates` — draws a colored box inside each cell.
6. Click the **PreviewRoom** button (found under **MasterRoom | Preview** in Details).

**What you will see:**

| Color | Cell type | Where it appears |
|---|---|---|
| Green lines | Grid boundary | Edges of every cell |
| Blue box | Floor cell | Interior cells — these receive floor and ceiling meshes |
| Black box | Wall cell | Edge cells (non-corner) — these will receive wall mesh stacks in Step 4 |
| Purple/Magenta box | Corner cell | The four corners — corner pieces, no doors allowed here |

A 6×6 room produces a 4×4 interior of Floor cells, a ring of Wall cells, and 4 Corner cells.

**Cell type terminology — important:**
- **Floor cell (blue)** — interior playable space. Not "occupied" in the sense of being
  blocked. These are the cells that receive floor tile meshes and will have ceiling meshes
  above them. Players walk through these cells.
- **Wall cell (black)** — NOT empty. These are reserved perimeter cells that will receive
  wall geometry in Step 4. They appear dark now because wall meshes have not been placed
  yet. Do not confuse "no mesh placed here yet" with "empty cell."
- **Corner cell (purple/magenta)** — the four corners of the room boundary. These receive
  a dedicated corner mesh piece. Doors cannot be placed on corner cells.

> **Expected log — null RoomDataAsset:** If `RoomDataAsset` is left empty, the Output Log
> will show a Critical warning and skip floor mesh placement. The grid visualization still
> draws correctly. This is intentional for Test A.

> **Coordinate labels:** `bShowCoordinates` on `AMasterRoom` requires the text component
> delegate to be bound. On `AFloorManager` this works immediately. On a standalone
> `AMasterRoom`, coordinate labels are displayed when the delegate is wired — this is done
> automatically when rooms are spawned via `AFloorManager::GenerateLayout`.

**To reset:** Click the **ClearPreview** button. This removes all debug lines and clears
any placed mesh instances.

---

### Test B: Floor Mesh Instances — Multi-Mesh Weighted Fill

Use this to configure and verify weighted random floor fill with multiple mesh types,
sizes, and optional rotation. This requires static mesh assets and three data assets.

---

#### Pivot Requirement

All floor mesh pivots must be at **Bottom-Center** — set in your DCC tool before import.
A pivot at the wrong position causes the mesh to appear offset from its cell boundary.

---

#### Step 1 — Prepare Your Static Meshes

You need at least two meshes for a meaningful weighted test:

| Mesh | Suggested size | Purpose |
|---|---|---|
| Small tile | 100 × 100 cm | 1×1 entries — required as the fallback mesh |
| Large tile | 100 × 200 cm | 1×2 entry — tests rotation and weighted selection |

> For a quick test using engine content, `Engine/BasicShapes/Plane` works as a 1×1 tile.
> A scaled version (or any flat rectangle) works for the 1×2. Pivot placement may be
> approximate for engine primitives — use real imported assets for final validation.

---

#### Step 2 — Create a UFloorData Asset

1. In the Content Browser → right-click → **Miscellaneous** → **Data Asset** → **FloorData**.
2. Name it `DA_FloorData_Test`.
3. Open the asset. Under **FloorData | Settings**, set `FillMode`:

| FillMode | Behavior |
|---|---|
| `Random` (default) | Every cell independently draws from the mesh pool. Each room looks different and seed variation is maximized. |
| `Uniform` | One mesh is chosen from the pool at generation time (respecting weights). The entire room uses that single tile. Different seeds → different uniform mesh. |

4. Add entries to **FloorMeshes** using the **+** button.

**Recommended starting setup (two entries):**

**Entry 0 — 1×1 tile (required fallback):**

| Field | Value | Notes |
|---|---|---|
| `Mesh` | Your 100×100 cm mesh | |
| `CellsX` | `1` | |
| `CellsY` | `1` | |
| `AllowRotation` | unchecked | No effect on a 1×1 mesh |
| `Weight` | `1.0` | |

**Entry 1 — 1×2 tile:**

| Field | Value | Notes |
|---|---|---|
| `Mesh` | Your 100×200 cm mesh | |
| `CellsX` | `1` | 100 cm on the X axis |
| `CellsY` | `2` | 200 cm on the Y axis |
| `AllowRotation` | checked | Randomizes which orientation (1×2 or 2×1) is tried first. Both are always attempted. |
| `Weight` | `2.0` | Appears roughly twice as often as the 1×1 entry |

> **About `AllowRotation`:** For non-square meshes, the generator **always** tries both
> orientations (1×2 and 2×1) to ensure full coverage. `AllowRotation` controls which is
> tried first — checked means the starting orientation is randomized per-cell (more visual
> variety), unchecked means the natural `CellsX×CellsY` orientation is always tried first.
> In most cases you want this checked on non-square tiles.

> **Why the 1×1 entry is recommended:** The exhaustive pool algorithm tries every candidate
> in every orientation before giving up on a cell, so a room filled with only 1×2 tiles
> will typically cover fully. However, a 1×1 entry guarantees coverage for any edge case
> where no larger tile fits (e.g., isolated single-cell gaps at non-standard room sizes).
> Without a 1×1 entry, those rare cells are left empty and the Output Log notes the count.

4. Save the asset.

---

#### Step 3 — Create or Update the URoomData Asset

1. Open `DA_RoomData_Test` (or create a new **RoomData** asset).
2. Set **FloorData** to `DA_FloorData_Test`.
3. Confirm `MinRoomSize` / `MaxRoomSize` are set (e.g. `(4,4)` / `(8,8)`).
4. Save the asset.

---

#### Step 4 — Configure and Preview the Room

1. Select your `AMasterRoom` actor in the level.
2. Under **MasterRoom | Debug**:
   - Set `PreviewPlacement.SizeOverride` to `(6, 6)` (or any size).
   - Set `PreviewPlacement.RoomDataAsset` to `DA_RoomData_Test`.
3. Click **PreviewRoom** (found under **MasterRoom | Preview** in Details).

---

**What to verify:**

- The Components panel shows **multiple entries under `FloorISMCPool`** — one per unique
  static mesh. With the two-entry setup above you should see two ISMC entries.
- Mesh instances cover every interior Floor cell with no gaps and no overlapping geometry.
- 1×2 tiles appear in both horizontal and vertical orientations across different seeds
  (change `PreviewSeed` and click **PreviewRoom** again to see variation).
- 1×2 tiles never visually bleed into adjacent cells.
- No mesh appears on Wall or Corner cells.

**Reading the Output Log:**

```
[MasterRoom_0] [MESH] Floor instances placed: N
[MasterRoom_0] [MESH] Fallback 1x1 cells: N
[MasterRoom_0] [MESH] Skipped cells (no 1x1 fallback): N   ← only if 1x1 entry is missing
```

- `Floor instances placed` counts every successful placement (any mesh size).
- `Fallback 1x1 cells` counts cells where no multi-cell mesh fit; the 1×1 entry was used.
- `Skipped cells` only appears when no 1×1 entry exists and a cell could not be covered.
  Add a 1×1 entry to eliminate these gaps.
- For most room sizes and a mixed 1×1/1×2 pool, `Fallback 1x1 cells` is zero — the
  exhaustive algorithm covers all cells with the larger tiles first.

---

**Tuning weights:**

Higher `Weight` = more frequent selection. Example ratios:

| Entry | Weight | Approximate share |
|---|---|---|
| 1×1 tile | 1.0 | ~33% of selections |
| 1×2 tile | 2.0 | ~67% of selections |

Weights do not need to sum to any specific value — they are relative. Setting all weights
to the same value gives equal probability.

---

## 4. Step 3b — Testing Ceiling Mesh Fill

Ceiling generation mirrors the floor fill exactly — same algorithm, same rotation logic,
same fallback behavior. The only differences are:
- Meshes come from a `UCeilingData` asset (referenced in `URoomData`).
- Meshes are placed at `AMasterRoom::RoomHeightCm` above the floor (set via `PreviewRoomHeightCm` in the editor).

---

### Pivot Requirement

All ceiling mesh pivots must be at **Bottom-Center**, the same as floor meshes. When placed
at `RoomHeightCm`, a BottomCenter-pivoted mesh sits with its bottom face at the ceiling
height — which is the visible underside of the ceiling as seen from the room.

---

### Step 1 — Create a UCeilingData Asset

1. In the Content Browser → right-click → **Miscellaneous** → **Data Asset** → **CeilingData**.
2. Name it `DA_CeilingData_Test`.
3. Open the asset. Under **CeilingData | Settings**, set `FillMode` — same options as FloorData:

| FillMode | Behavior |
|---|---|
| `Random` (default) | Every cell draws independently from the ceiling mesh pool. |
| `Uniform` | One mesh chosen from the pool tiles the entire ceiling. Weights still apply to the selection. |

4. Add entries to **CeilingMeshes** using the **+** button — same format as FloorData:

**Entry 0 — 1×1 tile (required fallback):**

| Field | Value |
|---|---|
| `Mesh` | Your 100×100 cm ceiling mesh |
| `CellsX` | `1` |
| `CellsY` | `1` |
| `AllowRotation` | unchecked |
| `Weight` | `1.0` |

**Entry 1 — optional larger tile:**

| Field | Value | Notes |
|---|---|---|
| `Mesh` | Your 100×200 cm ceiling mesh | |
| `CellsX` | `1` | |
| `CellsY` | `2` | |
| `AllowRotation` | checked | Randomizes which orientation is tried first; both are always attempted |
| `Weight` | `2.0` | |

> **Why the 1×1 entry is recommended:** Same guidance as FloorData — the exhaustive pool
> algorithm tries all candidates in all orientations before giving up, so a room filled with
> only 1×2 ceiling tiles will typically cover fully. Add a 1×1 entry to guarantee coverage
> for edge cases. Without it, any genuinely unfillable cell is skipped and logged.

4. Save the asset.

---

### Step 2 — Add CeilingData to URoomData

1. Open `DA_RoomData_Test`.
2. Set the `CeilingData` field to `DA_CeilingData_Test`.
3. Save the asset.

---

### Step 3 — Preview

1. Select your `AMasterRoom` actor.
2. Verify `PreviewPlacement.RoomDataAsset` is set to `DA_RoomData_Test` (under **MasterRoom | Debug**).
3. Set `PreviewRoomHeightCm` on the `AMasterRoom` actor to `300`, or whatever matches the
   visual top of your wall stack (Base + Middle + Top mesh heights combined). This field is
   also under **MasterRoom | Debug**.
4. Click **PreviewRoom** (under **MasterRoom | Preview**).

**What to verify:**

- The Components panel shows one or more entries under `CeilingISMCPool` — one per unique
  ceiling static mesh (same pool pattern as floors and walls).
- Ceiling meshes appear at the correct height (matching `PreviewRoomHeightCm` on the actor).
  If they appear at floor level, check that `PreviewRoomHeightCm` is non-zero on the actor.
- Coverage: every interior Floor cell has a ceiling mesh directly above it. No gaps
  (unless a cell's fallback was skipped due to a missing 1×1 entry).
- Rotation: with `AllowRotation = true`, 1×2 tiles appear in both orientations.
  Overlapping geometry indicates a missing 1×1 fallback entry — add one to fix it.

**Output Log:**

```
[MasterRoom_0] [MESH] Ceiling instances placed: N
[MasterRoom_0] [MESH] Fallback 1x1 cells: N
[MasterRoom_0] [MESH] Skipped cells (no 1x1 fallback): N   ← only if 1x1 entry is missing
```

- `Ceiling instances placed` counts every successful placement (any mesh size).
- `Fallback 1x1 cells` counts cells where only the 1×1 entry fit after the pool was exhausted.
- `Skipped cells` only appears when no 1×1 entry exists and a cell could not be covered.

With a mixed 1×2/2×2 pool and `AllowRotation = true`, `Fallback 1x1 cells` is typically zero.
Change `PreviewSeed` to verify different configurations.

---

## 5. Step 4 — Testing Wall Mesh Stacks

`AMasterRoom` places a wall module stack on every Wall cell. Each module consists of
up to four pieces — **Base → Middle1 → Middle2 → Top** — stacked vertically. BaseMesh
and TopMesh are required; the two Middle pieces are optional and control room height.

The generator bin-packs modules from your `UWallData` asset along each wall face,
selecting by weight and fitting by `CellsWide`. One `UInstancedStaticMeshComponent`
is created per unique static mesh encountered, stored in the `WallISMCPool` array
visible in the Components panel.

---

### What you need

| Asset | Required | Notes |
|---|---|---|
| `DA_RoomData_Test` | Yes | Created in Step 3 |
| `DA_WallData_Test` | Yes | Create now — see below |
| Wall static meshes | Yes | At minimum: one mesh for Base and one for Top |

---

### Step 1 — Choose or Import Wall Meshes

Any mesh that represents a wall section works. For a quick pivot-validation test,
use a thin box mesh (e.g. 100 × 10 × 100 cm).

> **Pivot convention (critical):** All wall mesh pieces must have a **BottomBackCenter**
> pivot — at the bottom of the mesh, centered left-to-right, flush with the exterior
> (back) face. This is what allows a single yaw rotation to orient the mesh for all four
> wall faces with no position offset correction.
>
> If you see walls floating away from the room edge or pointing inward, the pivot is wrong.
> Fix it in your DCC tool before re-importing.

---

### Step 2 — Create a UWallData Asset

1. Right-click in the Content Browser → **Miscellaneous** → **Data Asset**.
2. Select **WallData**. Name it `DA_WallData_Test`.
3. Open the asset. Under **WallModules**, click **+** to add one entry. Configure it:

**Per-module fields (inside WallModules entry):**

| Field | Value | Notes |
|---|---|---|
| `CellsWide` | `1` | This module covers 1 cell (100 cm) along the wall face |
| `BaseMesh` | Your base wall mesh | Required — generation skips modules without this |
| `MiddleMesh1` | Your middle wall mesh | Optional — leave empty for a Base + Top only stack |
| `MiddleMesh2` | Leave empty | Optional — add for a taller wall; requires MiddleMesh1 |
| `TopMesh` | Your top wall mesh | Required — generation skips modules without this |
| `PlacementOffset` | `(0, 0, 0)` initially | See depth correction note below |
| `BaseMeshStackHeight` | `0` initially | See gap correction note below |
| `MiddleMesh1StackHeight` | `0` | Same — only set if Middle1 shows a gap above it |
| `MiddleMesh2StackHeight` | `0` | Same — only set if Middle2 shows a gap above it |
| `Weight` | `1.0` | Weighted random selection — irrelevant with only one module |

**Asset-level fields (on the UWallData asset itself, not per module):**

| Field | Value | Notes |
|---|---|---|
| `CornerMesh` | Your corner mesh (optional) | Placed at all four room corners. See pivot note below. Leave empty for no corner geometry. |
| `bIsExteriorCapable` | unchecked | Not needed until Step 11 |

> **Corner mesh pivot:** The corner mesh must have a **BottomCenter** pivot — the same as
> floor tiles. It occupies one 100×100cm cell (the corner cell) and is rotated automatically
> for each of the four corners:
>
> | Corner | Yaw applied |
> |---|---|
> | SW (bottom-left) | 0° |
> | SE (bottom-right) | 90° |
> | NE (top-right) | 180° |
> | NW (top-left) | 270° |
>
> Design the mesh so that at 0° (SW corner) the interior-facing corner points toward the
> room center (NE direction). The other three corners are automatic rotations of this.
> A symmetric corner piece (e.g. a column or a quarter-circle) looks identical at all
> four rotations.

4. Save the asset.

> **Correcting mesh depth with PlacementOffset:**
> `PlacementOffset` is in *wall-forward space* — X points into the room, Y runs along
> the wall face, Z is up. This is independent of which room face the wall is on; the
> system rotates it automatically.
>
> If your wall mesh is deeper than 100cm (one cell), its interior face will overlap the
> adjacent floor cells. To pull it flush, set `PlacementOffset.X` to a negative value:
>
> | Mesh depth | PlacementOffset.X |
> |---|---|
> | 100cm (matches cell) | `0` |
> | 135cm | `-35` |
> | 150cm | `-50` |
> | 200cm | `-100` |
>
> You can fine-tune this in the editor: click **PreviewRoom**, observe the wall overlap,
> then adjust `PlacementOffset.X` on the module and click **PreviewRoom** again.
> No recompile is needed — it reads from the data asset.

> **Closing a gap between stacked pieces (BaseMeshStackHeight):**
> The generator uses each mesh's bounding box to determine how high the next piece
> should start. If a gap appears between pieces, the bounding box is reporting a height
> larger than the mesh's visible geometry.
>
> To fix: set `BaseMeshStackHeight` to the visual height of the BaseMesh (not the bounding
> box value). For example, if the bounding box reports 135 but there is a 10-unit gap,
> the visible height is 125 — set `BaseMeshStackHeight = 125`.
>
> The same fields exist for `MiddleMesh1StackHeight` and `MiddleMesh2StackHeight`.
> Leave any field at `0` to keep using the bounding box for that piece.

---

### Step 3 — Add WallData to URoomData

1. Open `DA_RoomData_Test`.
2. Set the `WallData` field to `DA_WallData_Test`.
3. Save the asset.

---

### Step 4 — Preview

1. Select your `AMasterRoom` actor in the level.
2. Verify `PreviewPlacement.RoomDataAsset` is still set to `DA_RoomData_Test`.
3. Click **PreviewRoom**.

**What to verify:**

- The Components panel shows one or more entries under `WallISMCPool` — one per
  unique static mesh used by your wall modules (BaseMesh, MiddleMesh1, TopMesh, etc.).
- Wall meshes appear along all four room edges, on every Wall cell (black debug boxes).
- **Corner cells** (purple boxes) show a corner mesh if `CornerMesh` is set on the
  `UWallData` asset. If left empty, corners remain open — both are valid.
- If a corner mesh appears at the wrong orientation, the yaw convention assumes the mesh
  at 0° has its interior corner pointing NE. Rotate your source mesh accordingly and
  re-import if all four corners look the same but wrong.
- Wall meshes face **inward** — the exterior (back) face of each mesh is flush with
  the outer edge of the room boundary. If meshes face outward or are misaligned,
  the pivot is wrong.
- Stack pieces are stacked vertically with no gaps: Base at Z=0, Middle1 above it
  (if assigned), Middle2 above that (if assigned), Top at the peak.
- If a gap appears between any two pieces, use the corresponding stack height override
  field on the module to correct it — no recompile needed.

**Confirming instance count via the Output Log:**

For a 6×6 room with no doors, the perimeter has 16 Wall cells (4 edges × 4 cells each,
corners excluded). With a `CornerMesh` assigned you expect:

```
[MasterRoom_0] [WALL] Wall stacks placed: 16
[MasterRoom_0] [WALL] Corner pieces placed: 4
[MasterRoom_0] [WALL] Door cells skipped: 0
```

If `CornerMesh` is null, `Corner pieces placed: 0` is logged and the line is still present.

> **Note — different wall style assets:** Corner fit and orientation have been validated
> with a single mesh style. When introducing new `UWallData` assets with different geometry,
> re-verify corner alignment in the editor — pivot placement and proportions vary per mesh,
> and the `CornerMesh` on each wall asset is independent.

> **Expected log — null WallData:** If `RoomDataAsset` has no WallData assigned, the
> Output Log will show a Critical warning and skip wall placement. Floor meshes are
> unaffected. This is the same early-exit pattern as the floor step.

---

## 6. Step 5 — Setting Up AFloorManager

`AFloorManager` holds the room layout for one floor. You place it in the level, fill in
the `RoomPlacements` array in the Details panel, and it drives spawning for all rooms on
that floor.

---

### Step 1 — Place AFloorManager in the Level

Drag an `AFloorManager` from the Outliner or Place Actors panel into your level.
Its world location becomes the origin for the entire floor grid — place it at the
intended floor height.

---

### Step 2 — Configure Floor Properties

Select the `AFloorManager` actor and open the **Details** panel.

| Property | Category | Default | What to set |
|---|---|---|---|
| `FloorIndex` | FloorManager\|Layout | 0 | Floor number (0 = ground floor, 1 = second floor, etc.) |
| `RoomHeightCm` | FloorManager\|Layout | 300 | Height of all rooms on this floor in centimeters |
| `FloorGridSize` | FloorManager\|Layout | (20, 20) | Total cell area of the floor (X × Y cells) |
| `OwningBuildingManager` | FloorManager\|Layout | nullptr | Reference to the ABuildingManager actor in this level |

---

### Step 3 — Add Room Placements

Under **FloorManager\|Layout**, find the `RoomPlacements` array and click **+** to add entries.
Each entry is an `FRoomPlacement` with these fields:

| Field | Purpose |
|---|---|
| `RoomClass` | The actor class to spawn (e.g. `AMasterRoom`) |
| `RoomDataAsset` | The `URoomData` asset that defines this room's style |
| `GridOrigin` | Top-left corner of the room footprint in cell coordinates |
| `SizeOverride` | Exact room size in cells. If (0,0), uses `RoomDataAsset->MinRoomSize` |
| `RotationDegrees` | Rotate the room layout: 0, 90, 180, or 270 |
| `Doors` | Array of `FDoorPlacement` entries (see below) |
| `bIsEntranceRoom` | Mark this room as the building entrance on floor 0 (one per building) |

**Adding a door to a room:**

Each `FDoorPlacement` in the `Doors` array describes one doorway:

| Field | Purpose |
|---|---|
| `Face` | Which wall the door is on: North (+Y), South (-Y), East (+X), West (-X) |
| `CellOffset` | Start cell of the door span, 0-based from the left as seen from inside |
| `DoorDataOverride` | Optional per-door style override; leave null to use the room's default DoorData |
| `bIsExteriorDoor` | True for doors that face the outside of the building |

---

### Step 4 — Test ClearLayout

Press the **ClearLayout** button (under **FloorManager\|Actions** in Details). This destroys
all `AMasterRoom` actors spawned by this floor manager. Use it to reset before re-generating.

**GenerateLayout** requires `OwningBuildingManager` to be set (Step 7). To test room generation
before the BuildingManager is configured, use `AMasterRoom::PreviewRoom` directly (Step 3–4).

---

## 7. Step 6 — Using PreviewLayout

`PreviewLayout` is an editor button on `AFloorManager` that draws the floor grid and every
room footprint in the viewport without spawning any actors. It also runs validation checks
and highlights invalid rooms in red.

---

### How to Use

1. Select your `AFloorManager` actor.
2. Press **PreviewLayout** (under **FloorManager\|Actions** in the Details panel).
3. The viewport immediately shows the floor grid and room footprints.
4. Press **ClearPreview** to remove the debug draw before re-running.

`PreviewLayout` clears the previous draw automatically each time it runs — you do not need
to press **ClearPreview** between previews unless you want to inspect the level without
the overlay.

---

### What You See

| Visual | Meaning |
|---|---|
| Gray grid lines | Floor cell grid — outer boundary and interior subdivisions |
| Coordinate labels (every 5 cells) | Cell (X, Y) coordinates in grid space |
| Green box | Room footprint that passed all validation checks |
| Red box | Room footprint that failed one or more checks |
| `[N] WxH` label on each box | Room index and resolved size |

---

### Validation Checks

Each time `PreviewLayout` runs, five checks fire automatically. Failures turn the room
box red and print `[CRITICAL]` messages to the Output Log.

| Check | What it catches |
|---|---|
| **Out of bounds** | Room footprint extends outside `FloorGridSize` |
| **Room overlap** | Two rooms share one or more cells |
| **Orphan door** | Door is on a face with no adjacent room, and `bIsExteriorDoor = false` |
| **Exterior door on non-perimeter face** | Door marked `bIsExteriorDoor = true` but the face is not on the building edge |
| **Misaligned shared door** | Two adjacent rooms both have doors on the shared face, but the start position or width differs |

All green = layout is valid. Press **GenerateLayout** (Step 7) to spawn the rooms.

---

### Visualizer Toggles

The `Visualizer` component on `AFloorManager` has toggle buttons visible in the Details
panel under **Debug\|Toggle**:

| Button | What it toggles |
|---|---|
| `ToggleGrid` | Show/hide grid lines and boundary box |
| `ToggleCoordinates` | Show/hide cell coordinate labels |

Pressing any toggle button automatically re-fires `PreviewLayout` — the viewport updates
immediately without pressing the button again.

---

## 8. Step 7 — Generating a Full Building via ABuildingManager

`ABuildingManager` is the root actor for the whole building. It coordinates all floor
managers and is the sole authority for spawning `AMasterRoom` instances.

---

### Step 1 — Place ABuildingManager

Drag an `ABuildingManager` into the level at the building's world position. All geometry
is relative to this actor's location — the world origin is never used.

---

### Step 2 — Connect Floor Managers

In the **Details** panel of `ABuildingManager`, find the `FloorManagers` array and add
a reference to each `AFloorManager` you placed for this building. Order does not matter
for generation (each floor manager is independent), but keeping them in ascending
`FloorIndex` order is recommended for clarity.

Also set **each** `AFloorManager`'s `OwningBuildingManager` field to point back to this
`ABuildingManager`. Without this reference, `GenerateLayout` on a floor manager will
fail with a `[CRITICAL]` log message and abort.

---

### Step 3 — Set Generation Seed

Under **BuildingManager\|Generation**, set `GenerationSeed` to any integer. This single
value drives all geometry randomization across every room and floor. The same seed always
produces the same building. Change it to get a different layout.

---

### Step 4 — Generate the Building

Press **GenerateBuilding** (under **BuildingManager\|Actions** in Details). This calls
`GenerateLayout` on every connected `AFloorManager`, which in turn spawns `AMasterRoom`
actors for every placement in `RoomPlacements`.

Press **ClearBuilding** to destroy all spawned rooms without resetting the layout.

---

### Door Notes — bUseColumns

By default, `UDoorData::bUseColumns` is **false**. This means the wall cells immediately
flanking the door opening (one cell to each side) receive normal wall modules from
`WallData`, exactly like any other wall cell.

Set `bUseColumns = true` on a `UDoorData` asset when you want to use a dedicated column
mesh at the flanking cells. Behavior summary:

| `bUseColumns` | `ColumnMesh` | Result |
|---|---|---|
| `false` | any | Flanking cells receive wall modules normally |
| `true` | set | `ColumnMesh` placed at flanking cells |
| `true` | null | Flanking cells reserved and left empty |

---

## 9. Tips and Experiments

This section captures non-obvious usage patterns and experiments worth exploring further.

---

### Using MasterRoom SizeOverride for Corridors

**Experiment:** Set `PreviewPlacement.SizeOverride` to `(18, 6)` on a `MasterRoom` actor
and preview it with your existing wall, floor, and ceiling assets.

The result is a narrow corridor — full floor and ceiling coverage, walls on both long sides,
proper corner pieces at all four ends, and door placement working along the long faces. This
can stand in for a hallway immediately without any additional code.

**Practical use right now:**
- Use this to prototype corridor layouts and test how your wall modules tile over long runs.
- Try different widths: `(length, 2)` for a tight 200cm-wide corridor, `(length, 3)` for a
  wider 300cm passage.
- Door placement works normally — add a `FDoorPlacement` to `PreviewPlacement.Doors` to
  cut an opening in one of the long walls.

> **Note for later:** A dedicated `AHallwayRoom` subclass is planned (CLAUDE.md Section 2,
> Step 10). Before that step is implemented, `SizeOverride` is your best tool for corridor
> prototyping. Some `SizeOverride` settings tested now may directly inform the minimum and
> maximum sizes baked into the `AHallwayRoom` subclass.

---

### Column Mesh Alignment at Door-Adjacent Corners

When a 2-cell door with columns is placed immediately next to a corner cell, the column
mesh may not align perfectly with the corner piece because both sides of the door use the
same mesh with the same rotation.

**Workaround for now:** Choose a column mesh that looks acceptable at any rotation, or
position the door at least one wall cell away from the corner so the column does not
directly adjoin the corner piece.

**This will be addressed in a future DoorData update** that adds per-side yaw overrides
for left and right columns. See DEVDOC.md Section 12.2 for the technical plan.

---

## 10. Debug Components Reference

Debug output is split across two components present on both `AMasterRoom` and `AFloorManager`.
Both are visible in the Details panel under the Components list.

---

### DevLog Component — Logging

`DevLog` controls all text output to the Output Log and the on-screen overlay.

| Property | Effect |
|---|---|
| `bEnableDebug` | Master switch — disabling this suppresses all non-critical log output |
| `bEnableScreenLogging` | Shows log messages as on-screen overlays in editor builds |
| `CurrentLogLevel` | Verbosity. `Important` is the default. `Verbose` shows per-cell detail |
| `bEnablePerformanceProfiling` | Logs timing for each generation step in milliseconds |

> `LogCritical` (used for null assets and bad configurations) always fires regardless of
> `bEnableDebug`. You cannot silence critical errors.

---

### Visualizer Component — Editor Visualization

`Visualizer` controls all debug drawing in the viewport. It is an editor-only component —
it does not compile into packaged builds.

**Properties:**

| Property | Effect |
|---|---|
| `bShowGrid` | Grid lines at every cell boundary |
| `bShowCellStates` | Colored box inside each cell showing its type |
| `bShowCoordinates` | `(X,Y)` label at each cell center |
| `GridColor` | Color of grid lines. Default: Green on AMasterRoom, Gray (80,80,80) on AFloorManager |
| `GridLineThickness` | Line thickness. Default: 5 on AMasterRoom, 1 on AFloorManager |

**Cell display colors** (for `bShowCellStates`):

| Color | State | Meaning |
|---|---|---|
| Blue | Empty | Floor cell — no mesh placed yet |
| Red | Occupied | Floor cell with a placed mesh |
| Yellow | Custom | Cell from a ForcedPlacement override (Step 12) |
| Teal | Void | Designer-reserved empty region (Step 12) |
| Black | Wall | Structural wall or corner cell |
| Green | Door | Wall cell within a door span |

**Toggle buttons** (under **Debug\|Toggle** in Details):

| Button | What it toggles |
|---|---|
| `ToggleGrid` | `bShowGrid` |
| `ToggleCoordinates` | `bShowCoordinates` |
| `ToggleCellStates` | `bShowCellStates` |

Any toggle automatically re-fires the preview draw on the owning actor — no manual refresh needed.

---

*Last updated: Steps 5–7 complete — AFloorManager setup and RoomPlacements, PreviewLayout with 5 validation checks and Visualizer toggles, ABuildingManager full generation workflow. bUseColumns added to UDoorData (default false — flanking cells receive wall modules unless opt-in). UDebugLog replaced by DevLog (logging) + Visualizer (editor draw) components. Sections 6/7/8 added for Steps 5/6/7; Sections 9/10 carry forward tips and debug reference.*
