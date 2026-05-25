//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SimpleCity/SimpleCityBuilder.h"

#include "Builders/SimpleCity/SimpleCityConfig.h"
#include "Builders/SimpleCity/SimpleCityModel.h"
#include "Builders/SimpleCity/SimpleCityQuery.h"
#include "Builders/SimpleCity/SimpleCitySelectorLogic.h"
#include "Builders/SimpleCity/SimpleCityToolData.h"
#include "Builders/SimpleCity/SimpleCityTransformLogic.h"
#include "Builders/SimpleCity/Utils/SimpleCityRoadBeautifier.h"
#include "Core/Dungeon.h"
#include "Core/Markers/DungeonMarkerNames.h"
#include "Core/Markers/DungeonMarkerVisualization.h"
#include "Core/Utils/DungeonModelHelper.h"
#include "Frameworks/MarkerGenerator/Impl/Grid/MarkerGenGridProcessor.h"

#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(SimpleCityBuilderLog);
#define CITY_INDEX(x, y) ((y) * CityModel->CityWidth + (x))


void USimpleCityBuilder::BuildDungeonImpl(UWorld* World) {

    if (!CityModel) {
        UE_LOG(SimpleCityBuilderLog, Error, TEXT("Invalid dungeon model provided to the grid builder"));
        return;
    }

    if (!CityConfig) {
        UE_LOG(SimpleCityBuilderLog, Error, TEXT("Invalid dungeon gridConfig provided to the grid builder"));
        return;
    }

    GenerateCityLayout();
}

void USimpleCityBuilder::InitializeState() {
    Super::InitializeState();
    
    CityModel = Cast<USimpleCityModel>(DungeonModel);
    CityConfig = Cast<USimpleCityConfig>(DungeonConfig);
}

void USimpleCityBuilder::GenerateCityLayout() {
    CityModel->CityWidth = Random.RandRange(CityConfig->MinCitySize, CityConfig->MaxCitySize);
    CityModel->CityLength = Random.RandRange(CityConfig->MinCitySize, CityConfig->MaxCitySize);

    const int32 Width = CityModel->CityWidth;
    const int32 Length = CityModel->CityLength;

    int32 NumCells = Width * Length;
    CityModel->Cells.Reset();
    CityModel->Cells.AddDefaulted(NumCells);

    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < Length; y++) {
            FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(x, y)];
            Cell.Position = FIntVector(x, y, 0);
            Cell.CellType = ESimpleCityCellType::House; // Later we will overwrite some with roads
            Cell.Rotation = GetRandomRotation();
        }
    }

    // Build a road network by removing some houses 
    // First build roads along the edge of the map
    for (int x = 0; x < Width; x++) {
        MakeRoad(x, 0);
        MakeRoad(x, Length - 1);
    }
    for (int z = 0; z < Length; z++) {
        MakeRoad(0, z);
        MakeRoad(Width - 1, z);
    }

    // Create roads in-between
    for (int x = GetRandomBlockSize() + 1; x < Width; x += GetRandomBlockSize() + 1) {
        if (Width - x <= 2) continue;
        for (int z = 0; z < Length; z++) {
            MakeRoad(x, z);
        }
    }

    for (int z = GetRandomBlockSize() + 1; z < Length; z += GetRandomBlockSize() + 1) {
        if (Length - z <= 2) continue;
        for (int x = 0; x < Width; x++) {
            MakeRoad(x, z);
        }
    }

    // Remove roads to create a non-uniform city
    RemoveRoadEdges();

    // Face the houses toward the road, or convert them to parks
    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < Length; y++) {
            FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(x, y)];
            if (Cell.CellType == ESimpleCityCellType::House) {
                FaceHouseTowardsRoad(Cell);
            }
        }
    }

    // Insert larger houses (with user defined dimensions)
    for (int x = 0; x < Width; x++) {
        for (int z = 0; z < Length; z++) {
            // Iterate through each custom block dimension
            for (const FCityBlockDimension& BlockDimension : CityConfig->CityBlockDimensions) {
                bool bProcess = Random.FRand() < BlockDimension.Probability;
                if (bProcess) {
                    int32 BlockWidth = BlockDimension.SizeX;
                    int32 BlockHeight = BlockDimension.SizeY;

                    auto InsertHouse = [&]() {
                        if (CanContainBiggerHouse(x, z, BlockWidth, BlockHeight)) {
                            if (Random.FRand() < CityConfig->BiggerHouseProbability) {
                                InsertBiggerHouse(x, z, BlockWidth, BlockHeight, 0, BlockDimension.MarkerName);
                            }
                        }
                    };


                    auto InsertHouse90 = [&]() {
                        // Try the 90 degrees rotated version
                        if (CanContainBiggerHouse(x, z, BlockHeight, BlockWidth)) {
                            if (Random.FRand() < CityConfig->BiggerHouseProbability) {
                                InsertBiggerHouse(x, z, BlockHeight, BlockWidth, 90, BlockDimension.MarkerName);
                            }
                        }
                    };

                    if (Random.FRand() < 0.5f) {
                        InsertHouse();
                        InsertHouse90();
                    }
                    else {
                        InsertHouse90();
                        InsertHouse();
                    }
                }
            }
        }
    }
}

