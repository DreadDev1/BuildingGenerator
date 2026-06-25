# Building Manager System — Project Reference

## How to use this file (read first)

This file is the single source of truth for this project's architecture, conventions, and
current status. Before writing or modifying any code, read the relevant section here.

When I give you a task, I will tell you:
- **What** I want done (the goal)
- **Where** it goes (which class/file)
- **Scope** — if I say "just the header" or "stub only", do not write the implementation.
  If I say "implement", write the full body. Do not go beyond the stated scope.

If something is ambiguous or would require a decision not already covered in this file,
stop and ask before writing code. Do not make architectural assumptions.

---

## 1. Project Identity

An open-world, enterable building generation system for Unreal Engine 5 (currently 5.7).
Buildings are placed at arbitrary world locations — never at world origin.
Target: C++ with full Blueprint exposure. Multiplayer baseline: 4-player co-op.
Single-player must also work without code changes.
The project is coded in Rider IDE. C++ is the primary language; Blueprints are for
designer access only, not for core logic.

---

## 2. Class Hierarchy and Responsibilities

```
ABuildingManager          — root actor, placed in world by designer
  └── AFloorManager       — one per floor, manages room layout for that floor
        └── AMasterRoom   — one per room, owns interior geometry
              ├── AHallwayRoom      (C++ subclass)
              ├── AStaircaseRoom    (C++ subclass)
              └── AElevatorRoom     (C++ subclass)
```

### ABuildingManager
- Placed by designer at any world location. Origin = ActorLocation. Never world 0,0,0.
- Sole caller of SpawnActor for AFloorManager and AMasterRoom instances.
- Only spawns under HasAuthority(). Clients never call SpawnActor for these classes.
- Owns the exterior wall mesh (separate UStaticMesh* from UWallData).
- Holds GenerationSeed (int32, Replicated). All geometry derives from this seed.
- After all rooms on a floor generate, detects exterior-facing wall cells and:
  1. Calls AMasterRoom::SuppressExteriorWallISM(Face) on those cells
  2. Places the exterior skin mesh ISMC at those positions
- Validates the vertical connection graph (staircases/elevators) before any generation.

### AFloorManager
- One instance per building floor. Origin offset from BuildingManager by FloorIndex x FloorHeightCm.
- Holds TArray<FRoomPlacement> RoomPlacements — the designer-authored layout.
- Holds RoomHeightCm (int32, EditAnywhere) — all rooms on this floor share this height.
  This value drives wall stack placement and ceiling Z offset for every AMasterRoom on the floor.
- Does NOT spawn actors directly. Calls ABuildingManager::RequestRoomSpawn().
- Provides WITH_EDITOR PreviewLayout and GenerateLayout buttons (CallInEditor).
- Runs validation pass in PreviewLayout — see Section 6 for validation rules.

### AMasterRoom
- Owns all ISMCs: FloorISMC, CeilingISMC, FlavorISMC (fixed, created in constructor). WallISMCPool (TArray) populated dynamically during PlaceWallMeshStacks — one ISMC per unique static mesh encountered.
- Receives FRoomPlacement and GenerationSeed via InitializeRoom() after spawn.
- Runs 8-step GenerateRoomInterior sequence (see Section 5).
- Door placement is fully manual — declared in FRoomPlacement.Doors array.
- GenerationType (Uniform or BSP) comes from URoomData asset.
- bReplicates = true. Geometry is NOT replicated — regenerated locally from seed.
- Interactive elements (doors, triggers) are replicated components/actors.

### AHallwayRoom
- Thin AMasterRoom subclass. Always Uniform generation regardless of URoomData setting.
- Suppresses flavor meshes (bSuppressFlavor = true).
- Typical width: 1-2 cells (100-200cm). Variable length.

### AStaircaseRoom
- Adds ConnectsFromFloor (int32) and ConnectsToFloor (int32).
- BuildingManager validates vertical graph using these before generation.
- Stair mesh treated as a ForcedPlacement entry.

### AElevatorRoom
- Adds ConnectsFromFloor, ConnectsToFloor, ElevatorTriggerExtent (FVector), TravelTime (float).
- In v1: functions as a locked staircase room with vertical travel trigger.

---

## 3. Data Asset Hierarchy

