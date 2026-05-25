//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/GridDungeonBuilder.h"

#include "Builders/Grid/GridDungeonBuilderLib.h"
#include "Builders/Grid/GridDungeonConfig.h"
#include "Builders/Grid/GridDungeonModel.h"
#include "Builders/Grid/GridDungeonModelHelper.h"
#include "Builders/Grid/GridDungeonQuery.h"
#include "Builders/Grid/GridDungeonSelectorLogic.h"
#include "Builders/Grid/GridDungeonToolData.h"
#include "Builders/Grid/GridDungeonTransformLogic.h"
#include "Builders/Grid/Volumes/GridDungeonPlatformVolume.h"
#include "Core/Dungeon.h"
#include "Core/Markers/DungeonMarkerNames.h"
#include "Core/Markers/DungeonMarkerVisualization.h"
#include "Core/Utils/DungeonModelHelper.h"
#include "Core/Utils/MathUtils.h"
#include "Core/Utils/Triangulator/Impl/DelauneyTriangleGenerator.h"
#include "Core/Volumes/DungeonMirrorVolume.h"
#include "Core/Volumes/DungeonNegationVolume.h"
#include "Frameworks/MarkerGenerator/Impl/Grid/MarkerGenGridProcessor.h"

#include "Containers/Queue.h"
#include "DrawDebugHelpers.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(GridDungeonBuilderLog);


void UGridDungeonBuilder::InitializeState() {
    Super::InitializeState();
    
    GridModel = Cast<UGridDungeonModel>(DungeonModel);
    GridConfig = Cast<UGridDungeonConfig>(DungeonConfig);
    GridQuery = Cast<UGridDungeonQuery>(DungeonQuery);
}

void UGridDungeonBuilder::BuildDungeonImpl(UWorld* World) {
    if (!GridModel) {
        UE_LOG(GridDungeonBuilderLog, Error, TEXT("Invalid dungeon model provided to the grid builder"));
        return;
    }

    if (!GridConfig) {
        UE_LOG(GridDungeonBuilderLog, Error, TEXT("Invalid dungeon gridConfig provided to the grid builder"));
        return;
    }

    if (GridModel) {
        GridModel->GridCellInfoLookup.Reset();
        GridModel->CellStairsLookup.Reset();
    }
    
    GaussRandom.Init(DungeonConfig->Seed);
    GridToMeshScale = GridConfig->GridCellSize;
    DungeonTransform = GetDungeonTransform();

    BuildDungeonCells();

    ApplyCellOffset();

    // Apply negation volumes by removing procedural geometry that lie within it
    ApplyNegationVolumes(World);

    // Add cells defined by platform volumes in the world
    AddUserDefinedPlatforms(World);

    // Create the room connections
    GenerateRoomConnections();

    // Connect the rooms by converting cells between the rooms into corridors.  Also adds new corridor cells if needed for the connection
    ConnectCorridors();

    // Apply negation volumes by removing procedural geometry that lie within it
    ApplyNegationVolumes(World);

    // Build a lookup of adjacent tiles for later use with height and stair creation
    GenerateAdjacencyLookup();

    check(GridConfig->StairGenerator);
    GridConfig->StairGenerator->Generate(GridModel, this);
    
    RemoveRedundantDoors();

    // Groups the dungeon into various clusters.  This is used to apply different themes on various parts of the dungeon, if specified, to give variations 
    CreateLayoutClusterGroups();

    GridModel->BuildState = DungeonModelBuildState::Complete;

    // Save the state for serialization
    SerializeModel();
}

void UGridDungeonBuilder::BuildDungeonCells() {
    if (GridConfig->bFastCellDistribution) {
        BuildDungeonCellsDistributed();
    }
    else {
        BuildDungeonCellsIterative();
    }
}

void UGridDungeonBuilder::BuildDungeonCellsIterative() {
    BuildCells();
    GridModel->BuildState = DungeonModelBuildState::Separation;
    while (GridModel->BuildState == DungeonModelBuildState::Separation) {
        Seperate();
    }
}

void UGridDungeonBuilder::BuildDungeonCellsDistributed() {
    int32 sx = -GridConfig->DungeonWidth * 0.5f / GridConfig->GridCellSize.X;
    int32 ex = -sx;

    int32 sy = -GridConfig->DungeonLength * 0.5f / GridConfig->GridCellSize.Y;
    int32 ey = -sy;

    int FitnessTries = 10;
    TSet<FIntVector> Occupied;
    TArray<FGridDungeonCell> cells;
    for (int ix = sx; ix <= ex; ix++) {
        for (int iy = sy; iy <= ey; iy++) {
            for (int t = 0; t < FitnessTries; t++) {
                int offsetX, offsetY;
                {
                    FIntVector offset = GenerateCellSize() / 2.0f;
                    offsetX = FMath::RoundToInt(offset.X * Random.FRand());
                    offsetY = FMath::RoundToInt(offset.Y * Random.FRand());
                }

                int x = ix + offsetX;
                int y = iy + offsetY;
                FRectangle bounds;
                bounds.Size = GenerateCellSize();
                bounds.Location = FIntVector(x, y, 0);
                if (CanFitDistributionCell(Occupied, bounds)) {
                    FGridDungeonCell cell = BuildCell(bounds);
                    cells.Add(cell);
                    SetDistributionCellOccupied(Occupied, bounds);
                    break;
                }
            }
        }
    }

    GridModel->Cells = cells;
    GridModel->BuildCellLookup();
    GridModel->BuildState = DungeonModelBuildState::Triangulation;
}

bool UGridDungeonBuilder::CanFitDistributionCell(const TSet<FIntVector>& Occupancy, const FRectangle& bounds) {
    for (int x = bounds.X(); x < bounds.X() + bounds.Size.X; x++) {
        for (int y = bounds.Y(); y < bounds.Y() + bounds.Size.Y; y++) {
            if (Occupancy.Contains(FIntVector(x, y, 0))) {
                return false;
            }
        }
    }
    // No cells within the bounds are occupied
    return true;
}

void UGridDungeonBuilder::SetDistributionCellOccupied(TSet<FIntVector>& Occupancy, const FRectangle& bounds) {
    for (int x = bounds.X(); x < bounds.X() + bounds.Size.X; x++) {
        for (int y = bounds.Y(); y < bounds.Y() + bounds.Size.Y; y++) {
            Occupancy.Add(FIntVector(x, y, 0));
        }
    }
}

void UGridDungeonBuilder::GenerateRoomConnections() {
    // Connect the rooms with delaunay triangulation to have nice evenly spaced triangles
    TriangulateRooms();

    // Build a minimum spanning tree of the above triangulation, to avoid having lots of loops
    BuildMinimumSpanningTree();
}

void UGridDungeonBuilder::MirrorDungeonWithVolume(ADungeonMirrorVolume* MirrorVolume) {
    FVector MirrorAxis(1, 0, 0);
    FVector MirrorNormal(0, 1, 0);
    MirrorAxis = MirrorVolume->GetTransform().GetRotation() * MirrorAxis;
    MirrorNormal = MirrorVolume->GetTransform().GetRotation() * MirrorNormal;
    FVector MirrorLocation = MirrorVolume->GetTransform().GetTranslation();

    TArray<FGridDungeonCell> CellsToRemove;
    TArray<FGridDungeonCell> CellsToAdd;

    for (FGridDungeonCell& Cell : GridModel->Cells) {
        float Width = Cell.Bounds.Size.X;
        float Length = Cell.Bounds.Size.Y;
        FVector CellLocation(Cell.Bounds.Location.X, Cell.Bounds.Location.Y, 0);
        const FVector Tips[4] = {
            CellLocation,
            CellLocation + FVector(Width, 0, 0),
            CellLocation + FVector(Width, Length, 0),
            CellLocation + FVector(0, Length, 0)
        };

        int32 TipsReflected = 0;
        FVector GridSize = GridConfig->GridCellSize;
        TArray<FVector> ReflectedTips;
        for (int i = 0; i < 4; i++) {
            const FVector Tip = Tips[i] * GridSize;

            // Check if this point lies in the reflective side
            FVector DirectionToTip = Tip - MirrorLocation;
            FVector TipUp = FVector::CrossProduct(DirectionToTip, MirrorAxis);
            if (TipUp.Z > 0) {
                TipsReflected++;
                ReflectedTips.Add(Tip);
                FVector ReflectedTip = FMath::GetReflectionVector(DirectionToTip, MirrorNormal);
                ReflectedTips.Add(ReflectedTip);
            }
        }

        if (TipsReflected == 0) {
            // This cell lies totally outside the reflective volume
            CellsToRemove.Add(Cell);
        }
        else if (TipsReflected == 4) {
            CellLocation = FVector(Cell.Bounds.Location.X, Cell.Bounds.Location.Y, Cell.Bounds.Location.Z);
            // This cell lies totally inside the reflective volume
            const FVector Center = (CellLocation + FVector(Width, Length, 0) / 2.0f) * GridSize;
            float DistanceToMirror = FVector::DotProduct(Center - MirrorLocation, MirrorNormal);
            FVector MirroredCenter = Center - MirrorNormal * (DistanceToMirror * 2.0f);

            FGridDungeonCell MirroredCell;
            MirroredCell.Id = GetNextCellId();
            MirroredCell.UserDefined = false;
            FRectangle bounds;
            bounds.Size = Cell.Bounds.Size;
            FVector MirroredCenterGrid = MirroredCenter / GridSize;
            bounds.Location.X = FMath::RoundToInt(MirroredCenterGrid.X - bounds.Size.X / 2.0f);
            bounds.Location.Y = FMath::RoundToInt(MirroredCenterGrid.Y - bounds.Size.Y / 2.0f);
            bounds.Location.Z = FMath::RoundToInt(MirroredCenterGrid.Z);

            MirroredCell.Bounds = bounds;
            MirroredCell.CellType = Cell.CellType;
            CellsToAdd.Add(MirroredCell);
        }
        else {
            if (ReflectedTips.Num() > 0) {
                // This cell partially lies within the reflective volume
                // Extend the bounds
                float MinX = ReflectedTips[0].X;
                float MaxX = ReflectedTips[0].X;
                float MinY = ReflectedTips[0].Y;
                float MaxY = ReflectedTips[0].Y;

                for (const FVector& ReflectedTip : ReflectedTips) {
                    MinX = FMath::Min(MinX, ReflectedTip.X);
                    MinY = FMath::Min(MinY, ReflectedTip.Y);

                    MaxX = FMath::Max(MaxX, ReflectedTip.X);
                    MaxY = FMath::Max(MaxY, ReflectedTip.Y);
                }

                FRectangle bounds;
                bounds.Size.X = FMath::RoundToInt((MaxX - MinX) / GridSize.X);
                bounds.Size.Y = FMath::RoundToInt((MaxY - MinY) / GridSize.Y);
                bounds.Location.X = FMath::RoundToInt(MinX / GridSize.X);
                bounds.Location.Y = FMath::RoundToInt(MinY / GridSize.Y);
                Cell.Bounds = bounds;
            }

        }

    }
    for (const FGridDungeonCell& CellToRemove : CellsToRemove) {
        GridModel->Cells.Remove(CellToRemove);
    }
    for (const FGridDungeonCell& CellToAdd : CellsToAdd) {
        GridModel->Cells.Add(CellToAdd);
    }

    // TODO: reflect doors and stair gridModel

    // Reflect the low level markers
    TArray<FDungeonMarkerInstance> MarkersToAdd;
    TArray<FDungeonMarkerInstance> MarkersToRemove;

    TArray<FDungeonMarkerInstance>& WorldMarkers = DungeonModel->WorldMarkers;
    for (const FDungeonMarkerInstance& Marker : WorldMarkers) {
        // Check if this point lies in the reflective side
        FVector DirectionToMarker = Marker.Transform.GetLocation() - MirrorLocation;
        FVector MarkerUp = FVector::CrossProduct(DirectionToMarker, MirrorAxis);
        if (MarkerUp.Z > 0) {
            // Reflect this tip
            FVector MarkerLocation = Marker.Transform.GetLocation();
            float DistanceToMirror = FVector::DotProduct(MarkerLocation - MirrorLocation, MirrorNormal);
            FVector MirroredLocation = MarkerLocation - MirrorNormal * (DistanceToMirror * 2.0f);
            FQuat MirroredRotation = Marker.Transform.GetRotation(); // TODO: Rotate by 180
            {
                FRotator Rotator(MirroredRotation);
                FVector ReflectedVector = FMath::GetReflectionVector(Rotator.Vector(), MirrorNormal);
                MirroredRotation = FQuat(ReflectedVector.Rotation());
            }

            FDungeonMarkerInstance ReflectedMarker = Marker;
            ReflectedMarker.Id = ++_MarkerIdCounter;
            ReflectedMarker.Transform.SetLocation(MirroredLocation);
            ReflectedMarker.Transform.SetRotation(MirroredRotation);

            MarkersToAdd.Add(ReflectedMarker);
        }
        else {
            // Lies outside. Discard
            MarkersToRemove.Add(Marker);
        }
    }

    // Remove discarded markers
    for (const FDungeonMarkerInstance& Marker : MarkersToRemove) {
        WorldMarkers.Remove(Marker);
    }

    // Add reflected markers
    for (const FDungeonMarkerInstance& Marker : MarkersToAdd) {
        WorldMarkers.Add(Marker);
    }
}