void USimpleCityBuilder::FaceHouseTowardsRoad(FSimpleCityCell& Cell) {
    int x = Cell.Position.X;
    int y = Cell.Position.Y;

    bool roadLeft = GetCellType(x - 1, y) == ESimpleCityCellType::Road;
    bool roadRight = GetCellType(x + 1, y) == ESimpleCityCellType::Road;
    bool roadTop = GetCellType(x, y - 1) == ESimpleCityCellType::Road;
    bool roadBottom = GetCellType(x, y + 1) == ESimpleCityCellType::Road;

    if (!roadLeft && !roadRight && !roadTop && !roadBottom) {
        Cell.CellType = ESimpleCityCellType::Park;
        // interior
        return;
    }

    float angle = 0;
    if (roadLeft) angle = 180;
    else if (roadRight) angle = 0;
    else if (roadTop) angle = 270;
    else if (roadBottom) angle = 90;

    Cell.Rotation = FQuat::MakeFromEuler(FVector(0, 0, angle));
}

void USimpleCityBuilder::RemoveRoadEdges() {
    const int32 Width = CityModel->CityWidth;
    const int32 Length = CityModel->CityLength;
    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < Length; y++) {
            if (IsStraightRoad(x, y)) {
                bool bRemove = Random.FRand() < CityConfig->RoadEdgeRemovalProbability;
                if (bRemove) {
                    RemoveRoadEdge(x, y);
                }
            }
        }
    }

    // Remove the isolated road cells
    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < Length; y++) {
            if (GetCellType(x, y) == ESimpleCityCellType::Road) {
                int32 Adjacent = 0;
                if (GetCellType(x, y - 1) == ESimpleCityCellType::Road) Adjacent++;
                if (GetCellType(x, y + 1) == ESimpleCityCellType::Road) Adjacent++;
                if (GetCellType(x - 1, y) == ESimpleCityCellType::Road) Adjacent++;
                if (GetCellType(x + 1, y) == ESimpleCityCellType::Road) Adjacent++;
                if (Adjacent == 0) {
                    // No adjacent roads connecting to this road cell. remove it
                    FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(x, y)];
                    Cell.CellType = ESimpleCityCellType::House;
                }
            }
        }
    }

}

bool USimpleCityBuilder::IsStraightRoad(int x, int y) {
    if (GetCellType(x, y) != ESimpleCityCellType::Road) {
        return false;
    }

    bool bTop = GetCellType(x, y - 1) == ESimpleCityCellType::Road;
    bool bBottom = GetCellType(x, y + 1) == ESimpleCityCellType::Road;
    bool bLeft = GetCellType(x - 1, y) == ESimpleCityCellType::Road;
    bool bRight = GetCellType(x + 1, y) == ESimpleCityCellType::Road;

    bool bHorizontal = bLeft && bRight;
    bool bVertical = bTop && bBottom;

    int32 Adjacent = 0;
    if (bTop) Adjacent++;
    if (bBottom) Adjacent++;
    if (bLeft) Adjacent++;
    if (bRight) Adjacent++;

    if (Adjacent != 2) return false;

    return bHorizontal || bVertical;
}

void USimpleCityBuilder::RemoveRoadEdge(int x, int y) {
    if (!IsStraightRoad(x, y)) {
        // Nothing to remove
        return;
    }

    TSet<FIntVector> RoadsToRemove;
    RoadsToRemove.Add(FIntVector(x, y, 0));
    int index = x - 1;
    while (IsStraightRoad(index, y)) {
        RoadsToRemove.Add(FIntVector(index, y, 0));
        index--;
    }
    index = x + 1;
    while (IsStraightRoad(index, y)) {
        RoadsToRemove.Add(FIntVector(index, y, 0));
        index++;
    }

    index = y - 1;
    while (IsStraightRoad(x, index)) {
        RoadsToRemove.Add(FIntVector(x, index, 0));
        index--;
    }
    index = y + 1;
    while (IsStraightRoad(x, index)) {
        RoadsToRemove.Add(FIntVector(x, index, 0));
        index++;
    }

    for (const FIntVector& Position : RoadsToRemove) {
        FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(Position.X, Position.Y)];
        Cell.CellType = ESimpleCityCellType::House;
    }

}