All designer configuration lives in UDataAsset subclasses. One URoomData = one room style.

```
URoomData
  ├── UFloorData        (mesh set for floors)
  ├── UCeilingData      (mesh set for ceilings — same struct as UFloorData)
  ├── UWallData         (4-piece wall stack)
  └── UDoorData         (door mesh + interaction config)
```

### URoomData fields
| Field | Type | Notes |
|---|---|---|
| FloorData | UFloorData* | |
| CeilingData | UCeilingData* | |
| WallData | UWallData* | |
| DoorData | UDoorData* | default door for this style |
| GenerationType | ERoomGenerationType | Uniform or BSP |
| MinRoomSize | FIntPoint | cells |
| MaxRoomSize | FIntPoint | cells |
| FlavorMeshes | TArray\<UStaticMesh*\> | ISM decoration pool |
| FlavorDensity | float | 0.0–1.0 |
| ForcedPlacements | TArray\<FForcedPlacement\> | applied after base generation |

### UFloorData / UCeilingData — asset-level fields
| Field | Type | Notes |
|---|---|---|
| FillMode | EFloorFillMode | `Random`: per-cell weighted pool (default). `Uniform`: one mesh chosen from the weighted pool at generation start; same mesh tiles the entire room. Different seeds → different uniform mesh. |

### UFloorData / UCeilingData — per mesh entry (FFloorMeshEntry)
| Field | Type | Notes |
|---|---|---|
| Mesh | UStaticMesh* | |
| CellsX | int32 | 1=100cm, 2=200cm, 4=400cm |
| CellsY | int32 | |
| AllowRotation | bool | non-square: both orientations always tried; flag only randomizes the starting orientation. Square: permits 0°/90°/180°/270° aesthetic rotation |
| Weight | float | weighted probability. In Uniform mode, weight controls how likely this entry is to be the one mesh chosen for the whole room |

### UWallData fields
| Field | Type | Notes |
|---|---|---|
| WallModules | TArray\<FWallModule\> | module pool — bin-packed along each wall face |
| CornerMesh | UStaticMesh* | single mesh for all four room corners. Pivot: BottomCenter 100×100cm. Rotated 0°/90°/180°/270° per corner. Null = no corner geometry |
| bIsExteriorCapable | bool | signals BuildingManager |

### FWallModule fields (element of UWallData::WallModules)
| Field | Type | Notes |
|---|---|---|
| CellsWide | int32 | face-width in 100cm cells (1, 2, or 4) |
| BaseMesh | UStaticMesh* | required |
| MiddleMesh1 | UStaticMesh* | optional — null = 2-piece stack |
| MiddleMesh2 | UStaticMesh* | optional — only if MiddleMesh1 set |
| TopMesh | UStaticMesh* | required |
| PlacementOffset | FVector | wall-forward space (X=into-room, Y=along-face, Z=up). Corrects for mesh depth ≠ 100cm. Example: mesh 135cm deep → X = -35 |
| BaseMeshStackHeight | float | overrides bounding box height for stacking when > 0. Use when BB ≠ visual top of mesh |
| MiddleMesh1StackHeight | float | same, for MiddleMesh1 |
| MiddleMesh2StackHeight | float | same, for MiddleMesh2 |
| Weight | float | weighted random probability |

### UDoorData fields
| Field | Type | Notes |
|---|---|---|
| DoorMesh | UStaticMesh* | placed spanning DoorWidth cells via MakeWallTransform |
| FrameMesh | UStaticMesh* | optional |
| ColumnMesh | UStaticMesh* | placed at CellOffset-1 and CellOffset+Width (flanking cells) when bUseColumns=true. Null + bUseColumns=true = flanking cells reserved but left empty |
| DoorWidth | EDoorWidth | TwoCell (200cm) or FourCell (400cm). Controls door span and column positions |
| bUseColumns | bool | Default false. When false, flanking cells are not reserved and receive normal wall modules from WallData. When true, flanking cells are reserved for ColumnMesh (cell left empty if ColumnMesh is null) |
| DoorPlacementOffset | FVector | wall-forward space (X=into-room, Y=along-face, Z=up). Corrects for door mesh depth ≠ wall face. Same convention as FWallModule::PlacementOffset |
| ColumnPlacementOffset | FVector | same convention, applied to ColumnMesh only. Set independently from DoorPlacementOffset |
| CollisionBoxExtent | FVector | trigger box size |
| InteractionType | EDoorInteraction | AutoTrigger or RequiresInput |
| bIsExteriorDoor | bool | |
| OpenSound | USoundBase* | |
| CloseSound | USoundBase* | |