bool UGridDungeonBuilder::PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics,
                                                const FDungeonMarkerInstance& InMarker) {
    for (UDungeonSelectorLogic* SelectionLogic : SelectionLogics) {
        UGridDungeonSelectorLogic* GridSelectionLogic = Cast<UGridDungeonSelectorLogic>(SelectionLogic);
        if (!GridSelectionLogic) {
            UE_LOG(GridDungeonBuilderLog, Warning,
                   TEXT("Invalid selection logic specified.  GridDungeonSelectorLogic expected"));
            return false;
        }

        FGridDungeonCell InvalidCell;
        // Perform blueprint based selection logic
        FVector location = InMarker.Transform.GetLocation();

        int32 gridX = FMath::FloorToInt(location.X / GridToMeshScale.X);
        int32 gridY = FMath::FloorToInt(location.Y / GridToMeshScale.Y);
        FGridDungeonCellInfo cellInfo = GridModel->GetGridCellLookup(gridX, gridY);
        FGridDungeonCell* cell = GridModel->GetCell(cellInfo.CellId);
        FGridDungeonCell& cellRef = cell ? *cell : InvalidCell;
        bool selected = GridSelectionLogic->SelectNode(GridModel, GridConfig, this, GridQuery, cellRef, Random, gridX,
                                                       gridY, InMarker.Transform);
        if (!selected) {
            return false;
        }
    }
    return true;
}

FTransform UGridDungeonBuilder::PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics,
                                                      const FDungeonMarkerInstance& InMarker) {
    FTransform result = FTransform::Identity;

    for (UDungeonTransformLogic* TransformLogic : TransformLogics) {
        UGridDungeonTransformLogic* GridTransformLogic = Cast<UGridDungeonTransformLogic>(TransformLogic);
        if (!GridTransformLogic) {
            UE_LOG(GridDungeonBuilderLog, Warning,
                   TEXT("Invalid transform logic specified.  GridDungeonTransformLogic expected"));
            continue;
        }

        FGridDungeonCell InvalidCell;
        FVector location = InMarker.Transform.GetLocation();
        int32 gridX = FMath::FloorToInt(location.X / GridToMeshScale.X);
        int32 gridY = FMath::FloorToInt(location.Y / GridToMeshScale.Y);
        FGridDungeonCellInfo cellInfo = GridModel->GetGridCellLookup(gridX, gridY);
        FGridDungeonCell* cell = GridModel->GetCell(cellInfo.CellId);
        FGridDungeonCell& cellRef = cell ? *cell : InvalidCell;
        FTransform logicOffset;
        if (TransformLogic) {
            GridTransformLogic->GetNodeOffset(GridModel, GridConfig, this, GridQuery, cellRef, Random, gridX, gridY,
                                              InMarker.Transform, logicOffset);
        }
        else {
            logicOffset = FTransform::Identity;
        }

        FTransform out;
        FTransform::Multiply(&out, &logicOffset, &result);
        result = out;
    }
    return result;
}

FGridDungeonCell UGridDungeonBuilder::BuildCell() {
    FRectangle bounds;
    bounds.Location = GetRandomPointInCircle(GridConfig->InitialRoomRadius);
    bounds.Size = GenerateCellSize();
    return BuildCell(bounds);
}

FGridDungeonCell UGridDungeonBuilder::BuildCell(const FRectangle& InBounds) {
    FGridDungeonCell cell;
    cell.Id = GetNextCellId();
    cell.UserDefined = false;
    cell.Bounds = InBounds;

    int32 area = InBounds.Size.X * InBounds.Size.Y;
    if (area >= GridConfig->RoomAreaThreshold) {
        cell.CellType = EGridDungeonCellType::Room;
    }

    return cell;
}

FIntVector UGridDungeonBuilder::GenerateCellSize() {
    int32 baseSize = GetRandomRoomSize();
    int32 width = baseSize;
    float aspectRatio = 1 + (Random.FRand() * 2 - 1) * GridConfig->RoomAspectDelta;
    aspectRatio = FMath::Max(aspectRatio, 0.1f);
    int32 length = FMath::RoundToInt(width * aspectRatio);
    width = FMath::Max(1, width);
    length = FMath::Max(1, length);
    if (Random.FRand() < 0.5f) {
        // Swap width / length
        int32 temp = width;
        width = length;
        length = temp;
    }
    return FIntVector(width, length, 0);
}

void UGridDungeonBuilder::Shuffle() {
    TArray<FGridDungeonCell>& cells = GridModel->Cells;
    int n = cells.Num();
    for (int i = 0; i < n; i++) {
        int r = i + static_cast<int>(Random.FRand() * (n - i));
        FGridDungeonCell t = cells[r];
        cells[r] = cells[i];
        cells[i] = t;
    }

    GridModel->BuildCellLookup();
}

void UGridDungeonBuilder::BuildCells() {
    TArray<FGridDungeonCell> cells;
    CellIdCounter = 0;
    for (int i = 0; i < GridConfig->NumCells; i++) {
        FGridDungeonCell cell = BuildCell();
        cells.Add(cell);
    }

    GridModel->Cells = cells;
    GridModel->BuildCellLookup();
}

void UGridDungeonBuilder::ApplyCellOffset() {
    FIntVector Offset = GetDungeonOffset();
    for (FGridDungeonCell& Cell : GridModel->Cells) {
        Cell.Bounds.Location += Offset;
    }
}

int32 UGridDungeonBuilder::GetNextCellId() {
    ++CellIdCounter;
    return CellIdCounter;
}

void UGridDungeonBuilder::AddGridMarker(const FString& InMarkerName, const FTransform& InTransform, const TSharedPtr<class IDungeonMarkerUserData>& InUserData) {
    FTransform WorldTransform = InTransform * DungeonTransform;
    FDungeonMarkerInstance& MarkerInstance = AddMarker(InMarkerName, WorldTransform);
    MarkerInstance.UserData = InUserData;
}

void UGridDungeonBuilder::AddGridMarker(const FString& InMarkerName, const FTransform& InTransform, int InCount, const FVector& InterOffset, const TSharedPtr<IDungeonMarkerUserData>& InUserData) {
    FTransform WorldTransform = InTransform * DungeonTransform;
    FTransform Transform = InTransform;
    FVector Location = Transform.GetLocation();
    for (int i = 0; i < InCount; i++) {
        FDungeonMarkerInstance& MarkerInstance = AddMarker(InMarkerName, Transform);
        MarkerInstance.UserData = InUserData;
        Location += InterOffset;
        Transform.SetLocation(Location);
    }
}

void UGridDungeonBuilder::GenerateRoomsFromPaintData(const TSet<FIntVector>& InCells) {
    TSet<FIntVector> Cells = InCells;
    while (Cells.Num() > 0) {
        // Find the bounds of a room
        // Start with the top left corner and move as far as possible to the right
        // Move down and try to increase the length of the room 
        int Z = 0;

        FIntVector Start(MAX_int32, MAX_int32, Z);

        for (const FIntVector& Location : Cells) {
            if (Location.X < Start.X) {
                Start = Location;
            }
            else if (Location.X == Start.X && Location.Y < Start.Y) {
                Start = Location;
            }

            Z = Location.Z;
        }

        int32 Width = 1;
        while (Cells.Contains(FIntVector(Start.X + Width, Start.Y, Z))) Width++;

        int32 Height = 1;
        bool bRowAvailable = true;
        while (bRowAvailable) {
            for (int dx = 0; dx < Width; dx++) {
                if (!Cells.Contains(FIntVector(Start.X + dx, Start.Y + Height, Z))) {
                    bRowAvailable = false;
                    break;
                }
            }
            if (bRowAvailable) {
                Height++;
            }
        }

        FIntVector RoomLocation(Start.X, Start.Y, Z);
        FIntVector RoomSize(Width, Height, 1);
        FRectangle Bounds(RoomLocation, RoomSize);

        AddUserDefinedPlatform(Bounds, EGridDungeonCellType::Room, false);

        // Remove the cells occupied by this room
        for (int dx = 0; dx < Width; dx++) {
            for (int dy = 0; dy < Height; dy++) {
                Cells.Remove(FIntVector(Start.X + dx, Start.Y + dy, Z));
            }
        }
    }
}

void UGridDungeonBuilder::AddUserDefinedPlatforms(UWorld* World) {
    if (!Dungeon) return;

    // Add geometry defined by the editor paint tool
    UGridDungeonToolData* ToolData = Cast<UGridDungeonToolData>(Dungeon->GetToolData());
    if (ToolData) {
        for (const FGridToolPaintStrokeData& CellData : ToolData->PaintedCells) {
            const FIntVector& CellPoint = CellData.Location;
            if (CellData.CellType == EGridPaintToolCellType::Corridor) {
                FGridDungeonCell cell;
                cell.Id = GetNextCellId();
                cell.UserDefined = true;
                FRectangle Bounds;
                Bounds.Location = CellPoint;
                Bounds.Size = FIntVector(1, 1, 1);
                cell.Bounds = Bounds;
                cell.CellType = EGridDungeonCellType::Corridor;
                GridModel->Cells.Add(cell); // TODO: Check for duplicates
            }
        }

        // Generate room rectangle list
        {
            TMap<int, TSet<FIntVector>> RoomCellsByZ;

            for (const FGridToolPaintStrokeData& CellData : ToolData->PaintedCells) {
                if (CellData.CellType == EGridPaintToolCellType::Room) {
                    int32 Z = CellData.Location.Z;
                    if (!RoomCellsByZ.Contains(Z)) {
                        RoomCellsByZ.Add(Z, TSet<FIntVector>());
                    }
                    RoomCellsByZ[Z].Add(CellData.Location);
                }
            }

            for (auto& Entry : RoomCellsByZ) {
                int32 Z = Entry.Key;
                const TSet<FIntVector>& Cells = Entry.Value;
                GenerateRoomsFromPaintData(Cells);
            }
        }
    }

    // Add platform volumes defined in the world
    if (World) {
        for (TObjectIterator<AGridDungeonPlatformVolume> Volume; Volume; ++Volume) {
            if (Volume->Dungeon == Dungeon) {
                AddUserDefinedPlatform(*Volume);
            }
        }
    }

    GridModel->BuildCellLookup();
}

void UGridDungeonBuilder::AddUserDefinedPlatform(AGridDungeonPlatformVolume* Volume) {
    if (!IsValid(Volume)) {
        return;
    }
    FRectangle Bounds;
    Volume->GetDungeonVolumeBounds(GridToMeshScale, Bounds);
    AddUserDefinedPlatform(Bounds, Volume->CellType, Volume->bUseVolumeHeight);
}

void UGridDungeonBuilder::AddUserDefinedPlatform(const FRectangle& Bounds, const EGridDungeonCellType& CellType, bool bInHeightClamped) {
    FGridDungeonCell Cell;
    Cell.Id = GetNextCellId();
    Cell.UserDefined = true;
    Cell.CellType = CellType;
    Cell.Bounds = Bounds;
    Cell.Bounds.Size.Z = 0;
    Cell.bHeightClamped = bInHeightClamped;

    // Remove any cell that intersects with this user defined platform
    for (int i = 0; i < GridModel->Cells.Num();) {
        if (Cell.Bounds.IntersectsWith(GridModel->Cells[i].Bounds)) {
            GridModel->Cells.RemoveAt(i);
        }
        else {
            i++;
        }
    }

    GridModel->Cells.Add(Cell);
}

void UGridDungeonBuilder::ApplyNegationVolumes(UWorld* World) {
    using namespace GridDungeonBuilderImpl;
    if (World) {
        // Grab the bounds of all the negation volumes
        TArray<NegationVolumeInfo> NegationList;
        for (TObjectIterator<ADungeonNegationVolume> NegationVolume; NegationVolume; ++NegationVolume) {
            if (!NegationVolume->IsValidLowLevel() || !IsValid(*NegationVolume)) {
                continue;
            }
            if (NegationVolume->Dungeon != Dungeon) {
                continue;
            }

            FRectangle VolumeBounds;
            NegationVolume->GetDungeonVolumeBounds(GridToMeshScale, VolumeBounds);

            NegationVolumeInfo Info;
            Info.Volume = *NegationVolume;
            Info.Bounds = VolumeBounds;
            NegationList.Add(Info);
        }

        // Remove any cells that fall within any of the negation bounds
        TSet<int32> CellsToRemove;
        for (const FGridDungeonCell& cell : GridModel->Cells) {
            for (const NegationVolumeInfo& VolumeInfo : NegationList) {
                if (cell.UserDefined && !VolumeInfo.Volume->AffectsUserDefinedCells) {
                    // The volume does not affect user defined cells. Ignore it
                    continue;
                }
                bool intersects = cell.Bounds.IntersectsWith(VolumeInfo.Bounds);
                bool remove = intersects;
                if (VolumeInfo.Volume->Reversed) {
                    remove = !remove;

                    if (!remove) {
                        // Do an exhaustive search, and make sure it is fully inside
                        FRectangle intersection = FRectangle::Intersect(cell.Bounds, VolumeInfo.Bounds);
                        if (intersection.Size != cell.Bounds.Size) {
                            remove = true;
                        }
                    }
                }

                if (remove) {
                    CellsToRemove.Add(cell.Id);
                    break;
                }
            }
        }

        GridModel->CellIdToIndexLookup.Reset();

        for (int i = 0; i < GridModel->Cells.Num();) {
            int32 cellId = GridModel->Cells[i].Id;
            if (CellsToRemove.Contains(cellId)) {
                GridModel->Cells.RemoveAt(i);
            }
            else {
                i++;
            }
        }
    }

    GridModel->BuildCellLookup();
}

FIntVector UGridDungeonBuilder::GetDungeonOffset() const {
    FIntVector Offset = FIntVector::ZeroValue;
    if (Dungeon) {
        //FVector OffsetF = Dungeon->GetActorLocation() / GridConfig->GridCellSize;
        FVector OffsetF = FVector::Zero();
        Offset = FIntVector(FMath::RoundToInt(OffsetF.X),
                            FMath::RoundToInt(OffsetF.Y),
                            FMath::RoundToInt(OffsetF.Z));

    }
    return Offset;
}