ESimpleCityCellType USimpleCityBuilder::GetCellType(int x, int y) {
    if (x < 0 || y < 0 || x >= CityModel->CityWidth || y >= CityModel->CityLength) {
        return ESimpleCityCellType::Empty;
    }
    return CityModel->Cells[CITY_INDEX(x, y)].CellType;
}

FQuat USimpleCityBuilder::GetRandomRotation() {
    float Angle = Random.RandRange(0, 3) * 90;
    return FQuat::MakeFromEuler(FVector(0, 0, Angle));
}

int USimpleCityBuilder::GetRandomBlockSize() {
    return Random.RandRange(CityConfig->MinBlockSize, CityConfig->MaxBlockSize);
}

bool USimpleCityBuilder::CanContainBiggerHouse(int x, int y, int w, int h) {
    for (int dx = 0; dx < w; dx++) {
        for (int dy = 0; dy < h; dy++) {
            int32 xx = x + dx;
            int32 yy = y + dy;
            ESimpleCityCellType CellType = ESimpleCityCellType::Empty;
            if (xx < CityModel->CityWidth && yy < CityModel->CityLength) {
                CellType = CityModel->Cells[CITY_INDEX(x + dx, y + dy)].CellType;
            }

            if (CellType != ESimpleCityCellType::House && CellType != ESimpleCityCellType::Park) {
                return false;
            }
        }
    }
    return true;
}

void USimpleCityBuilder::InsertBiggerHouse(int x, int y, int w, int h, float Angle, const FString& MarkerName) {
    for (int dx = 0; dx < w; dx++) {
        for (int dy = 0; dy < h; dy++) {
            FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(x + dx, y + dy)];
            if (dx == 0 && dy == 0) {
                Cell.CellType = ESimpleCityCellType::UserDefined;
                Cell.Rotation = FQuat::MakeFromEuler(FVector(0, 0, Angle));
                Cell.BlockSize = FIntVector(w, h, 0);
                Cell.MarkerNameOverride = MarkerName;
            }
            else {
                // Make these cells empty, as they will be occupied by the bigger house and we don't want any markers here
                Cell.CellType = ESimpleCityCellType::Empty;
            }
        }
    }
}

void USimpleCityBuilder::MakeRoad(int32 x, int32 y) {
    FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(x, y)];
    Cell.CellType = ESimpleCityCellType::Road;
    Cell.Rotation = FQuat::Identity;
}

void USimpleCityBuilder::EmitDungeonMarkers_Implementation() {
    Super::EmitDungeonMarkers_Implementation();

    CityModel = Cast<USimpleCityModel>(DungeonModel);
    CityConfig = Cast<USimpleCityConfig>(DungeonConfig);

    ClearMarkerList();

    //FVector BasePosition = Dungeon ? Dungeon->GetActorLocation() : FVector::ZeroVector;
    const FTransform BaseTransform = Dungeon ? Dungeon->GetActorTransform() : FTransform::Identity;
    
    auto& cells = CityModel->Cells;
    const int32 width = CityModel->CityWidth;
    const int32 length = CityModel->CityLength;
    const FVector CellSize = FVector(CityConfig->CellSize.X, CityConfig->CellSize.Y, 0);


    for (int x = 0; x < width; x++) {
        for (int y = 0; y < length; y++) {
            FSimpleCityCell& Cell = CityModel->Cells[CITY_INDEX(x, y)];
            FVector CellPositionLogical = UDungeonModelHelper::MakeVector(Cell.Position);
            FVector BlockSizeLogical = UDungeonModelHelper::MakeVector(Cell.BlockSize);
            FVector WorldPosition = (CellPositionLogical + FVector(0.5f, 0.5f, 0)) * CellSize;
            FString MarkerName = "Unknown";
            FQuat Rotation = FQuat::Identity;
            if (Cell.CellType == ESimpleCityCellType::House) {
                MarkerName = FSimpleCityBuilderMarkers::HOUSE;
                Rotation = Cell.Rotation;
            }
            if (Cell.CellType == ESimpleCityCellType::Park) {
                MarkerName = FSimpleCityBuilderMarkers::PARK;
                Rotation = Cell.Rotation;
            }
            else if (Cell.CellType == ESimpleCityCellType::UserDefined) {
                MarkerName = Cell.MarkerNameOverride;
                WorldPosition = (CellPositionLogical + BlockSizeLogical / 2.0f) * CellSize;
                Rotation = Cell.Rotation;
            }
            else if (Cell.CellType == ESimpleCityCellType::Road) {
                float Angle = 0;
                MarkerName = FSimpleCityRoadBeautifier::GetRoadMarkerName(x, y, CityModel, Angle);
                Rotation = FQuat::MakeFromEuler(FVector(0, 0, Angle));
            }
            FTransform MarkerTransform = FTransform(Rotation, WorldPosition) * BaseTransform;
            AddMarker(MarkerName, MarkerTransform);
        }
    }
}