---

## 4. Key Structs

### FRoomPlacement
```cpp
USTRUCT(BlueprintType)
struct FRoomPlacement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AMasterRoom> RoomClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    URoomData* RoomDataAsset;

    // Position in FloorManager local grid (cell units, not world units)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint GridOrigin;

    // If (0,0), uses RoomData Min/MaxRoomSize
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint SizeOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RotationDegrees = 0;

    // All doors for this placement — fully manual, no auto-placement
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FDoorPlacement> Doors;

    // Marks this room as the building entrance on floor 0.
    // ABuildingManager reads this to locate where to cut the exterior skin opening.
    // Only one placement per building should have this true.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsEntranceRoom = false;
};
```

### FDoorPlacement
```cpp
UENUM(BlueprintType)
enum class ERoomFace : uint8
{
    North   UMETA(DisplayName = "North  (+Y)"),
    South   UMETA(DisplayName = "South  (-Y)"),
    East    UMETA(DisplayName = "East   (+X)"),
    West    UMETA(DisplayName = "West   (-X)")
};

USTRUCT(BlueprintType)
struct FDoorPlacement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERoomFace Face = ERoomFace::North;

    // Start cell of the door span, 0-based from the leftmost cell of that face as seen
    // from inside the room. The span covers [CellOffset, CellOffset + DoorWidth) cells.
    // Column cells (if ColumnMesh set) occupy CellOffset-1 and CellOffset+DoorWidth.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CellOffset = 0;

    // Overrides URoomData::DoorData for this specific door if set
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UDoorData* DoorDataOverride = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsExteriorDoor = false;
};
```

### FForcedPlacement
```cpp
USTRUCT(BlueprintType)
struct FForcedPlacement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint CellPosition;   // local to room grid

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RotationYaw = 0.f;
};
```

---

## 5. AMasterRoom Generation Sequence

GenerateRoomInterior_Implementation() runs these 8 steps in order.
Subclasses call Super to run the full sequence, then add subclass logic after.
AHallwayRoom skips step 7 (flavor). AStaircaseRoom adds stair mesh in step 6.

1. **BuildGrid()** — allocate FIntPoint cell array sized to resolved RoomGridSize
2. **ClassifyCells()** — edges → WallCell, corners → CornerCell, interior → FloorCell
3. **PlaceFloorMeshes()** — fill FloorCells using URoomData::FloorData weighted random (FRandomStream seeded from GenerationSeed)
4. **PlaceCeilingMeshes()** — mirror floor pass at RoomHeight Z offset
5. **PlaceWallMeshStacks()** — per face, per position: column cell (CellOffset±Width) → place ColumnMesh or leave empty (always suppresses wall module); door start → place DoorMesh spanning DoorWidth cells; else → bin-pack UWallData modules (available run stops at column and door span positions); then place CornerMesh at all four corners
6. **ApplyForcedPlacements()** — override cells from URoomData::ForcedPlacements
7. **PlaceFlavorMeshes()** — fill remaining interior cells from FlavorMeshes pool at FlavorDensity
8. **SpawnDoorTriggers()** — for each door in ActivePlacement.Doors, create replicated UBoxComponent trigger

---

## 6. Room Generation — Uniform vs BSP

Set per room style in URoomData::GenerationType.

**Uniform**: weighted random fill scanning left-to-right, top-to-bottom. For each unclaimed
cell, builds a weighted candidate pool from the full mesh array and tries candidates in
weighted-random order until one fits or the pool is exhausted. For non-square meshes, both
footprint orientations (CellsX×CellsY and CellsY×CellsX) are always tried — AllowRotation
only randomizes which is attempted first. If no pool candidate fits, falls back to a 1×1
entry if one exists, otherwise skips the cell. AHallwayRoom always uses Uniform regardless
of URoomData setting.