void UGridDungeonBuilder::Seperate() {
    if (GridModel->BuildState != DungeonModelBuildState::Separation) return;

    Shuffle();
    int32 count = GridModel->Cells.Num();
    TArray<FIntVector> forces;
    forces.AddZeroed(count);

    GridModel->Cells.Sort(CompareFromCenterPredicate);

    bool separated = false;
    for (int a = 0; a < count; a++) {
        for (int b = a + 1; b < count; b++) {
            if (a == b) continue;
            FRectangle& c0 = GridModel->Cells[a].Bounds;
            FRectangle& c1 = GridModel->Cells[b].Bounds;

            if (c0.IntersectsWith(c1)) {
                FIntVector force(0, 0, 0);
                FRectangle intersection = FRectangle::Intersect(c0, c1);
                bool applyOnX = (intersection.Width() < intersection.Height());
                if (intersection.Width() == intersection.Height()) {
                    applyOnX = Random.FRand() > 0.5f;
                }
                if (applyOnX) {
                    force.X = intersection.Width();
                    force.X *= GetForceDirectionMultiplier(c0.X(), c1.X(), c0.Y(), c1.Y());
                }
                else {
                    force.Y = intersection.Height();
                    force.Y *= GetForceDirectionMultiplier(c0.Y(), c1.Y(), c0.X(), c1.X());
                }
                forces[a].X += force.X;
                forces[a].Y += force.Y;

                forces[b].X -= force.X;
                forces[b].Y -= force.Y;

                separated = true;
            }
        }
    }

    for (int a = 0; a < count; a++) {
        FIntVector force(0, 0, 0);
        FIntVector& f = forces[a];
        if (FMath::Abs(f.X) > 0 || FMath::Abs(f.Y) > 0) {
            if (FMath::Abs(f.X) > FMath::Abs(f.Y)) {
                force.X = FMath::Sign(f.X);
            }
            else {
                force.Y = FMath::Sign(f.Y);
            }
        }
        FGridDungeonCell& cell = GridModel->Cells[a];
        FIntVector& location = cell.Bounds.Location;
        location.X += force.X;
        location.Y += force.Y;
    }

    if (!separated) {
        GridModel->BuildState = DungeonModelBuildState::Triangulation;
    }

}

TArray<FGridDungeonCell*> UGridDungeonBuilder::GetCellsOfType(EGridDungeonCellType CellType) {
    FGridDungeonCell* CellArray = GridModel->Cells.GetData();
    TArray<FGridDungeonCell*> filtered;
    for (int i = 0; i < GridModel->Cells.Num(); i++) {
        FGridDungeonCell* cell = CellArray + i;
        if (cell->CellType == CellType) {
            filtered.Add(cell);
        }
    }
    return filtered;
}

void UGridDungeonBuilder::TriangulateRooms() {
    FRandomStream RoomCenterOffsetRandom;
    RoomCenterOffsetRandom.Initialize(GridConfig->Seed);

    TArray<FGridDungeonCell*> rooms = GetCellsOfType(EGridDungeonCellType::Room);
    TArray<float> positions;
    DelauneyTriangleGenerator generator;

    const float RandomOffsetLength = 0.01f;
    for (const FGridDungeonCell* room : rooms) {
        FVector2D RoomCenterOffset = FDAMathUtils::GetRandomDirection2D(RoomCenterOffsetRandom) * RandomOffsetLength;
        FIntVector RoomCenter = room->Center();
        generator.AddPoint(FVector2D(RoomCenter.X, RoomCenter.Y) + RoomCenterOffset);
    }

    if (rooms.Num() >= 3) {
        generator.Triangulate();
        const TArray<FDelauneyTriangle>& Triangles = generator.GetTriangles();
        for (const FDelauneyTriangle& Triangle : Triangles) {
            FGridDungeonCell& cell0 = *rooms[Triangle.v0];
            FGridDungeonCell& cell1 = *rooms[Triangle.v1];
            FGridDungeonCell& cell2 = *rooms[Triangle.v2];

            ConnectCells(cell0, cell1);
            ConnectCells(cell1, cell2);
            ConnectCells(cell2, cell0);
        }
    }
    else if (rooms.Num() == 2) {
        ConnectCells(*rooms[0], *rooms[1]);
    }

    GridModel->BuildState = DungeonModelBuildState::SpanningTree;
}

bool UGridDungeonBuilder::CheckLoop(FGridDungeonCell* currentNode, FGridDungeonCell* comingFrom, TSet<FGridDungeonCell*>& visited) {
    visited.Add(currentNode);
    // check if any of the children have already been visited
    for (int32 childId : currentNode->FixedRoomConnections) {
        FGridDungeonCell* child = GridModel->GetCell(childId);
        if (!child) continue;

        if (child == comingFrom) continue;
        if (visited.Contains(child)) {
            return true;
        }
        bool branchHasLoop = CheckLoop(child, currentNode, visited);
        if (branchHasLoop) return true;
    }
    return false;
}

bool UGridDungeonBuilder::ContainsLoop(const TArray<FGridDungeonCell*>& rooms) {
    for (FGridDungeonCell* room : rooms) {
        if (room) {
            TSet<FGridDungeonCell*> visited;
            bool hasLoop = CheckLoop(room, nullptr, visited);
            if (hasLoop) return true;
        }
    }

    return false;
}


void UGridDungeonBuilder::BuildMinimumSpanningTree() {
    using namespace GridDungeonBuilderImpl;
    TArray<FGridDungeonCell*> rooms = GetCellsOfType(EGridDungeonCellType::Room);
    TMap<int32, TSet<int32>> edgesMapped;

    // Generate unique edge list
    TArray<Edge> edges;
    for (const FGridDungeonCell* room : rooms) {
        for (int32 connectedRoomId : room->ConnectedRooms) {
            FGridDungeonCell* other = GridModel->GetCell(connectedRoomId);
            float distance = GetDistance(*room, *other);
            int32 id0 = room->Id;
            int32 id1 = other->Id;
            if (!edgesMapped.Contains(id0) || !edgesMapped[id0].Contains(id1)) {
                Edge edge(id0, id1, distance);
                edges.Add(edge);

                if (!edgesMapped.Contains(id0)) edgesMapped.Add(id0, TSet<int32>());
                if (!edgesMapped.Contains(id1)) edgesMapped.Add(id1, TSet<int32>());
                edgesMapped[id0].Add(id1);
                edgesMapped[id1].Add(id0);
            }
        }
    }

    edges.Sort();

    for (const Edge& edge : edges) {
        FGridDungeonCell* cell0 = GridModel->GetCell(edge.cellA);
        FGridDungeonCell* cell1 = GridModel->GetCell(edge.cellB);
        if (cell0 && cell1) {
            AddUnique<int32>(cell0->FixedRoomConnections, cell1->Id);
            AddUnique<int32>(cell1->FixedRoomConnections, cell0->Id);

            // Check if this new edge insertion caused a loop in the MST
            bool loop = ContainsLoop(rooms);
            if (loop) {
                cell0->FixedRoomConnections.Remove(cell1->Id);
                cell1->FixedRoomConnections.Remove(cell0->Id);
            }
        }
    }

    // Add some edges from the Delauney triangulation based on a probability
    for (FGridDungeonCell* room : rooms) {
        for (int32 otherDelauney : room->ConnectedRooms) {
            if (!room->FixedRoomConnections.Contains(otherDelauney)) {
                float probability = Random.FRand();
                if (probability < GridConfig->SpanningTreeLoopProbability) {
                    FGridDungeonCell* other = GridModel->GetCell(otherDelauney);
                    if (other) {
                        room->FixedRoomConnections.Add(otherDelauney);
                        other->FixedRoomConnections.Add(room->Id);
                    }
                }
            }
        }
    }


    GridModel->BuildState = DungeonModelBuildState::Corridors;
}

void UGridDungeonBuilder::RemoveRedundantDoors() {
    if (!GridConfig || !GridModel || GridConfig->DoorProximitySteps <= 0) {
        return;
    }

    TArray<FGridDungeonCellDoor>& Doors = GridModel->DoorManager.GetDoors();
    TArray<FGridDungeonCellDoor> DoorsToRemove;
    for (FGridDungeonCellDoor& Door : Doors) {
        int32 CellIdA = Door.AdjacentCells[0];
        int32 CellIdB = Door.AdjacentCells[1];

        // Temporarily disable the door to see if the two adjacent cells can still reach (possbily through a nearby door)
        Door.bEnabled = false;

        bool pathExists = ContainsAdjacencyPath(CellIdA, CellIdB, GridConfig->DoorProximitySteps, false);
        if (!pathExists) {
            // No path exists. We need a door here
            Door.bEnabled = true;
        }
        else {
            // Remove the door
            DoorsToRemove.Add(Door);
        }
    }

    for (const FGridDungeonCellDoor& Door : DoorsToRemove) {
        GridModel->DoorManager.RemoveDoor(Door);
        RemoveStairAtDoor(Door);
    }

    GridModel->BuildCellLookup();
    GenerateAdjacencyLookup();
}

void UGridDungeonBuilder::ConnectCorridors() {
    TArray<FGridDungeonCell*> rooms = GetCellsOfType(EGridDungeonCellType::Room);
    if (rooms.Num() < 2) return;
    TSet<int32> visited;
    FGridDungeonCell* startingRoom = rooms[0];
    ConnectCooridorRecursive(-1, startingRoom->Id, visited);

    // Remove unused cells
    for (int i = 0; i < GridModel->Cells.Num();) {
        if (GridModel->Cells[i].CellType == EGridDungeonCellType::Unknown) {
            GridModel->Cells.RemoveAt(i);
        }
        else {
            i++;
        }
    }
    // Rebuild the cell cache list, since it has been modified
    GridModel->BuildCellLookup();

    GridModel->BuildState = DungeonModelBuildState::Complete;
}

void UGridDungeonBuilder::ConnectCooridorRecursive(int32 incomingRoomId, int32 currentRoomId, TSet<int32>& visited) {
    using namespace GridDungeonBuilderImpl;
    if (incomingRoomId >= 0) {
        int32 c0 = incomingRoomId;
        int32 c1 = currentRoomId;
        if (visited.Contains(HASH(c0, c1))) return;
        visited.Add(HASH(c0, c1));
        visited.Add(HASH(c1, c0));

        ConnectRooms(incomingRoomId, currentRoomId);
    }

    FGridDungeonCell* currentRoom = GridModel->GetCell(currentRoomId);
    check(currentRoom);
    TArray<int32> children = currentRoom->FixedRoomConnections;
    for (int32 otherRoomId : children) {
        FGridDungeonCell* otherRoom = GridModel->GetCell(otherRoomId);
        check(otherRoom);
        int32 i0 = currentRoom->Id;
        int32 i1 = otherRoom->Id;
        if (!visited.Contains(HASH(i0, i1))) {
            ConnectCooridorRecursive(currentRoomId, otherRoomId, visited);
        }
    }
}

void UGridDungeonBuilder::ConnectAdjacentCells(int32 roomA, int32 roomB) {
    using namespace GridDungeonBuilderImpl;
    FGridDungeonCell* cellA = GridModel->GetCell(roomA);
    FGridDungeonCell* cellB = GridModel->GetCell(roomB);
    check(cellA && cellB);

    FRectangle intersection = FRectangle::Intersect(cellA->Bounds, cellB->Bounds);
    bool adjacent = (intersection.Width() > 0 || intersection.Height() > 0);
    if (adjacent) {
        FIntVector doorPointA;
        FIntVector doorPointB;
        doorPointA.Z = cellA->Bounds.Location.Z;
        doorPointB.Z = cellB->Bounds.Location.Z;
        if (intersection.Width() > 0) {
            // shares a horizontal edge
            doorPointA.X = intersection.X() + intersection.Width() / 2;
            doorPointA.Y = intersection.Y() - 1;

            doorPointB.X = doorPointA.X;
            doorPointB.Y = doorPointA.Y + 1;
        }
        else {
            // shares a vertical edge
            doorPointA.X = intersection.X() - 1;
            doorPointA.Y = intersection.Y() + intersection.Height() / 2;

            doorPointB.X = doorPointA.X + 1;
            doorPointB.Y = doorPointA.Y;
        }

        bool bCanCreateDoorAtLocation = true;
        if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
            if (cellA->CellType == EGridDungeonCellType::Room) {
                TSet<FIntVector> CornersA;
                GetCellCorners(cellA->Bounds, CornersA);

                if (CornersA.Contains(doorPointA) || CornersA.Contains(doorPointB)) {
                    bCanCreateDoorAtLocation = false;
                }
            }

            if (bCanCreateDoorAtLocation && cellB->CellType == EGridDungeonCellType::Room) {
                TSet<FIntVector> CornersB;
                GetCellCorners(cellB->Bounds, CornersB);

                if (CornersB.Contains(doorPointA) || CornersB.Contains(doorPointB)) {
                    bCanCreateDoorAtLocation = false;
                }
            }
        }


        // Add a door and return (no corridors needed for adjacent rooms)
        if (bCanCreateDoorAtLocation) {
            GridModel->DoorManager.CreateDoor(doorPointA, doorPointB, roomA, roomB);
        }
    }
}

TArray<FGridDungeonCellDoor> UGridDungeonBuilder::GetDoors() {
    return GridModel->DoorManager.GetDoors();
}