void USimpleCityBuilder::DrawDebugData(UWorld* InWorld, bool bPersistant /*= false*/, float lifeTime /*= 0*/) {
    if (!CityModel || !CityConfig) return;

    const float DebugCellHeight = 10;
    FVector BasePosition = Dungeon ? Dungeon->GetActorLocation() : FVector::ZeroVector;
    FVector CellSize = FVector(CityConfig->CellSize.X, CityConfig->CellSize.Y, DebugCellHeight);
    FVector HalfCellSize = CellSize / 2.0f;

    for (const FSimpleCityCell& cell : CityModel->Cells) {
        FVector Location = UDungeonModelHelper::MakeVector(cell.Position) * CellSize + BasePosition;

        FColor Color = FColor::White;
        switch (cell.CellType) {

        case ESimpleCityCellType::Road:
            Color = FColor::Black;
            break;

        case ESimpleCityCellType::House:
            Color = FColor::Red;
            break;

        case ESimpleCityCellType::UserDefined:
            Color = FColor::Purple;
            break;

        default:
            Color = FColor::White;
            break;
        }

        FColor SolidColor = Color;
        SolidColor.A = 32;

        DrawDebugSolidBox(InWorld, Location, HalfCellSize, SolidColor, bPersistant, lifeTime);
        DrawDebugBox(InWorld, Location, HalfCellSize, Color, bPersistant, lifeTime);

    }
}

void USimpleCityBuilder::MirrorDungeon() {

}

TSubclassOf<UDungeonModel> USimpleCityBuilder::GetModelClass() {
    return USimpleCityModel::StaticClass();
}

TSubclassOf<UDungeonConfig> USimpleCityBuilder::GetConfigClass() {
    return USimpleCityConfig::StaticClass();
}

TSubclassOf<UDungeonToolData> USimpleCityBuilder::GetToolDataClass() {
    return USimpleCityToolData::StaticClass();
}

TSubclassOf<UDungeonQuery> USimpleCityBuilder::GetQueryClass() {
    return USimpleCityQuery::StaticClass();
}

void USimpleCityBuilder::GetDefaultMarkerNames(TArray<FString>& OutMarkerNames) {
    OutMarkerNames.Reset();
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::HOUSE);
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::PARK);
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::ROAD_X);
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::ROAD_T);
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::ROAD_S);
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::ROAD_CORNER);
    OutMarkerNames.Add(FSimpleCityBuilderMarkers::ROAD_END);
}

void USimpleCityBuilder::CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules) {
    Super::CreateMarkerVisualizationRules(OutRules);
    OutRules = {};
    
    if (!CityConfig) {
        CityConfig = Cast<USimpleCityConfig>(DungeonConfig);
    }
    
    OutRules.SetGridSize(CityConfig ? FVector(CityConfig->CellSize, 0) : FVector::ZeroVector);
    
    OutRules.Set(FSimpleCityBuilderMarkers::HOUSE, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::PARK, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::ROAD_X, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::ROAD_T, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::ROAD_S, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::ROAD_CORNER, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::ROAD_END, { EDungeonMarkerVisualizationShape::Tile });
    OutRules.Set(FSimpleCityBuilderMarkers::ROAD, { EDungeonMarkerVisualizationShape::Tile });
}

TSharedPtr<IMarkerGenProcessor> USimpleCityBuilder::CreateMarkerGenProcessor(const FTransform& InDungeonTransform) const {
    if (const USimpleCityConfig* Config = CityConfig ? CityConfig.Get() : Cast<USimpleCityConfig>(DungeonConfig)) {
        const FVector2D GridSize = Config->CellSize;
        return MakeShareable(new FMarkerGenGridProcessor(InDungeonTransform, FVector(GridSize, 1)));
    }
    return nullptr;
}

void USimpleCityBuilder::MirrorDungeonWithVolume(ADungeonMirrorVolume* MirrorVolume) {
}

bool USimpleCityBuilder::PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
    return false;
}

FTransform USimpleCityBuilder::PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) {
    return FTransform::Identity;
}

#undef CITY_INDEX