**BSP**: recursively subdivides FloorCell bounding rect. Splits on random axis until
nodes fall below MinSubdivisionSize. Fills each leaf with one weighted random mesh.
Used for interior floor variety — not for dungeon layout. Ported from existing
Blueprint dungeon generator project.

Both types run ForcedPlacements as a post-pass override.

---

## 7. Multiplayer Replication Rules — NEVER VIOLATE THESE

| Element | Rule |
|---|---|
| ABuildingManager | bReplicates=true, GenerationSeed is UPROPERTY(Replicated) |
| AFloorManager | bReplicates=true, RoomPlacements is UPROPERTY(Replicated) |
| AMasterRoom existence | Server spawns, replicates automatically to clients |
| ISM geometry (walls/floors/ceilings/flavor) | NEVER replicated. Client regenerates locally from seed |
| Door actors / state | bReplicates=true, spawned by BuildingManager under HasAuthority() |
| Door open/close state | UPROPERTY(ReplicatedUsing=OnRep_DoorState) |
| Trigger overlaps | Server RPC from client → Multicast result |

**Do not individually replicate ISM instance transforms under any circumstances.**
Late joiners receive AMasterRoom via actor replication then regenerate geometry locally.

### Door RPC pattern
```cpp
UFUNCTION(Server, Reliable)
void Server_RequestDoorInteraction(int32 DoorIndex);

UFUNCTION(NetMulticast, Reliable)
void Multicast_SetDoorState(int32 DoorIndex, bool bOpen);

UPROPERTY(ReplicatedUsing = OnRep_DoorState)
TArray<bool> DoorStates;
```

### SpawnActor pattern — always use this, never deviate
```cpp
AMasterRoom* ABuildingManager::RequestRoomSpawn(
    TSubclassOf<AMasterRoom> RoomClass,
    const FTransform& SpawnTransform,
    const FRoomPlacement& Placement)
{
    if (!HasAuthority()) return nullptr;

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AMasterRoom* Room = GetWorld()->SpawnActor<AMasterRoom>(
        RoomClass, SpawnTransform, Params);

    if (Room)
    {
        Room->InitializeRoom(Placement, GenerationSeed);
    }
    return Room;
}
```

---

## 8. Mesh and Pivot Conventions — HARD REQUIREMENTS

| Mesh type | Pivot position |
|---|---|
| Floor | Bottom-Center |
| Ceiling | Bottom-Center (placed at RoomHeight Z offset) |
| All wall pieces (Base/Middle1/Middle2/Top) | BottomBackCenter |
| Corner piece | Bottom-Center (100×100cm footprint, rotated per corner) |
| Door mesh | BottomCenter of frame opening |
| Stair mesh | Bottom of lowest step, front-center |
| Flavor mesh | Bottom-Center |

**Permitted sizes — 100cm grid discipline:**
- Floor/Ceiling: 100x100, 200x100, 100x200, 200x200, 400x200, 200x400, 400x400
- Wall Y-axis: 100cm, 200cm, 400cm
- Hallway width: 100cm (1 cell) or 200cm (2 cells) only
- Mixed sizes (e.g. 300cm) are NOT supported in v1

All mesh pivot points are set in Blender before import. This is a designer responsibility.
BottomBackCenter on wall pieces enables 90° rotation for all cardinal directions
without any offset calculation in code.

---

## 9. WITH_EDITOR Conventions

All debug visualization is editor-only — nothing in these blocks ships in a packaged build.

### WITH_EDITOR vs WITH_EDITORONLY_DATA — pick the right macro

UHT rejects a reflected `UPROPERTY` wrapped in `#if WITH_EDITOR` with the error
*"Properties should not be wrapped by WITH_EDITOR, use WITH_EDITORONLY_DATA instead."*
The two macros are not interchangeable:

- **`WITH_EDITORONLY_DATA`** — guards editor-only *data*: any `UPROPERTY`, and an entire
  `UCLASS`/`USTRUCT`/`UENUM` whose reflected members should not exist in a packaged build.
  Use this around editor-only fields and around editor-only reflected types.
- **`WITH_EDITOR`** — guards editor-only *code*: function bodies, `UFUNCTION(CallInEditor)`
  logic, and other non-data logic that only runs in the editor.