int32 UGridDungeonBuilder::RegisterCorridorCell(int cellX, int cellY, int cellZ, int32 roomA, int32 roomB,
                                                bool canRegisterDoors) {
    FGridDungeonCell* cellA = GridModel->GetCell(roomA);
    FGridDungeonCell* cellB = GridModel->GetCell(roomB);
    check(roomA && roomB);

    FRectangle PaddingBounds(cellX, cellY, 1, 1);
    PaddingBounds.Location.Z = cellZ;

    if (cellA->Bounds.Contains(PaddingBounds.Location) || cellB->Bounds.Contains(PaddingBounds.Location)) {
        // ignore
        return -1;
    }

    bool bRequiresPadding = true;
    int32 CurrentCellId = -1;
    for (FGridDungeonCell& cell : GridModel->Cells) {
        if (cell.Bounds.Contains(PaddingBounds.Location)) {
            if (cell.Id == roomA || cell.Id == roomB) {
                // collides with inside of the room.
                return -1;
            }

            if (cell.CellType == EGridDungeonCellType::Unknown) {
                // Convert this cell into a corridor 
                cell.CellType = EGridDungeonCellType::Corridor;
            }

            // Intersects with an existing cell. do not add corridor padding
            bRequiresPadding = false;
            CurrentCellId = cell.Id;
            break;
        }
    }

    if (bRequiresPadding) {
        FGridDungeonCell corridorCell;
        corridorCell.Id = GetNextCellId();
        corridorCell.UserDefined = false;
        corridorCell.Bounds = PaddingBounds;
        corridorCell.CellType = EGridDungeonCellType::CorridorPadding;
        GridModel->Cells.Add(corridorCell);
        GridModel->BuildCellLookup();

        CurrentCellId = corridorCell.Id;
    }


    if (canRegisterDoors) {
        // Check if we are adjacent to to any of the room nodes
        if (AreCellsAdjacent(CurrentCellId, roomA)) {
            ConnectAdjacentCells(CurrentCellId, roomA);
        }
        if (AreCellsAdjacent(CurrentCellId, roomB)) {
            ConnectAdjacentCells(CurrentCellId, roomB);
        }
    }

    return CurrentCellId; // Return the cell id of the registered cell
}


void UGridDungeonBuilder::FindConnectionPath(FGridDungeonCell* roomA, FGridDungeonCell* roomB, TArray<FIntVector>& OutPath) {
    if (!roomA || !roomB) return;

    TArray<FIntVector> ConnectionPointsA;
    TArray<FIntVector> ConnectionPointsB;
    // If we have thick walls, we don't want doors on the corners of the room
    bool bSkipCornersOnPathSelection = (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks);
    GetRoomConnectionPointsForDoors(roomA->Bounds, bSkipCornersOnPathSelection, ConnectionPointsA);
    GetRoomConnectionPointsForDoors(roomB->Bounds, bSkipCornersOnPathSelection, ConnectionPointsB);

    // Remove points that are inside the other room bounds
    RemovePointsInsideBounds(roomA->Bounds, ConnectionPointsB);
    RemovePointsInsideBounds(roomB->Bounds, ConnectionPointsA);

    // Sort based on the center point
    {
        FVector CenterA = roomA->Bounds.CenterF();
        FVector CenterB = roomB->Bounds.CenterF();
        FVector Center = (CenterA + CenterB) / 2.0f;

        auto SortPredicate = [Center](const FIntVector& A, const FIntVector& B) -> bool {
            float DistSqA = (Center - FVector(A.X, A.Y, A.Z)).SizeSquared();
            float DistSqB = (Center - FVector(B.X, B.Y, B.Z)).SizeSquared();

            return DistSqA < DistSqB;
        };

        ConnectionPointsA.Sort(SortPredicate);
        ConnectionPointsB.Sort(SortPredicate);
    }

    TArray<FGridDungeonCell> RoomCells;
    GetCellsOfType(EGridDungeonCellType::Room, RoomCells);

    TArray<FIntVector> WorstPath;
    for (const FIntVector& PointA : ConnectionPointsA) {
        for (const FIntVector& PointB : ConnectionPointsB) {
            FIntVector Start = PointA;
            FIntVector End = PointB;

            FIntVector Mid1 = FIntVector(Start.X, End.Y, 0);
            FIntVector Mid2 = FIntVector(End.X, Start.Y, 0);

            OutPath.Reset();
            GetManhattanPath(Start, Mid1, End, OutPath);
            if (!ContainsPointInsideCells(OutPath, RoomCells)) {
                return;
            }

            OutPath.Reset();
            GetManhattanPath(Start, Mid2, End, OutPath);
            if (!ContainsPointInsideCells(OutPath, RoomCells)) {
                return;
            }

            if (WorstPath.Num() == 0) {
                WorstPath = OutPath;
            }
        }
    }

    // Did not found a solution.  Return the first solution that was found
    OutPath = WorstPath;
}

void UGridDungeonBuilder::GetRoomConnectionPointsForDoors(const FRectangle& InBounds, bool bInSkipCornerTiles,
                                                          TArray<FIntVector>& OutConnectionPoints) {
    int sx = InBounds.X();
    int ex = InBounds.X() + InBounds.Width();
    int sy = InBounds.Y();
    int ey = InBounds.Y() + InBounds.Height();
    if (bInSkipCornerTiles) {
        sx++;
        sy++;
        ex--;
        ey--;
    }

    for (int x = sx; x < ex; x++) {
        OutConnectionPoints.Add(FIntVector(x, InBounds.Y() - 1, 0));
        OutConnectionPoints.Add(FIntVector(x, InBounds.Y() + InBounds.Height(), 0));
    }

    for (int y = sy; y < ey; y++) {
        OutConnectionPoints.Add(FIntVector(InBounds.X() - 1, y, 0));
        OutConnectionPoints.Add(FIntVector(InBounds.X() + InBounds.Width(), y, 0));
    }
}

void UGridDungeonBuilder::RemovePointsInsideBounds(const FRectangle& InBounds,
                                                   TArray<FIntVector>& OutConnectionPoints) {
    for (int i = 0; i < OutConnectionPoints.Num(); i++) {
        if (InBounds.Contains(OutConnectionPoints[i])) {
            OutConnectionPoints.RemoveAt(i);
            i--;
        }
    }
}

bool UGridDungeonBuilder::ContainsPointInsideBounds(const FRectangle& InBounds, const TArray<FIntVector>& InPoints) {
    for (const FIntVector& Point : InPoints) {
        if (InBounds.Contains(Point)) {
            return true;
        }
    }
    return false;
}

bool UGridDungeonBuilder::ContainsPointInsideCells(const TArray<FIntVector>& InPoints, TArray<FGridDungeonCell>& InCells) {
    for (const FGridDungeonCell& Cell : InCells) {
        if (ContainsPointInsideBounds(Cell.Bounds, InPoints)) {
            return true;
        }
    }
    return false;
}

void UGridDungeonBuilder::GetManhattanPath(const FIntVector& InStart, const FIntVector& InMid, const FIntVector& InEnd,
                                           TArray<FIntVector>& OutPath) {
    OutPath.Add(InStart);
    GetPathOnAxisLine(InStart, InMid, OutPath);
    if (InMid != InStart) {
        OutPath.Add(InMid);
    }
    GetPathOnAxisLine(InMid, InEnd, OutPath);
    if (InEnd != InMid) {
        OutPath.Add(InEnd);
    }
}

void UGridDungeonBuilder::GetPathOnAxisLine(const FIntVector& InStart, const FIntVector& InEnd,
                                            TArray<FIntVector>& OutPath) {
    if (InStart == InEnd) {
        OutPath.Add(InStart);
        return;
    }

    int LengthX = InEnd.X - InStart.X;
    int LengthY = InEnd.Y - InStart.Y;
    int Length = FMath::Max(FMath::Abs(LengthX), FMath::Abs(LengthY));
    int DX = (LengthX == 0) ? 0 : static_cast<int>(FMath::Sign(LengthX));
    int DY = (LengthY == 0) ? 0 : static_cast<int>(FMath::Sign(LengthY));

    FIntVector P = InStart;
    for (int i = 0; i < Length; i++) {
        // Do not add the start and the end point
        if (i > 0 && i < Length) {
            OutPath.Add(P);
        }

        P.X += DX;
        P.Y += DY;
    }
}


void UGridDungeonBuilder::ConnectRooms(int32 roomAId, int32 roomBId) {
    FGridDungeonCell* roomA = GridModel->GetCell(roomAId);
    FGridDungeonCell* roomB = GridModel->GetCell(roomBId);
    check(roomA && roomB);

    FRectangle intersection = FRectangle::Intersect(roomA->Bounds, roomB->Bounds);

    int32 MinIntersectionSize = 1;
    if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
        MinIntersectionSize = 3;
    }
    int IntersectionSize = FMath::Max(intersection.Width(), intersection.Height());


    bool adjacent = IntersectionSize >= MinIntersectionSize;
    if (adjacent) {
        ConnectAdjacentCells(roomAId, roomBId);
    }
    else {
        TArray<FIntVector> PathList;
        FindConnectionPath(roomA, roomB, PathList);


        int32 PreviousCellId = -1;
        TSet<FIntPoint> VisitedCells;

        for (const FIntVector& PathPoint : PathList) {
            const int x = PathPoint.X;
            const int y = PathPoint.Y;
            const int z = PathPoint.Z;

            // add a corridor cell
            int32 CurrentCellId = RegisterCorridorCell(x, y, z, roomAId, roomBId, true);
            VisitedCells.Add(FIntPoint(x, y));

            // Check if we need to create a door between the two.  
            // This is needed in case we have an extra room through the corridor.  
            // This room needs to have doors created
            {
                if (PreviousCellId != -1 && CurrentCellId != PreviousCellId) {
                    FGridDungeonCell* PreviousCell = GridModel->GetCell(PreviousCellId);
                    FGridDungeonCell* CurrentCell = GridModel->GetCell(CurrentCellId);
                    if (PreviousCell && CurrentCell && PreviousCell != CurrentCell) {
                        if (CurrentCell->CellType == EGridDungeonCellType::Room || PreviousCell->CellType == EGridDungeonCellType::Room) {
                            ConnectAdjacentCells(PreviousCellId, CurrentCellId);
                        }
                    }
                }
                PreviousCellId = CurrentCellId;
            }

            int LaneWidth = FMath::Clamp(GridConfig->LaneWidth, 1, 10);
            if (LaneWidth > 1) {
                int Offset = LaneWidth / 2;
                for (int dx = 0; dx < LaneWidth; dx++) {
                    for (int dy = 0; dy < LaneWidth; dy++) {
                        int xx = x + dx - Offset;
                        int yy = y + dy - Offset;
                        if (xx == x && yy == y) continue;
                        FIntPoint VisitedKey(xx, yy);
                        if (!VisitedCells.Contains(VisitedKey)) {
                            RegisterCorridorCell(xx, yy, z, roomAId, roomBId);
                            VisitedCells.Add(VisitedKey);
                        }
                    }
                }
            }
        }
    }

}

DungeonModelBuildState UGridDungeonBuilder::GetBuildState() {
    return GridModel->BuildState;
}

FGridDungeonCell UGridDungeonBuilder::GetCell(int32 Id) {
    FGridDungeonCell* cell = GridModel->GetCell(Id);
    return cell ? *cell : FGridDungeonCell();
}

bool UGridDungeonBuilder::GetStair(int32 ownerCell, int32 connectedToCell, FGridDungeonStairInfo& outStair) {
    if (GridModel->CellStairsLookup.Contains(ownerCell)) {
        for (const FGridDungeonStairInfo& stair : GridModel->CellStairsLookup[ownerCell]) {
            if (stair.ConnectedToCell == connectedToCell) {
                outStair = stair;
                return true;
            }
        }
    }
    return false;
}


bool UGridDungeonBuilder::ContainsAdjacencyPath(int32 cellIdA, int32 cellIdB, int32 maxDepth,
                                                bool bForceRoomConnection) {
    using namespace GridDungeonBuilderImpl;
    FGridDungeonCell* cellA = GridModel->GetCell(cellIdA);
    FGridDungeonCell* cellB = GridModel->GetCell(cellIdB);
    if (!cellA || !cellB) return false;

    // Force a connection if any one is a room
    if (bForceRoomConnection && (cellA->CellType == EGridDungeonCellType::Room || cellB->CellType == EGridDungeonCellType::Room)) {
        return false;
    }

    TQueue<StairAdjacencyQueueNode> queue;
    TSet<uint32> visited;
    queue.Enqueue(StairAdjacencyQueueNode(cellIdA, 0));
    
    StairAdjacencyQueueNode topNode;
    while (queue.Dequeue(topNode)) {
        if (topNode.depth > maxDepth) continue;

        int32 topId = topNode.cellId;
        if (visited.Contains(topId)) continue;
        visited.Add(topId);
        if (topId == cellIdB) {
            // Reached the target cell
            return true;
        }
        FGridDungeonCell* top = GridModel->GetCell(topId);
        if (!top) continue;
        for (int32 adjacentCellId : top->AdjacentCells) {
            if (visited.Contains(adjacentCellId)) continue;

            // Check if we have a valid path between these two adjacent cells 
            // (either through same height or by a already registered stair)
            FGridDungeonCell* adjacentCell = GridModel->GetCell(adjacentCellId);

            bool pathExists = (adjacentCell->Bounds.Location.Z == top->Bounds.Location.Z);
            if (!pathExists) {
                // Cells are on different heights.  Check if we have a stair connecting these cells
                FGridDungeonStairInfo stair;
                if (GetStair(topId, adjacentCellId, stair)) {
                    pathExists = true;
                }
                if (!pathExists) {
                    if (GetStair(adjacentCellId, topId, stair)) {
                        pathExists = true;
                    }
                }
            }

            if (pathExists) {
                // We sure we are not going through a wall
                if (top->CellType == EGridDungeonCellType::Room || adjacentCell->CellType == EGridDungeonCellType::Room) {
                    const bool containsDoor = GridModel->DoorManager.ContainsDoorBetweenCells(top->Id, adjacentCellId);
                    pathExists = containsDoor;
                }
            }

            if (pathExists) {
                queue.Enqueue(StairAdjacencyQueueNode(adjacentCellId, topNode.depth + 1));
            }
        }
    }
    return false;
}