Rules of thumb:
- A reflected member (`UPROPERTY`) → always `WITH_EDITORONLY_DATA`, never `WITH_EDITOR`.
- An editor-only class that owns reflected properties (e.g. `UBGVisualizer`) → wrap the
  whole class in `WITH_EDITORONLY_DATA`. Its `UFUNCTION`s come along inside that block.
- Match the `.cpp` guard to the header guard exactly. If the class definition compiles in
  under `WITH_EDITORONLY_DATA` but its implementation is under `WITH_EDITOR`, a config where
  the two macros diverge yields unresolved externals. `UBGVisualizer`/`.cpp` both use
  `WITH_EDITORONLY_DATA`.
- Any `.cpp` code that reads an editor-only property must itself be guarded
  (`WITH_EDITORONLY_DATA` or `WITH_EDITOR`) or the packaged build won't compile.

Examples in this project: `AFloorManager::Visualizer` and `AMasterRoom::Visualizer`
(`WITH_EDITORONLY_DATA`), `AMasterRoom::PreviewRoomHeightCm` (`WITH_EDITORONLY_DATA`),
the entire `UBGVisualizer` component (`WITH_EDITORONLY_DATA`), and the CallInEditor button
functions below (`WITH_EDITOR` is acceptable for those since they are code, not data).

AFloorManager exposes two CallInEditor buttons:
- PreviewLayout — runs validation, draws debug grid, no actors spawned
- GenerateLayout — commits full generation, spawns actors and places meshes
- ClearLayout — destroys all spawned room actors for this floor

Debug draw uses DrawDebugBox and DrawDebugString.
TextRenderer coordinate display from the previous grid visualizer project
can be ported directly for cell coordinate labels.

### PreviewLayout validation checks
- Room overlap: two FRoomPlacement footprints intersect
- Out-of-bounds: footprint extends beyond FloorGridSize
- Misaligned shared doors: adjacent rooms have doors on shared face at different CellOffset
- Orphan door: door declared with no adjacent room or hallway
- Exterior door on interior face: bIsExteriorDoor=true but face is not on building perimeter
- Staircase without matching landing on ConnectsToFloor

---

## 10. UPROPERTY / UFUNCTION Macro Conventions

```cpp
// Standard property exposure
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room|Layout")
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room|Meshes")
UPROPERTY(Replicated)
UPROPERTY(ReplicatedUsing = OnRep_FunctionName)

// Function exposure
UFUNCTION(BlueprintCallable, Category = "Room|Generation")
UFUNCTION(BlueprintNativeEvent, Category = "Room|Generation")   // C++ default + BP override
UFUNCTION(BlueprintImplementableEvent, Category = "Room|Events") // BP override only
UFUNCTION(CallInEditor, Category = "Debug")                     // editor button
UFUNCTION(Server, Reliable)
UFUNCTION(NetMulticast, Reliable)
```

Category strings follow "ClassName|Subcategory" format consistently.

---

## 11. Build.cs Dependencies

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "NetCore",
});

// Add these only if GeometryScript is needed for exterior mesh generation
// "GeometryCore", "GeometryScriptingCore", "ProceduralMeshComponent"