int UGridDungeonBuilder::GetForceDirectionMultiplier(float a, float b, float a1, float b1) {
    if (a == b) {
        return (a1 < b1) ? -1 : 1;
    }
    return (a < b) ? -1 : 1;
}

FIntVector UGridDungeonBuilder::GetRandomPointInCircle(double radius) {
    float angle = Random.FRand() * PI * 2;
    float u = Random.FRand() + Random.FRand();
    float r = (u > 1) ? 2 - u : u;
    r *= radius;
    int32 x = FMath::RoundToInt(FMath::Cos(angle) * r);
    int32 y = FMath::RoundToInt(FMath::Sin(angle) * r);
    return FIntVector(x, y, 0);
}

bool UGridDungeonBuilder::AreCellsAdjacent(int32 cellAId, int32 cellBId) {
    FGridDungeonCell* cellA = GridModel->GetCell(cellAId);
    FGridDungeonCell* cellB = GridModel->GetCell(cellBId);
    check(cellA && cellB);
    FRectangle intersection = FRectangle::Intersect(cellA->Bounds, cellB->Bounds);
    bool adjacent = (intersection.Width() > 0 || intersection.Height() > 0);
    return adjacent;
}

int32 UGridDungeonBuilder::GetRandomRoomSize() {
    float r = 0;
    while (r <= 0) r = GaussRandom.NextGaussianFloat(GridConfig->NormalMean, GridConfig->NormalStd);
    float roomSize = GridConfig->MinCellSize + r * (GridConfig->MaxCellSize - GridConfig->MinCellSize);
    return FMath::RoundToInt(roomSize);
}

void UGridDungeonBuilder::GenerateAdjacencyLookup() {
    // Cache the cell types based on their positions
    TMap<int32, TMap<int32, FGridDungeonCellInfo>>& GridCellInfoLookup = GridModel->GridCellInfoLookup;
    GridCellInfoLookup.Reset();
    for (const FGridDungeonCell& cell : GridModel->Cells) {
        if (cell.CellType == EGridDungeonCellType::Unknown) continue;
        FIntVector basePosition = cell.Bounds.Location;
        FTransform transform = FTransform::Identity;
        for (int dx = 0; dx < cell.Bounds.Size.X; dx++) {
            for (int dy = 0; dy < cell.Bounds.Size.Y; dy++) {
                int32 x = basePosition.X + dx;
                int32 y = basePosition.Y + dy;

                // register the cell type in the lookup
                if (!GridCellInfoLookup.Contains(x)) GridCellInfoLookup.Add(x, TMap<int32, FGridDungeonCellInfo>());
                GridCellInfoLookup[x].Add(y, FGridDungeonCellInfo(cell.Id, cell.CellType));
            }
        }
    }

    // Create cell adjacency list
    for (FGridDungeonCell& cell : GridModel->Cells) {
        if (cell.CellType == EGridDungeonCellType::Unknown) continue;
        FIntVector basePosition = cell.Bounds.Location;
        FTransform transform = FTransform::Identity;
        int32 SizeX = cell.Bounds.Size.X;
        int32 SizeY = cell.Bounds.Size.Y;
        for (int dx = 0; dx < SizeX; dx++) {
            for (int dy = 0; dy < SizeY; dy++) {
                if (dx >= 0 && dx < SizeX - 1 && dy >= 0 && dy < SizeY - 1) {
                    // Ignore the cells in the middle
                    continue;
                }

                int32 x = cell.Bounds.Location.X + dx;
                int32 y = cell.Bounds.Location.Y + dy;
                CheckAndMarkAdjacent(cell, x + 1, y);
                CheckAndMarkAdjacent(cell, x, y + 1);
            }
        }
    }

    // Cache the positions of the doors in the grid
    for (const FGridDungeonCellDoor& Door : GetDoors()) {
        int32 x0 = Door.AdjacentTiles[0].X;
        int32 y0 = Door.AdjacentTiles[0].Y;
        int32 x1 = Door.AdjacentTiles[1].X;
        int32 y1 = Door.AdjacentTiles[1].Y;
        if (GridCellInfoLookup.Contains(x0) && GridCellInfoLookup[x0].Contains(y0)) GridCellInfoLookup[x0][y0].
            ContainsDoor = true;
        if (GridCellInfoLookup.Contains(x1) && GridCellInfoLookup[x1].Contains(y1)) GridCellInfoLookup[x1][y1].
            ContainsDoor = true;
    }


}

void UGridDungeonBuilder::CheckAndMarkAdjacent(FGridDungeonCell& cell, int32 otherCellX, int32 otherCellY) {
    FGridDungeonCellInfo info = GridModel->GetGridCellLookup(otherCellX, otherCellY);
    if (info.CellId == cell.Id) return;
    FGridDungeonCell* otherCell = GridModel->GetCell(info.CellId);
    if (!otherCell) return;
    if (otherCell->CellType == EGridDungeonCellType::Unknown || cell.CellType == EGridDungeonCellType::Unknown) return;

    // Mark the two cells as adjacent
    cell.AdjacentCells.AddUnique(otherCell->Id);
    otherCell->AdjacentCells.AddUnique(cell.Id);
}

void UGridDungeonBuilder::DrawDebugData(UWorld* InWorld, bool bPersistant, float LifeTime) {
    using namespace GridDungeonBuilderImpl;

    if (!GridModel || !GridConfig) return;

    g_DebugGroupColors.Remove(-1);
    g_DebugGroupColors.Add(-1, FColor::Black);

    for (const FGridDungeonCell& cell : GridModel->Cells) {
        if (cell.CellType == EGridDungeonCellType::Unknown) continue;
        FVector Location = UDungeonModelHelper::MakeVector(cell.Bounds.Location);
        FVector Size = UDungeonModelHelper::MakeVector(cell.Bounds.Size);
        Size /= 2;
        Location += Size;
        Location *= GridToMeshScale;
        Size *= GridToMeshScale;
        Size += FVector(0, 0, 5);
        FColor Color = FColor::White;
        switch (cell.CellType) {
        case EGridDungeonCellType::Room:
            Color = FColor::Red;
            break;

        case EGridDungeonCellType::Corridor:
            Color = FColor(0, 255, 255);
            break;

        case EGridDungeonCellType::CorridorPadding:
            Color = FColor(0, 0, 255);
            break;
        }

        /*
        // Assign color based on the group
        {
            int32 GroupId = cell.ClusterId;
            if (g_DebugGroupColors.Contains(GroupId)) {
                Color = g_DebugGroupColors[GroupId];
            }
            else {
                // Assign a new random group color
                Color = FColor(
                    FMath::RandRange(64, 255),
                    FMath::RandRange(64, 255),
                    FMath::RandRange(64, 255));
                g_DebugGroupColors.Add(GroupId, Color);
            }
        }
        */

        FColor SolidColor = Color;
        SolidColor.A = 240;

        //DrawDebugSolidBox(InWorld, Location, Size, SolidColor, bPersistant, lifeTime);
        DrawDebugBox(InWorld, Location, Size, Color, bPersistant, LifeTime);

        /*
        std::stringstream message;
        message << Location.Z << " [" << cell.Id << "]";
        //FString HeightText = FString::FormatAsNumber(Location.Z);
        FString HeightText = message.str().c_str();
        DrawDebugString(InWorld, Location, HeightText, NULL, Color.White, lifeTime);
        */

        // Debug draw the adjacency list
        for (int32 adjacentCellId : cell.AdjacentCells) {
            FGridDungeonCell* adjacentCell = GridModel->GetCell(adjacentCellId);
            if (!adjacentCell) continue;
            FVector CellCenter0, CellCenter1;
            UGridDungeonModelHelper::GetCellCenter(cell, CellCenter0);
            UGridDungeonModelHelper::GetCellCenter(*adjacentCell, CellCenter1);
            CellCenter0 *= GridToMeshScale;
            CellCenter1 *= GridToMeshScale;
            DrawDebugLine(InWorld, CellCenter0, CellCenter1, FColor(255, 128, 0), bPersistant, LifeTime, 0, 20);
        }
    }

    // Draw room connections
    TArray<FGridDungeonCell*> Rooms = GetCellsOfType(EGridDungeonCellType::Room);
    for (const FGridDungeonCell* Room : Rooms) {
        for (int32 OtherDelauney : Room->ConnectedRooms) {
            if (FGridDungeonCell* Other = GridModel->GetCell(OtherDelauney)) {
                bool bPreferedEdge = Room->FixedRoomConnections.Contains(OtherDelauney);
                FColor color = bPreferedEdge ? FColor::Red : FColor::Green;
                FVector CellCenter0, CellCenter1;
                UGridDungeonModelHelper::GetCellCenter(*Room, CellCenter0);
                UGridDungeonModelHelper::GetCellCenter(*Other, CellCenter1);
                CellCenter0 *= GridToMeshScale;
                CellCenter1 *= GridToMeshScale;
                DrawDebugLine(InWorld, CellCenter0, CellCenter1, color, bPersistant, LifeTime, 0, 40);
            }
        }
    }


    // Draw cluster info
    FRandomStream DebugRandomStream(GridConfig->Seed);
    const FVector& GridSize = GridConfig->GridCellSize;
    for (const auto& Pair : GridModel->Clusters) {
        const FGridDungeonCellClusterInfo& ClusterInfo = Pair.Value;
        if (ClusterInfo.Boundary.Num() > 1) {
            FColor BorderColor = FColor::MakeRandomSeededColor(DebugRandomStream.RandRange(0, 10000));
            for (int i = 0; i < ClusterInfo.Boundary.Num(); i++) {
                FVector Start = GridSize * FVector(ClusterInfo.Boundary[i], ClusterInfo.GridZ);
                FVector End = GridSize * FVector(ClusterInfo.Boundary[(i + 1) % ClusterInfo.Boundary.Num()], ClusterInfo.GridZ);
                
                DrawDebugLine(InWorld, Start, End, BorderColor, bPersistant, LifeTime, 0, 50);
            }
        }
    } 

    for (const FGridDungeonCellDoor& door : GetDoors()) {
        FVector Start = UDungeonModelHelper::MakeVector(door.AdjacentTiles[0]);
        FVector End = UDungeonModelHelper::MakeVector(door.AdjacentTiles[1]);

        Start += FVector(0.5f, 0.5f, 0);
        End += FVector(0.5f, 0.5f, 0);
        Start *= GridToMeshScale;
        End *= GridToMeshScale;
        DrawDebugLine(InWorld, Start, End, FColor::Green, bPersistant, LifeTime, 0, 20);
    }
}

void UGridDungeonBuilder::MirrorDungeon() {
    if (Dungeon) {
        for (TObjectIterator<ADungeonMirrorVolume> Volume; Volume; ++Volume) {
            if (!Volume || !IsValid(*Volume) || !Volume->IsValidLowLevel()) {
                continue;
            }
            if (Volume->Dungeon == Dungeon) {
                // Build a lookup of the theme for faster access later on
                MirrorDungeonWithVolume(*Volume);

                // Cache the cell types based on their positions
                TMap<int32, TMap<int32, FGridDungeonCellInfo>>& GridCellInfoLookup = GridModel->GridCellInfoLookup;
                GridCellInfoLookup.Reset();
                for (const FGridDungeonCell& cell : GridModel->Cells) {
                    if (cell.CellType == EGridDungeonCellType::Unknown) continue;
                    FIntVector basePosition = cell.Bounds.Location;
                    FTransform transform = FTransform::Identity;
                    for (int dx = 0; dx < cell.Bounds.Size.X; dx++) {
                        for (int dy = 0; dy < cell.Bounds.Size.Y; dy++) {
                            int32 x = basePosition.X + dx;
                            int32 y = basePosition.Y + dy;

                            // register the cell type in the lookup
                            if (!GridCellInfoLookup.Contains(x)) GridCellInfoLookup.
                                Add(x, TMap<int32, FGridDungeonCellInfo>());
                            GridCellInfoLookup[x].Add(y, FGridDungeonCellInfo(cell.Id, cell.CellType));
                        }
                    }
                }

                GridModel->BuildCellLookup();
            }
        }
    }
}

TSubclassOf<UDungeonModel> UGridDungeonBuilder::GetModelClass() {
    return UGridDungeonModel::StaticClass();
}

TSubclassOf<UDungeonConfig> UGridDungeonBuilder::GetConfigClass() {
    return UGridDungeonConfig::StaticClass();
}

TSubclassOf<UDungeonToolData> UGridDungeonBuilder::GetToolDataClass() {
    return UGridDungeonToolData::StaticClass();
}

TSubclassOf<UDungeonQuery> UGridDungeonBuilder::GetQueryClass() {
    return UGridDungeonQuery::StaticClass();
}