if (Target.bBuildEditor)
{
    PrivateDependencyModuleNames.AddRange(new string[]
    {
        "UnrealEd",
    });
}
```

---

## 12. Open Questions (resolve before implementing affected systems)

- **RoomHeight**: ~~fixed per building (BuildingManager), per floor (FloorManager),
  or per room style (URoomData/MasterRoom)?~~
  **DECIDED: Per floor.** AFloorManager::RoomHeightCm is authoritative. All AMasterRoom
  instances on a floor receive it via InitializeRoom(). Staircase geometry bridges the
  delta between adjacent floor RoomHeightCm values.

- **Exterior door**: ~~does BuildingManager place an exterior-facing door mesh in addition
  to the interior door, or does the interior door serve both directions?~~
  **DECIDED: Interior door serves both.** One room per building is designated as the
  entrance (FRoomPlacement::bIsEntranceRoom = true, floor 0 only). ABuildingManager reads
  that room's exterior door position and cuts an opening in the exterior skin ISMC there.
  No separate exterior door mesh is placed.

- **Wall module required fields**: BaseMesh and TopMesh are required on every FWallModule.
  Generation skips any module missing either. MiddleMesh1 and MiddleMesh2 are optional —
  MiddleMesh1 alone adds one middle layer; both add two. This gives designers direct control
  over room height without code changes. **CONFIRMED during Step 4 testing.**

- **Wall ISMC architecture**: Fixed per-layer ISMCs (WallBaseISMC etc.) were replaced with
  a dynamic `WallISMCPool: TArray<UISMC*>` — one ISMC per unique static mesh. This was
  necessary because different wall modules have different static meshes per layer.
  **DECIDED during Step 4.**

- **World Partition streaming**: ~~one cell per building or proximity-based?~~
  **DECIDED: One ABuildingManager = one streaming cell.** The entire building (all floors,
  all rooms) loads and unloads as a single unit. Players outside that cell's load radius
  have no awareness of the building.

- **Ceiling Z source**: ~~UCeilingData had a per-asset RoomHeightCm field.~~
  **DECIDED: Removed.** Ceiling Z comes from `AMasterRoom::RoomHeightCm`, which is set by
  `InitializeRoom()` from `AFloorManager::RoomHeightCm`. For editor preview, `AMasterRoom`
  exposes `PreviewRoomHeightCm` (`WITH_EDITORONLY_DATA`, in the "MasterRoom|Debug" Details
  category). Designers set this to match the visual top of their wall stack.

- **Existing UDataAsset files**: ~~confirm whether existing project data assets match the
  schema in Section 3.~~
  **MOOT: Data asset classes were created from scratch in Step 1.** No legacy assets to
  reconcile.

- **Door width encoding**: Width lives on UDoorData (EDoorWidth enum, TwoCell/FourCell),
  not on FDoorPlacement. FDoorPlacement.CellOffset is the start of the span.
  Multiple doors on one room are handled entirely by the FDoorPlacement array — no
  separate bMultipleDoors flag is needed. **DECIDED during door implementation.**

- **Column mesh behavior**: Controlled by `UDoorData::bUseColumns` (default false).
  When false, flanking cells (CellOffset-1 and CellOffset+DoorWidth) are not reserved —
  the bin-pack run treats them as normal wall cells. When true, flanking cells are reserved:
  ColumnMesh is placed if set, cell is left empty if null.
  Default is false so a door-only setup (no ColumnMesh) fills flanking gaps with wall modules
  automatically. **UPDATED: bUseColumns added after original always-reserve behavior caused
  empty gaps when ColumnMesh was null.**

- **Column per-side yaw**: Left and right column meshes may need independent yaw overrides
  to align with corner pieces on asymmetric wall styles. Deferred — add LeftColumnYaw /
  RightColumnYaw fields to UDoorData when new wall style assets expose the misalignment.
  See DEVDOC.md Section 9.2. **DEFERRED — not blocking any current step.**

---

## 13. Implementation Status

Track progress here. Update this section as steps complete.

- [x] **Step 1** — UDataAsset schema: URoomData, UFloorData, UCeilingData, UWallData, UDoorData
- [x] **Step 2** — AMasterRoom grid + cell classification (no mesh placement, debug draw only)
- [x] **Step 3/3b** — Uniform floor + ceiling fill: exhaustive pool-based weighted random, multi-mesh support, both footprint orientations always tried for non-square meshes (AllowRotation only controls start order), 1×1 fallback recommended; EFloorFillMode (Random/Uniform) on UFloorData/UCeilingData; UCeilingData::RoomHeightCm removed (ceiling Z from AMasterRoom::RoomHeightCm); PreviewRoomHeightCm added for editor preview
- [x] **Step 4** — Wall mesh stack placement + corner pieces + door mesh placement: bin-packed wall modules (BottomBackCenter pivot), CornerMesh on UWallData (BottomCenter, 0°/90°/180°/270° per corner, shares WallISMCPool); EDoorWidth (TwoCell/FourCell) on UDoorData; DoorMesh placed spanning width via MakeWallTransform; ColumnMesh at flanking cells (always suppresses wall module, mesh placed only when set); DoorPlacementOffset + ColumnPlacementOffset; GetEffectiveDoorData + GetDoorWidthCells + FindColumnForPos helpers
- [x] **Step 5** — FRoomPlacement + FDoorPlacement structs + AFloorManager Details panel: AFloorManager class with RoomPlacements, RoomHeightCm, FloorGridSize, FloorIndex; SpawnedRooms runtime array; WITH_EDITOR CallInEditor stubs for PreviewLayout/GenerateLayout/ClearLayout (ClearLayout fully implemented)
- [x] **Step 6** — WITH_EDITOR PreviewLayout debug visualization: draws full floor grid (outer boundary, interior lines, coordinate labels every 5 cells); draws each room footprint green/red; runs 5 validation checks (out of bounds, room overlap, orphan door, exterior door on non-perimeter face, misaligned shared doors); staircase landing check deferred to Step 10; persistent debug lines (Duration=-1), ClearPreview button flushes via FlushPersistentDebugLines + FlushDebugStrings
- [x] **Step 7** — ABuildingManager SpawnActor authority + AFloorManager spawning: ABuildingManager with GenerationSeed (Replicated), RequestRoomSpawn (HasAuthority guard, AlwaysSpawn), GetLifetimeReplicatedProps; GenerateBuilding/ClearBuilding editor buttons iterate FloorManagers; AFloorManager::GenerateLayout calls RequestRoomSpawn per placement, stores in SpawnedRooms; OwningBuildingManager reference on AFloorManager (set by designer or by BuildingManager at runtime)
- [ ] **Step 8** — Multiplayer replication: seed replication, client-side regen, door RPC
- [ ] **Step 9** — BSP fill pass (port from existing Blueprint dungeon generator)
- [ ] **Step 10** — AHallwayRoom, AStaircaseRoom subclasses
- [ ] **Step 11** — Exterior wall separation (suppress interior ISM on perimeter faces)
- [ ] **Step 12** — ForcedPlacements, FlavorMeshes, weighted random fill
- [ ] **Step 13** — AElevatorRoom

---

## 14. How to Prompt Claude Code Effectively

This section is for you — the developer — not for Claude Code itself.

### The most important rule
**One task at a time, explicit scope.** The more specific the prompt, the better the
output. Vague prompts produce code that compiles but doesn't fit the architecture.

### Prompt templates that work well

**For a new file:**
```
Create the header file for AMasterRoom at Source/[Project]/Public/MasterRoom.h.
Include only the class declaration, UPROPERTY fields from Section 3 of CLAUDE.md,
and the function signatures from Section 5. No implementations yet.
Use the UPROPERTY macros from Section 10. Do not add anything not listed.
```

**For an implementation:**
```
Implement AMasterRoom::BuildGrid() in MasterRoom.cpp.
This is step 1 of the generation sequence in CLAUDE.md Section 5.
It allocates the cell array sized to RoomGridSize. No mesh placement.
The cell type enum is ECellType with values Floor, Wall, Corner.
Match the style of the existing [filename] file already in the project.
```

**For understanding existing code:**
```
Explain what BenPyton's ProceduralDungeon plugin does in RoomData.h,
specifically how it declares door positions and room bounds.
I want to understand it conceptually — do not suggest changes to my project yet.
```

**For debugging:**
```
This function is producing incorrect wall cell classification at room corners.
Here is the current code: [paste]
Here is what it should do: [describe from CLAUDE.md Section 5 step 2]
Here is the symptom: [describe]
Diagnose only — do not rewrite the function unless I ask.
```

### Scope control phrases
- "header only" — write only the .h file
- "stub only" — write the function signature and an empty body or TODO
- "implement only X" — write one specific function, nothing else
- "diagnose only" — explain the problem, do not fix it yet
- "do not modify [filename]" — explicitly protect files you don't want touched
- "match the style of [filename]" — enforce consistency with existing code

### When to reference CLAUDE.md explicitly
For any task involving architecture, replication, mesh conventions, or struct
definitions, include: "Follow the conventions in CLAUDE.md Section [N]."
Claude Code reads CLAUDE.md at session start but a direct reference during a
complex task keeps the relevant section front of mind.

### Updating CLAUDE.md
After any significant decision, architectural change, or completed implementation step:
- Mark the step done in Section 13
- Update the relevant section if the implementation differed from the plan
- Add any new conventions that emerged during implementation
- Record resolved open questions in Section 12 with the decision made

Keeping CLAUDE.md current is what makes the next session productive.
A stale CLAUDE.md is worse than no CLAUDE.md.

update project