void UGridDungeonBuilder::CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules) {
    OutRules = {};
    if (!GridConfig) {
        GridConfig = Cast<UGridDungeonConfig>(DungeonConfig);
    }

    const FTransform HalfWallOffset = FTransform(FQuat::Identity, FVector(0, 0, -1), FVector(1, 1, 0.5f));
    OutRules.SetGridSize(GridConfig ? GridConfig->GridCellSize : FVector::ZeroVector);
    
    OutRules.Set(FGridBuilderMarkers::GROUND, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FGridBuilderMarkers::STAIR, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FGridBuilderMarkers::STAIR2X, { EDungeonMarkerVisualizationShape::Tile });
    
    OutRules.Set(FGridBuilderMarkers::WALL, { EDungeonMarkerVisualizationShape::Wall });
    OutRules.Set(FGridBuilderMarkers::WALLHALF, { EDungeonMarkerVisualizationShape::Wall, HalfWallOffset });
    OutRules.Set(FGridBuilderMarkers::ROOMWALL, { EDungeonMarkerVisualizationShape::Wall });
    OutRules.Set(FGridBuilderMarkers::FENCE, { EDungeonMarkerVisualizationShape::Wall });
    OutRules.Set(FGridBuilderMarkers::DOOR, { EDungeonMarkerVisualizationShape::Wall });
    
    OutRules.Set(FGridBuilderMarkers::FENCE_SEPARATOR, { EDungeonMarkerVisualizationShape::Pillar });
    OutRules.Set(FGridBuilderMarkers::WALL_SEPARATOR, { EDungeonMarkerVisualizationShape::Pillar });
    OutRules.Set(FGridBuilderMarkers::WALLHALF_SEPARATOR, { EDungeonMarkerVisualizationShape::Pillar, HalfWallOffset });
    OutRules.Set(FGridBuilderMarkers::ROOMWALL_SEPARATOR, { EDungeonMarkerVisualizationShape::Pillar });
}


bool UGridDungeonBuilder::IdentifyBuildSucceeded() const {
    UGridDungeonModel* CastModel = Cast<UGridDungeonModel>(DungeonModel);
    if (!CastModel) return false;

    int32 NumRooms = 0;
    bool bHasUserDefinedCells = false;
    for (const FGridDungeonCell& Cell : CastModel->Cells) {
        if (Cell.UserDefined) {
            bHasUserDefinedCells = true;
        }

        if (Cell.CellType == EGridDungeonCellType::Room) {
            NumRooms++;
        }
    }

    // Success Condition: We have user defined volumes (e.g. platform volumes) or at-least one room generated 
    return bHasUserDefinedCells || NumRooms > 0;
}

TSharedPtr<IMarkerGenProcessor> UGridDungeonBuilder::CreateMarkerGenProcessor(const FTransform& InDungeonTransform) const {
    if (const UGridDungeonConfig* Config = GridConfig ? GridConfig.Get() : Cast<UGridDungeonConfig>(DungeonConfig)) {
        const FVector GridSize = Config->GridCellSize;
        const FIntVector OffsetCoord = GetDungeonOffset();
        const FVector Offset = FVector(OffsetCoord) * GridSize;
        return MakeShareable(new FMarkerGenGridProcessor(FTransform(Offset), GridSize));
    }
    return nullptr;
}

void UGridDungeonBuilder::GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) {
    OutMarkerNames.Reset();
    OutMarkerNames.Add("Ground");
    OutMarkerNames.Add("Wall");
    OutMarkerNames.Add("WallSeparator");
    OutMarkerNames.Add("Fence");
    OutMarkerNames.Add("FenceSeparator");
    OutMarkerNames.Add("Door");
    OutMarkerNames.Add("Stair");
    OutMarkerNames.Add("Stair2X");
    OutMarkerNames.Add("WallHalf");
    OutMarkerNames.Add("WallHalfSeparator");
}

bool UGridDungeonBuilder::ContainsCell(int32 x, int32 y) {
    return GridModel->GridCellInfoLookup.Contains(x) && GridModel->GridCellInfoLookup[x].Contains(y);
}

void UGridDungeonBuilder::GetRooms(TArray<FGridDungeonCell>& RoomCells) {
    GetCellsOfType(EGridDungeonCellType::Room, RoomCells);
}

void UGridDungeonBuilder::GetCorridors(TArray<FGridDungeonCell>& CorridorCells) {
    GetCellsOfType(EGridDungeonCellType::Corridor, CorridorCells);
    GetCellsOfType(EGridDungeonCellType::CorridorPadding, CorridorCells);
}

void UGridDungeonBuilder::GetCellsOfType(EGridDungeonCellType CellType, TArray<FGridDungeonCell>& Cells) {
    for (const FGridDungeonCell& cell : GridModel->Cells) {
        if (cell.CellType == CellType) {
            Cells.Add(cell);
        }
    }
}

void UGridDungeonBuilder::BuildMesh_Floor(const FGridDungeonCell& cell) {
    FIntVector basePosition = cell.Bounds.Location;
    FTransform transform = FTransform::Identity;
    for (int dx = 0; dx < cell.Bounds.Size.X; dx++) {
        for (int dy = 0; dy < cell.Bounds.Size.Y; dy++) {
            int32 x = basePosition.X + dx;
            int32 y = basePosition.Y + dy;
            int32 z = basePosition.Z;

            FVector position(x, y, z);
            position += FVector(0.5f, 0.5f, 0);
            position *= GridToMeshScale;
            transform.SetLocation(position);

            AddGridMarker(FGridBuilderMarkers::GROUND, transform);
        }
    }
}

void UGridDungeonBuilder::BuildMesh_RoomDecoration(const FGridDungeonCell& cell) {
    FIntVector ILocation = cell.Bounds.Location;
    FIntVector ISize = cell.Bounds.Size;
    FVector Position = UDungeonModelHelper::MakeVector(ILocation) * GridToMeshScale;
    FVector Size = UDungeonModelHelper::MakeVector(ISize) * GridToMeshScale;
    FVector Center = Position + Size / 2.0f;

    FTransform transform = FTransform::Identity;
    int32 x = ILocation.X;
    int32 y = ILocation.Y;
    int32 z = ILocation.Z;
    for (int32 dx = 0; dx < ISize.X; dx++) {
        FVector Location = FVector(x + dx, y, z);
        transform.SetRotation(FQuat(FVector(0, 0, 1), 0));
        transform.SetLocation(Location * GridToMeshScale);
        if (dx > 0) {
            AddGridMarker(FGridBuilderMarkers::ROOMWALL_SEPARATOR, transform);
        }
        Location += FVector(0.5f, 0, 0);
        transform.SetLocation(Location * GridToMeshScale);
        AddGridMarker(FGridBuilderMarkers::ROOMWALL, transform);

        Location = FVector(x + dx, y + ISize.Y, z);
        transform.SetRotation(FQuat(FVector(0, 0, 1), PI));
        transform.SetLocation(Location * GridToMeshScale);
        if (dx > 0) {
            AddGridMarker(FGridBuilderMarkers::ROOMWALL_SEPARATOR, transform);
        }
        Location += FVector(0.5f, 0, 0);
        transform.SetLocation(Location * GridToMeshScale);
        AddGridMarker(FGridBuilderMarkers::ROOMWALL, transform);
    }

    for (int32 dy = 0; dy < ISize.Y; dy++) {
        FVector Location = FVector(x, y + dy, z);
        transform.SetLocation(Location * GridToMeshScale);
        transform.SetRotation(FQuat(FVector(0, 0, 1), -PI / 2));
        if (dy > 0) {
            AddGridMarker(FGridBuilderMarkers::ROOMWALL_SEPARATOR, transform);
        }
        Location += FVector(0, 0.5f, 0);
        transform.SetLocation(Location * GridToMeshScale);
        AddGridMarker(FGridBuilderMarkers::ROOMWALL, transform);

        Location = FVector(x + ISize.X, y + dy, z);
        transform.SetRotation(FQuat(FVector(0, 0, 1), PI / 2));
        transform.SetLocation(Location * GridToMeshScale);
        if (dy > 0) {
            AddGridMarker(FGridBuilderMarkers::ROOMWALL_SEPARATOR, transform);
        }
        Location += FVector(0, 0.5f, 0);
        transform.SetLocation(Location * GridToMeshScale);
        AddGridMarker(FGridBuilderMarkers::ROOMWALL, transform);
    }

    {
        // Add open space markers
        transform = FTransform::Identity;
        transform.SetLocation(Center);
        AddGridMarker(FGridBuilderMarkers::ROOMOPENSPACE, transform);
    }
}

void UGridDungeonBuilder::FixDoorTransform(int32 x0, int32 y0, int32 x1, int32 y1, FTransform& OutTransform) {
    FGridDungeonCell* cell0 = GridModel->GetCell(GridModel->GetGridCellLookup(x0, y0).CellId);
    FGridDungeonCell* cell1 = GridModel->GetCell(GridModel->GetGridCellLookup(x1, y1).CellId);
    if (!cell0 || !cell1) return;
    float z = FMath::Max(cell0->Bounds.Location.Z, cell1->Bounds.Location.Z);
    FVector Location = OutTransform.GetLocation();
    Location.Z = z * GridToMeshScale.Z;
    //OutTransform.SetLocation(Location);
}

void UGridDungeonBuilder::AddCorridorPadding(int x, int y, int z) {
    FGridDungeonCell padding;
    padding.Id = GetNextCellId();
    padding.UserDefined = false;

    FRectangle bounds(x, y, 1, 1);
    bounds.Location.Z = z;
    padding.Bounds = bounds;
    padding.CellType = EGridDungeonCellType::CorridorPadding;

    GridModel->Cells.Add(padding);
}

void UGridDungeonBuilder::RemoveStairAtDoor(const FGridDungeonCellDoor& Door) {
    int32 CellA = Door.AdjacentCells[0];
    int32 CellB = Door.AdjacentCells[1];

    if (GridModel->CellStairsLookup.Contains(CellA)) {
        TArray<FGridDungeonStairInfo>& CellAStairs = GridModel->CellStairsLookup[CellA];
        for (int i = 0; i < CellAStairs.Num(); i++) {
            if (CellAStairs[i].ConnectedToCell == CellB) {
                // Found our cell. Remove it
                CellAStairs.RemoveAt(i);
                break;
            }
        }
    }

    // Remove from the other direction
    if (GridModel->CellStairsLookup.Contains(CellB)) {
        TArray<FGridDungeonStairInfo>& CellBStairs = GridModel->CellStairsLookup[CellB];
        for (int i = 0; i < CellBStairs.Num(); i++) {
            if (CellBStairs[i].ConnectedToCell == CellA) {
                // Found our cell. Remove it
                CellBStairs.RemoveAt(i);
                break;
            }
        }
    }
}

void UGridDungeonBuilder::SerializeModel() {
    GridModel->Doors = GridModel->DoorManager.GetDoors();
    GridModel->Stairs.Reset();

    TArray<TArray<FGridDungeonStairInfo>> StairMultiArray;
    GridModel->CellStairsLookup.GenerateValueArray(StairMultiArray);
    for (const TArray<FGridDungeonStairInfo>& StairArray : StairMultiArray) {
        for (const FGridDungeonStairInfo& Stair : StairArray) {
            GridModel->Stairs.Add(Stair);
        }
    }
}

class FDAVoxelMarkerEmitHelper {
    FDAVoxelMarkerEmitHelper(UDungeonBuilder* InBuilder, const FVector& InGridSize) {
        if (!InBuilder) {
            return;
        }
        
        FDAVoxelMeshGenerationSettings VoxelMeshGenSettings{};
        ADungeon* OuterDungeon = Cast<ADungeon>(InBuilder->GetOuter());
        if (OuterDungeon) {
            VoxelMeshGenSettings = OuterDungeon->VoxelMeshSettings;
        }

        constexpr float LocalVolumeSize = 200;
        const float WallThickness = VoxelMeshGenSettings.WallThickness;
        // Get world-specific ceiling height for volume placement
        const float BaseRoomHeight = InGridSize.Z * 2;
        float TotalCeilingHeight = BaseRoomHeight;
        if (OuterDungeon && OuterDungeon->VoxelSDFModel) {
            TotalCeilingHeight = OuterDungeon->VoxelSDFModel->GetTotalCeilingHeight(BaseRoomHeight);
        }
        const float TargetX = InGridSize.X + WallThickness;
        const float TargetY = WallThickness;
        const float TargetZ = TotalCeilingHeight + WallThickness * 4;
        const FVector VolumeEdgeScale = FVector(TargetX, TargetY, TargetZ) / LocalVolumeSize;
        const FVector VolumeStairScale = InGridSize / LocalVolumeSize;
    }
};

void UGridDungeonBuilder::BuildMesh_Room(const FGridDungeonCell& cell) {
    using namespace GridDungeonBuilderImpl;

    BuildMesh_Floor(cell);

    FVector HalfWallOffset = GridToMeshScale * FVector(0, 0, -1);
    // Build the room walls
    FIntVector basePosition = cell.Bounds.Location;
    int32 Elevation;
    
    typedef TFunction<void(const FString&, const FTransform&)> TFuncEmitVoxelMarker;
    TFuncEmitVoxelMarker FnEmitVoxelEdgeMarker;
    {
        FDAVoxelMeshGenerationSettings VoxelMeshGenSettings{};
        ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter());
        if (OuterDungeon) {
            VoxelMeshGenSettings = OuterDungeon->VoxelMeshSettings;
        }

        FVector GridSize = GridConfig->GridCellSize;
        constexpr float LocalVolumeSize = 200;
        const float WallThickness = VoxelMeshGenSettings.WallThickness;
        // Get world-specific ceiling height for volume placement
        const float BaseRoomHeight = GridSize.Z * 2;
        float TotalCeilingHeight = BaseRoomHeight;
        if (OuterDungeon && OuterDungeon->VoxelSDFModel) {
            TotalCeilingHeight = OuterDungeon->VoxelSDFModel->GetTotalCeilingHeight(BaseRoomHeight);
        }
        const float TargetX = GridSize.X + WallThickness;
        const float TargetY = WallThickness;
        const float TargetZ = TotalCeilingHeight + WallThickness * 4;
        const FVector VolumeEdgeScale = FVector(TargetX, TargetY, TargetZ) / LocalVolumeSize;
        const FVector VolumeStairScale = GridSize / LocalVolumeSize;
        
        FnEmitVoxelEdgeMarker = [&](const FString& InMarkerName, const FTransform& InMarkerTransform) {
            FVector Scale = InMarkerTransform.GetScale3D();
            FVector Location = InMarkerTransform.GetLocation();
            FQuat Rotation = InMarkerTransform.GetRotation();
            Location += FVector(0, 0, TargetZ * 0.5f - WallThickness);
            Scale *= VolumeEdgeScale;
            FTransform VolumeMarkerTransform(Rotation, Location, Scale);
            AddMarker(InMarkerName, VolumeMarkerTransform * DungeonTransform);	
        };
    }

    // build walls along the width
    for (int dx = 0; dx < cell.Bounds.Size.X; dx++) {
        int32 x = basePosition.X + dx;
        int32 y = basePosition.Y;
        int32 z = basePosition.Z;

        FTransform Transform = FTransform::Identity;
        FVector Position(x, y, z);

        if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsEdges) {
            Position += FVector(0.5f, 0, 0);
        }
        else if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
            Position += FVector(0.5f, 0.5f, 0);
        }

        Position *= GridToMeshScale;
        Transform.SetLocation(Position);
        Transform.SetRotation(FQuat(FVector(0, 0, 1), PI));
        int32 OffsetZ = 0;

        bool bMakeDoor = (GridModel->GetGridCellLookup(x, y).ContainsDoor && GridModel->GetGridCellLookup(x, y - 1).ContainsDoor);
        Elevation = GetElevation(cell, x, y - 1, OffsetZ);
        OffsetTransformZ(OffsetZ * GridToMeshScale.Z, Transform);

        FString MarkerName = bMakeDoor ? FGridBuilderMarkers::DOOR : FGridBuilderMarkers::WALL;
        FString VoxelMarkerName = bMakeDoor ? FVoxelBuilderMarkers::INTERNAL_VOXEL_DOOR : FVoxelBuilderMarkers::INTERNAL_VOXEL_WALL;
        AddGridMarker(MarkerName, Transform);
        FnEmitVoxelEdgeMarker(VoxelMarkerName, Transform);
        
        AddGridMarker(FGridBuilderMarkers::WALLHALF, Transform, Elevation, HalfWallOffset);

        // Add the pillar
        Transform.SetLocation(FVector(x, y, z) * GridToMeshScale);
        OffsetTransformZ(OffsetZ * GridToMeshScale.Z, Transform);
        AddGridMarker(FGridBuilderMarkers::WALL_SEPARATOR, Transform);
        AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, Transform, Elevation, HalfWallOffset);

        y += cell.Bounds.Size.Y;
        Elevation = GetElevation(cell, x, y, OffsetZ);
        FGridDungeonCellInfo AdjacentCellInfo = GridModel->GetGridCellLookup(x, y);
        bool bContainsAdjacentRoom = AdjacentCellInfo.CellType == EGridDungeonCellType::Room;
        if (!bContainsAdjacentRoom || GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {

            if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsEdges) {
                Position.Y = y * GridToMeshScale.Y;
            }
            else if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
                Position.Y = (y - 0.5f) * GridToMeshScale.Y;
            }

            Transform.SetLocation(Position);
            Transform.SetRotation(FQuat(FVector(0, 0, 1), 0));

            bMakeDoor = (GridModel->GetGridCellLookup(x, y).ContainsDoor && GridModel->GetGridCellLookup(x, y - 1).ContainsDoor);

            bool bEmitDoorWallMarker = true;
            if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
                if (bContainsAdjacentRoom && bMakeDoor) {
                    bEmitDoorWallMarker = false;
                }
            }

            MarkerName = bMakeDoor ? FGridBuilderMarkers::DOOR : FGridBuilderMarkers::WALL;
            OffsetTransformZ(OffsetZ * GridToMeshScale.Z, Transform);
            if (bEmitDoorWallMarker) {
                AddGridMarker(MarkerName, Transform);
            }
            AddGridMarker(FGridBuilderMarkers::WALLHALF, Transform, Elevation, HalfWallOffset);

            // Add the pillar
            Transform.SetLocation(FVector(x + 1, y, z) * GridToMeshScale);
            OffsetTransformZ(OffsetZ * GridToMeshScale.Z, Transform);
            AddGridMarker(FGridBuilderMarkers::WALL_SEPARATOR, Transform);
            AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, Transform, Elevation, HalfWallOffset);
        }
    }

    // build walls along the length
    for (int dy = 0; dy < cell.Bounds.Size.Y; dy++) {
        int32 x = basePosition.X;
        int32 y = basePosition.Y + dy;
        int32 z = basePosition.Z;

        FTransform transform = FTransform::Identity;
        FVector position(x, y, z);
        if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsEdges) {
            position += FVector(0, 0.5f, 0);
        }
        else if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
            position += FVector(0.5f, 0.5f, 0);
        }
        position *= GridToMeshScale;
        transform.SetLocation(position);
        transform.SetRotation(FQuat(FVector(0, 0, 1), PI / 2));
        int32 OffsetZ = 0;

        bool makeDoor = (GridModel->GetGridCellLookup(x, y).ContainsDoor && GridModel
                                                                            ->GetGridCellLookup(x - 1, y).ContainsDoor);
        Elevation = GetElevation(cell, x - 1, y, OffsetZ);

        FString MarkerName = makeDoor ? FGridBuilderMarkers::DOOR : FGridBuilderMarkers::WALL;
        FString VoxelMarkerName = makeDoor ? FVoxelBuilderMarkers::INTERNAL_VOXEL_DOOR : FVoxelBuilderMarkers::INTERNAL_VOXEL_WALL;
        
        OffsetTransformZ(OffsetZ * GridToMeshScale.Z, transform);
        AddGridMarker(MarkerName, transform);
        FnEmitVoxelEdgeMarker(VoxelMarkerName, transform);
        AddGridMarker(FGridBuilderMarkers::WALLHALF, transform, Elevation, HalfWallOffset);

        // Add the pillar
        transform.SetLocation(FVector(x, y + 1, z) * GridToMeshScale);
        OffsetTransformZ(OffsetZ * GridToMeshScale.Z, transform);
        AddGridMarker(FGridBuilderMarkers::WALL_SEPARATOR, transform);
        AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, transform, Elevation, HalfWallOffset);

        x += cell.Bounds.Size.X;
        Elevation = GetElevation(cell, x, y, OffsetZ);
        FGridDungeonCellInfo AdjacentCellInfo = GridModel->GetGridCellLookup(x, y);

        bool bContainsAdjacentRoom = AdjacentCellInfo.CellType == EGridDungeonCellType::Room;
        if (!bContainsAdjacentRoom || GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
            if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsEdges) {
                position.X = x * GridToMeshScale.X;
            }
            else if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
                position.X = (x - 0.5f) * GridToMeshScale.X;
            }
            transform.SetLocation(position);
            transform.SetRotation(FQuat(FVector(0, 0, 1), -PI / 2));

            makeDoor = (GridModel->GetGridCellLookup(x, y).ContainsDoor && GridModel
                                                                           ->GetGridCellLookup(x - 1, y).ContainsDoor);

            bool bEmitDoorWallMarker = true;
            if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
                if (bContainsAdjacentRoom && makeDoor) {
                    bEmitDoorWallMarker = false;
                }
            }


            MarkerName = makeDoor ? FGridBuilderMarkers::DOOR : FGridBuilderMarkers::WALL;
            OffsetTransformZ(OffsetZ * GridToMeshScale.Z, transform);
            if (bEmitDoorWallMarker) {
                AddGridMarker(MarkerName, transform);
            }
            AddGridMarker(FGridBuilderMarkers::WALLHALF, transform, Elevation, HalfWallOffset);

            // Add the pillar
            transform.SetLocation(FVector(x, y, z) * GridToMeshScale);
            OffsetTransformZ(OffsetZ * GridToMeshScale.Z, transform);
            AddGridMarker(FGridBuilderMarkers::WALL_SEPARATOR, transform);
            AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, transform, Elevation, HalfWallOffset);
        }
    }
}

int32 UGridDungeonBuilder::GetElevation(const FGridDungeonCell& baseCell, int32 x, int32 y, int32& OutZOffset) {
    OutZOffset = 0;
    FGridDungeonCellInfo info = GridModel->GetGridCellLookup(x, y);
    int32 Elevation = 0;
    if (info.CellType == EGridDungeonCellType::Unknown) return Elevation;
    FGridDungeonCell* otherCell = GridModel->GetCell(info.CellId);
    if (!otherCell) {
        return Elevation;
    }
    OutZOffset = otherCell->Bounds.Location.Z - baseCell.Bounds.Location.Z;
    Elevation = FMath::Max(Elevation, FMath::Abs(OutZOffset));
    OutZOffset = FMath::Max(0, OutZOffset);

    //return FMath::Max(elevation, baseCell.Bounds.Location.Z - otherCell->Bounds.Location.Z);
    return Elevation;
}

int32 UGridDungeonBuilder::GetStairHeight(const FGridDungeonStairInfo& stair) {
    FGridDungeonCell* owner = GridModel->GetCell(stair.OwnerCell);
    FGridDungeonCell* target = GridModel->GetCell(stair.ConnectedToCell);
    if (!owner || !target) return 1;
    return FMath::Abs(owner->Bounds.Location.Z - target->Bounds.Location.Z);
}


void UGridDungeonBuilder::BuildMesh_Stairs(const FGridDungeonCell& cell) {
    using namespace GridDungeonBuilderImpl;

    // Draw all the stairs registered with this cell
    if (!GridModel->CellStairsLookup.Contains(cell.Id)) {
        // No stairs registered here
        return;
    }

    for (const FGridDungeonStairInfo& Stair : GridModel->CellStairsLookup[cell.Id]) {
        FTransform transform = FTransform::Identity;
        transform.SetLocation(Stair.Position);
        transform.SetRotation(Stair.Rotation);
        const int StairHeight = GetStairHeight(Stair);
        FString StairType = GetStairMarkerName(StairHeight);
        // (stairHeight > 1) ? FGridBuilderMarkers::ST_STAIR2X : FGridBuilderMarkers::ST_STAIR;
        AddGridMarker(StairType, transform);
    }
}

bool UGridDungeonBuilder::ContainsStair(const FGridDungeonCell& baseCell, int32 x, int32 y) {
    FGridDungeonCellInfo info = GridModel->GetGridCellLookup(x, y);
    if (info.CellType == EGridDungeonCellType::Unknown) return false;

    FGridDungeonCell* cell = GridModel->GetCell(info.CellId);
    if (!cell) return false;

    FGridDungeonStairInfo stair;
    if (GetStair(cell->Id, baseCell.Id, stair)) {
        FVector IPosition = (stair.Position / GridToMeshScale);
        int32 ix = FMath::FloorToInt(IPosition.X);
        int32 iy = FMath::FloorToInt(IPosition.Y);
        if (ix == x && iy == y) {
            return true;
        }
    }
    return false;
}

bool UGridDungeonBuilder::ContainsStair(int32 ownerCellId, int32 connectedToCellId) {
    if (!GridModel->CellStairsLookup.Contains(ownerCellId)) {
        return false;
    }

    for (const FGridDungeonStairInfo& stairInfo : GridModel->CellStairsLookup[ownerCellId]) {
        if (stairInfo.ConnectedToCell == connectedToCellId) {
            return true;
        }
    }
    return false;
}

void UGridDungeonBuilder::AddCorridorPadding(const FIntVector& P) {
    AddCorridorPadding(P.X, P.Y, P.Z);
}

void UGridDungeonBuilder::CreateLayoutClusterGroups() {
    GridModel->Clusters.Reset();
    GridModel->BuildCellLookup();
    GenerateAdjacencyLookup();

    int32 GroupIdCounter = 0;

    // Make all rooms as different groups
    for (FGridDungeonCell& Cell : GridModel->Cells) {
        if (Cell.CellType == EGridDungeonCellType::Room) {
            Cell.ClusterId = GroupIdCounter;
            GroupIdCounter++;
        }
    }

    // Flood fill from each corridor cell and form a group
    for (FGridDungeonCell& Cell : GridModel->Cells) {
        if (Cell.ClusterId != -1 || Cell.CellType == EGridDungeonCellType::Unknown) {
            continue;
        }

        if (Cell.CellType == EGridDungeonCellType::Corridor || Cell.CellType == EGridDungeonCellType::CorridorPadding) {
            FloodFillCorridorGroup(Cell, GroupIdCounter);
            GroupIdCounter++;
        }
    }

    // Save the cluster data
    for (const FGridDungeonCell& Cell : GridModel->Cells) {
        if (Cell.ClusterId == INDEX_NONE) {
            continue;
        }
        FGridDungeonCellClusterInfo& ClusterInfo = GridModel->Clusters.FindOrAdd(Cell.ClusterId);
        ClusterInfo.Id = Cell.ClusterId;
        ClusterInfo.CellIds.Add(Cell.Id);
    }

    // Generate the boundary for the clusters
    for (auto& Entry : GridModel->Clusters) {
        int ClusterId = Entry.Key;
        TArray<FRectangle> ClusterRects;
        FGridDungeonCellClusterInfo& ClusterInfo = Entry.Value;
        for (const int32 CellId : ClusterInfo.CellIds) {
            if (const FGridDungeonCell* GridDungeonCell = GridModel->GetCell(CellId)) {
                ClusterRects.Add(GridDungeonCell->Bounds);
            }
        }

        ClusterInfo.Boundary = GridDungeonBuilderImpl::GenerateBoundaryForConnectedRects(ClusterRects);
        ClusterInfo.GridZ = ClusterRects.Num() > 0 ? ClusterRects[0].Location.Z : 0;
    }
}

void UGridDungeonBuilder::FloodFillCorridorGroup(FGridDungeonCell& StartCell, int32 GroupId) {
    TQueue<int32> Queue;
    TSet<int32> Visited;
    Queue.Enqueue(StartCell.Id);

    int32 TopId;
    while (Queue.Dequeue(TopId)) {
        if (Visited.Contains(TopId)) {
            continue;
        }
        Visited.Add(StartCell.Id);

        FGridDungeonCell* Top = GridModel->GetCell(TopId);
        if (!Top) continue;
        Top->ClusterId = GroupId;

        // Grab the adjacent corridor cells and add them to the stack
        for (int32 AdjacentCellId : Top->AdjacentCells) {
            if (Visited.Contains(AdjacentCellId)) {
                continue;
            }

            FGridDungeonCell* AdjacentCell = GridModel->GetCell(AdjacentCellId);
            if (!AdjacentCell) continue;
            if (AdjacentCell->ClusterId != -1) continue;
            if (AdjacentCell->CellType != EGridDungeonCellType::Corridor && AdjacentCell->CellType != EGridDungeonCellType::CorridorPadding) {
                // Not a corridor cell
                continue;
            }

            // Check if the adjacent cells are traversable
            bool bTraversable = true;
            if (Top->Bounds.Location.Z != AdjacentCell->Bounds.Location.Z) {
                if (GridConfig->bClusterWithHeightVariation) {
                    // There is a height variation.  Check if these two cells have a stair connection
                    if (!GridModel->ContainsStairBetweenCells(TopId, AdjacentCellId)) {
                        // There is a height variation and no stairs connecting these cells
                        bTraversable = false;
                    }
                }
                else {
                    // Height variations are not allowed while clustering
                    bTraversable = false;
                }
            }

            if (bTraversable) {
                Queue.Enqueue(AdjacentCellId);
            }
        }
    }
}

bool UGridDungeonBuilder::CanDrawFence(const FGridDungeonCell& baseCell, int32 x, int32 y, bool& isElevatedFence, bool& drawPillar, int& elevationHeight) {
    using namespace GridDungeonBuilderImpl;

    FGridDungeonCellInfo info = GridModel->GetGridCellLookup(x, y);
    isElevatedFence = false;
    drawPillar = false;
    elevationHeight = 0;
    if (info.CellType == EGridDungeonCellType::Unknown) {
        isElevatedFence = false;
        drawPillar = true;
        return true;
    }
    bool bCanDrawFence = false;
    FGridDungeonCell* otherCell = GridModel->GetCell(info.CellId);
    if (otherCell->Bounds.Location.Z < baseCell.Bounds.Location.Z) {
        isElevatedFence = true;
        elevationHeight = baseCell.Bounds.Location.Z - otherCell->Bounds.Location.Z;
        drawPillar = true;
        // Make sure we don't have a stair between the two cells
        if (!ContainsStair(baseCell, x, y)) {
            bCanDrawFence = true;
        }
    }

    // If the two cells are a room / corridor combo, we don't want a fence here
    if (IsRoomCorridor(baseCell.CellType, info.CellType)) {
        isElevatedFence = false;
        elevationHeight = 0;
        bCanDrawFence = false;
    }
    return bCanDrawFence;
}

void UGridDungeonBuilder::BuildMesh_Corridor_BlockWalls(const FGridDungeonCell& cell) {
    BuildMesh_Floor(cell);

    TSet<FIntVector> FencePositions;
    FRectangle Border = cell.Bounds;
    Border.ExpandBy(1);

    TArray<FIntVector> BorderPoints;
    Border.GetBorderPoints(BorderPoints);

    for (const FIntVector& P : BorderPoints) {
        if (FencePositions.Contains(P)) {
            continue;
        }

        FGridDungeonCellInfo CellInfo = GridModel->GetGridCellLookup(P.X, P.Y);
        if (CellInfo.CellType == EGridDungeonCellType::Unknown) {
            FencePositions.Add(P);
        }
    }

    for (const FIntVector& P : FencePositions) {
        FIntVector GridPosition = P;

        FVector Position = FDAMathUtils::ToVector(P);
        Position += FVector(0.5f, 0.5f, 0);
        Position *= GridConfig->GridCellSize;
        FTransform Transform(FRotator::ZeroRotator, Position);
        AddGridMarker(FGridBuilderMarkers::FENCE, Transform);
    }
}

void UGridDungeonBuilder::BuildMesh_Corridor(const FGridDungeonCell& cell) {
    BuildMesh_Floor(cell);

    FVector HalfWallOffset = GridToMeshScale * FVector(0, 0, -1);
    FIntVector basePosition = cell.Bounds.Location;
    // build fence along the width
    for (int dx = 0; dx < cell.Bounds.Size.X; dx++) {
        int32 x = basePosition.X + dx;
        int32 y = basePosition.Y;
        int32 z = basePosition.Z;

        int elevationHeight;
        bool isElevatedFence, drawPillar;
        bool drawFence = CanDrawFence(cell, x, y - 1, isElevatedFence, drawPillar, elevationHeight);
        FTransform transform = FTransform::Identity;
        if (drawFence || isElevatedFence) {
            FVector position(x, y, z);
            position += FVector(0.5f, 0, 0);
            position *= GridToMeshScale;
            transform.SetLocation(position);
            transform.SetRotation(FQuat(FVector(0, 0, 1), 0));
            if (drawFence) {
                AddGridMarker(FGridBuilderMarkers::FENCE, transform);
            }
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF, transform, elevationHeight, HalfWallOffset);
            }
        }
        if (drawFence || drawPillar) {
            transform.SetLocation(FVector(x, y, z) * GridToMeshScale);
            AddGridMarker(FGridBuilderMarkers::FENCE_SEPARATOR, transform);
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, transform, elevationHeight, HalfWallOffset);
            }
        }


        y += cell.Bounds.Size.Y;
        drawFence = CanDrawFence(cell, x, y, isElevatedFence, drawPillar, elevationHeight);
        transform = FTransform::Identity;
        if (drawFence || isElevatedFence) {
            FVector position(x, y, z);
            position += FVector(0.5f, 0, 0);
            position *= GridToMeshScale;
            transform.SetLocation(position);
            transform.SetRotation(FQuat(FVector(0, 0, 1), PI));
            if (drawFence) {
                AddGridMarker(FGridBuilderMarkers::FENCE, transform);
            }
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF, transform, elevationHeight, HalfWallOffset);
            }
        }
        if (drawFence || drawPillar) {
            transform.SetLocation(FVector(x + 1, y, z) * GridToMeshScale);
            AddGridMarker(FGridBuilderMarkers::FENCE_SEPARATOR, transform);
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, transform, elevationHeight, HalfWallOffset);
            }
        }

    }

    // build fence along the length
    for (int dy = 0; dy < cell.Bounds.Size.Y; dy++) {
        int32 x = basePosition.X;
        int32 y = basePosition.Y + dy;
        int32 z = basePosition.Z;

        int elevationHeight;
        bool isElevatedFence, drawPillar;
        bool drawFence = CanDrawFence(cell, x - 1, y, isElevatedFence, drawPillar, elevationHeight);
        FTransform transform = FTransform::Identity;
        if (drawFence || isElevatedFence) {
            FVector position(x, y, z);
            position += FVector(0, 0.5f, 0);
            position *= GridToMeshScale;
            transform.SetLocation(position);
            transform.SetRotation(FQuat(FVector(0, 0, 1), -PI / 2));
            if (drawFence) {
                AddGridMarker(FGridBuilderMarkers::FENCE, transform);
            }
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF, transform, elevationHeight, HalfWallOffset);
            }
        }
        if (drawFence || drawPillar) {
            transform.SetLocation(FVector(x, y + 1, z) * GridToMeshScale);
            AddGridMarker(FGridBuilderMarkers::FENCE_SEPARATOR, transform);
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, transform, elevationHeight, HalfWallOffset);
            }
        }

        x += cell.Bounds.Size.X;
        drawFence = CanDrawFence(cell, x, y, isElevatedFence, drawPillar, elevationHeight);
        transform = FTransform::Identity;
        if (drawFence || isElevatedFence) {
            FVector position(x, y, z);
            position += FVector(0, 0.5f, 0);
            position *= GridToMeshScale;
            transform.SetLocation(position);
            transform.SetRotation(FQuat(FVector(0, 0, 1), PI / 2));
            if (drawFence) {
                AddGridMarker(FGridBuilderMarkers::FENCE, transform);
            }
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF, transform, elevationHeight, HalfWallOffset);
            }
        }
        if (drawFence || drawPillar) {
            transform.SetLocation(FVector(x, y, z) * GridToMeshScale);
            AddGridMarker(FGridBuilderMarkers::FENCE_SEPARATOR, transform);
            if (isElevatedFence) {
                AddGridMarker(FGridBuilderMarkers::WALLHALF_SEPARATOR, transform, elevationHeight, HalfWallOffset);
            }
        }
    }
}

void UGridDungeonBuilder::EmitDungeonMarkers_Implementation() {
    Super::EmitDungeonMarkers_Implementation();

    GridModel = Cast<UGridDungeonModel>(DungeonModel);
    GridConfig = Cast<UGridDungeonConfig>(DungeonConfig);
    GridQuery = Cast<UGridDungeonQuery>(DungeonQuery);

    if (GridModel->Cells.Num() == 0) return;
    ClearMarkerList();
    TArray<FDungeonMarkerInstance>& WorldMarkers = DungeonModel->WorldMarkers;

    // Populate the marker all over the map
    for (const FGridDungeonCell& cell : GridModel->Cells) {
        int32 StartMarkerIndex = WorldMarkers.Num();
        switch (cell.CellType) {
        case EGridDungeonCellType::Room:
            BuildMesh_Room(cell);
            BuildMesh_RoomDecoration(cell);
            break;

        case EGridDungeonCellType::Corridor:
        case EGridDungeonCellType::CorridorPadding:
            {
                if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsEdges) {
                    BuildMesh_Corridor(cell);
                }
                else if (GridConfig->WallLayoutType == EGridDungeonWallType::WallsAsTileBlocks) {
                    BuildMesh_Corridor_BlockWalls(cell);
                }
            }
            break;
        default:
            break;
        }
        BuildMesh_Stairs(cell);

        int32 EndMarkerIndex = WorldMarkers.Num();
        // Set the cell id on the inserted markers for this cell
        for (int i = StartMarkerIndex; i < EndMarkerIndex; i++) {
            WorldMarkers[i].UserData = MakeShareable(new FGridBuilderMarkerUserData(cell.Id, cell.ClusterId));
        }
    }

    if (GridConfig->bEnableClusteredTheming) {
        AssignClusteredThemesOnMarkers();
    }

    // Mirror the dungeon based on the mirror volumes placed in the level
    MirrorDungeon();
}

void UGridDungeonBuilder::AssignClusteredThemesOnMarkers() {
    if (!Dungeon) return;
    TMap<FString, FClusterThemeInfo> ClusterThemesByName;
    for (const FClusterThemeInfo& ClusterTheme : Dungeon->ClusterThemes) {
        if (ClusterThemesByName.Contains(ClusterTheme.ClusterThemeName)) {
            UE_LOG(GridDungeonBuilderLog, Warning, TEXT("Duplicate cluster theme name found [%s]. Ignoring this theme"),
                   *ClusterTheme.ClusterThemeName);
            continue;
        }
        ClusterThemesByName.Add(ClusterTheme.ClusterThemeName, ClusterTheme);
    }

    TMap<int32, FString> ThemeNamesByClusterId;
    for (FDungeonMarkerInstance& Marker : DungeonModel->WorldMarkers) {
        // Grab the user data we set in the EmitDungeonMarkers_Implementation function
        int32 CellId = -1;
        int32 ClusterId = -1;
        if (Marker.UserData.IsValid()) {
            TSharedPtr<FGridBuilderMarkerUserData> GridUserData = StaticCastSharedPtr<FGridBuilderMarkerUserData>(
                Marker.UserData);
            CellId = GridUserData->CellId;
            ClusterId = GridUserData->ClusterId;
        }

        if (ClusterId == -1) {
            // Not part of a cluster
            continue;
        }

        FString ClusterThemeName;
        if (!ThemeNamesByClusterId.Contains(ClusterId)) {
            // We don't have a theme override for this cluster id. Assign one
            ClusterThemeName = GetClusterThemeForId(ClusterId, CellId);
            ThemeNamesByClusterId.Add(ClusterId, ClusterThemeName);
        }
        else {
            ClusterThemeName = ThemeNamesByClusterId[ClusterId];
        }

        Marker.ClusterThemeOverride = ClusterThemeName;
    }
}

FString UGridDungeonBuilder::GetClusterThemeForId(int32 ClusterId, int32 CellId) {
    if (!Dungeon || ClusterId == -1) {
        return FString();
    }

    int32 NumThemes = Dungeon->ClusterThemes.Num();
    if (NumThemes == 0) {
        return FString();
    }

    // TODO: Apply user defined constraints 

    int32 Index = Random.RandRange(0, NumThemes - 1);
    return Dungeon->ClusterThemes[Index].ClusterThemeName;
}

