//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SnapMap/SnapMapDungeonBuilder.h"

#include "Builders/SnapMap/SnapMapAsset.h"
#include "Builders/SnapMap/SnapMapDungeonModel.h"
#include "Builders/SnapMap/SnapMapDungeonQuery.h"
#include "Builders/SnapMap/SnapMapDungeonSelectorLogic.h"
#include "Builders/SnapMap/SnapMapDungeonToolData.h"
#include "Builders/SnapMap/SnapMapDungeonTransformLogic.h"
#include "Core/Dungeon.h"
#include "Core/Utils/DungeonUtils.h"
#include "Core/Volumes/DungeonNegationVolume.h"
#include "Core/Volumes/DungeonThemeOverrideVolume.h"
#include "Frameworks/GraphGrammar/GraphGrammar.h"
#include "Frameworks/GraphGrammar/GraphGrammarProcessor.h"
#include "Frameworks/GraphGrammar/Script/GrammarScriptGraph.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"
#include "Frameworks/Snap/Lib/Streaming/SnapStreaming.h"
#include "Frameworks/Snap/Lib/Utils/SnapDiagnostics.h"
#include "Frameworks/Snap/SnapMap/SnapMapGraphSerialization.h"
#include "Frameworks/Snap/SnapMap/SnapMapLibrary.h"
#include "Frameworks/ThemeEngine/SceneProviders/Providers/PooledDungeonSceneProvider.h"

#include "DrawDebugHelpers.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(SnapMapDungeonBuilderLog);

USnapMapDungeonBuilder::USnapMapDungeonBuilder(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
}

void USnapMapDungeonBuilder::BuildDungeonImpl(UWorld* World) {
    SnapMapModel = Cast<USnapMapDungeonModel>(DungeonModel);
    SnapMapConfig = Cast<USnapMapDungeonConfig>(DungeonConfig);

    if (!World || !SnapMapConfig.IsValid() || !SnapMapModel.IsValid()) {
        UE_LOG(SnapMapDungeonBuilderLog, Error, TEXT("Invalid reference passed to the snap builder"));
        return;
    }

    USnapMapAsset* FlowGraph = SnapMapConfig->DungeonFlowGraph.LoadSynchronous();
    if (!FlowGraph) {
        UE_LOG(SnapMapDungeonBuilderLog, Error, TEXT("Dungeon Flow asset not specified"));
        return;
    }

    USnapMapModuleDatabase* ModuleDB = SnapMapConfig->ModuleDatabase.LoadSynchronous();
    if (!ModuleDB) {
        UE_LOG(SnapMapDungeonBuilderLog, Error, TEXT("Module Database asset is not specified"));
        return;
    }

    const int32 MaxTries = SnapMapConfig->bSupportBuildRetries ? FMath::Max(1, SnapMapConfig->NumBuildRetries) : 1;
    int32 NumTries = 0;

    TSet<int32> ProcessedSeeds;
    int32 Seed = SnapMapConfig->Seed;
    SnapLib::FModuleNodePtr GraphRootNode = nullptr;
    while (NumTries < MaxTries && !GraphRootNode.IsValid()) {
        ProcessedSeeds.Add(Seed);
        GraphRootNode = GenerateModuleNodeGraph(Seed);
        NumTries++;

        if (!GraphRootNode.IsValid()) {
            FRandomStream SeedGenerator(Seed);
            Seed = SeedGenerator.RandRange(0, MAX_int32 - 1);
            while (ProcessedSeeds.Contains(Seed)) {
                Seed++;
            }
        }
    }

    if (!GraphRootNode) {
        UE_LOG(SnapMapDungeonBuilderLog, Error, TEXT("Cannot find a valid snap solution"));
        return;
    }
    
    FSnapMapGraphSerializer::Serialize({ GraphRootNode }, {}, SnapMapModel->ModuleInstances, SnapMapModel->Connections, SnapMapModel->Walls);

    if (!BuilderParameters.bLayoutPreviewBuildOnly) {
        if (UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
            LevelStreamingModel->Release(GetWorld());
        }

        FGuid SpawnRoomId = GraphRootNode->ModuleInstanceId;
        FSnapStreamingLib::GenerateLevelStreamingModel(World, { GraphRootNode }, Dungeon->LevelStreamingModel, USnapMapStreamingChunk::StaticClass(),
            [this, &SpawnRoomId](UDungeonStreamingChunk* InChunk, SnapLib::FModuleNodePtr Node)  {
                USnapStreamingChunkBase* Chunk = Cast<USnapStreamingChunkBase>(InChunk);
                if (Chunk) {
                    Chunk->ModuleTransform = Node->WorldTransform;
                    if (Node->ModuleInstanceId == SpawnRoomId) {
                        Chunk->bAlwaysResident = true;
                    }
                }
            });
    }
}



SnapLib::FModuleNodePtr USnapMapDungeonBuilder::GenerateModuleNodeGraph(int32 InSeed) const {
    UGrammarScriptGraph* MissionGraph = NewObject<UGrammarScriptGraph>(SnapMapModel.Get());
    SnapMapModel->MissionGraph = MissionGraph;

    // build the mission graph from the mission grammar rules
    {
        UGraphGrammar* MissionGrammar = SnapMapConfig->DungeonFlowGraph->MissionGrammar;
        FGraphGrammarProcessor GraphGrammarProcessor;
        GraphGrammarProcessor.Initialize(MissionGraph, MissionGrammar, InSeed);
        GraphGrammarProcessor.Execute(MissionGraph, MissionGrammar);
    }

    SnapLib::FGrowthStaticState StaticState{};
    StaticState.Random = Random;
    StaticState.BoundsContraction = SnapMapConfig->CollisionTestContraction;
    StaticState.DungeonBaseTransform = FTransform::Identity;
    StaticState.StartTimeSecs = FPlatformTime::Seconds();
    StaticState.MaxProcessingTimeSecs = SnapMapConfig->MaxProcessingTimeSecs;
    StaticState.Diagnostics = Diagnostics;
    StaticState.NegationVolumes = FDungeonNegationVolumeState::PopulateNegationVolumeBounds(Dungeon);

    SnapLib::IModuleDatabasePtr ModDB = MakeShareable(new FSnapMapModuleDatabaseImpl(SnapMapConfig->ModuleDatabase.LoadSynchronous()));
    SnapLib::FSnapGraphGenerator GraphGenerator(ModDB, StaticState);
    SnapLib::ISnapGraphNodePtr StartNode = MakeShareable(new FSnapGraphGrammarNode(MissionGraph->FindRootNode()));
    SnapLib::FModuleNodePtr ModuleTreeRoot = GraphGenerator.Generate(StartNode);
    if (ModuleTreeRoot.IsValid() && Dungeon) {
        // Transform the modules relative to the dungeon's transform
        const FTransform BaseTransform = Dungeon->GetActorTransform();
        SnapLib::TraverseModuleGraph(ModuleTreeRoot, [&BaseTransform](SnapLib::FModuleNodePtr Node) {
            Node->WorldTransform = Node->WorldTransform * BaseTransform;
        });
    }
    return ModuleTreeRoot;
}

bool USnapMapDungeonBuilder::IdentifyBuildSucceeded() const {
    return SnapMapModel.IsValid() && SnapMapModel->ModuleInstances.Num() > 0;
}

void USnapMapDungeonBuilder::BuildPreviewSnapLayout() {
    SnapMapModel = Cast<USnapMapDungeonModel>(DungeonModel);
    SnapMapConfig = Cast<USnapMapDungeonConfig>(DungeonConfig);

    UWorld* World = Dungeon ? Dungeon->GetWorld() : nullptr;

    if (!World || !SnapMapConfig.IsValid() || !SnapMapModel.IsValid()) {
        UE_LOG(SnapMapDungeonBuilderLog, Error, TEXT("Invalid reference passed to the snap builder"));
        return;
    }

    USnapMapAsset* FlowGraph = SnapMapConfig->DungeonFlowGraph.LoadSynchronous();
    if (!FlowGraph) {
        UE_LOG(SnapMapDungeonBuilderLog, Error, TEXT("Dungeon Flow asset not specified"));
        return;
    }

    const int32 Seed = DungeonConfig->Seed;
    Random.Initialize(Seed);
    SnapMapModel->Reset();
    
    if (UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
        LevelStreamingModel->Release(GetWorld());
    }

    const SnapLib::FModuleNodePtr GraphRootNode = GenerateModuleNodeGraph(Seed);
    FSnapMapGraphSerializer::Serialize({ GraphRootNode }, {}, SnapMapModel->ModuleInstances, SnapMapModel->Connections, SnapMapModel->Walls);
}


void USnapMapDungeonBuilder::DestroyDungeonImpl(UWorld* World) {
    Super::DestroyDungeonImpl(World);

    SnapMapModel = Cast<USnapMapDungeonModel>(DungeonModel);
    SnapMapConfig = Cast<USnapMapDungeonConfig>(DungeonConfig);
    
    if (UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
        LevelStreamingModel->Release(GetWorld());
    }

    if (Diagnostics.IsValid()) {
        Diagnostics->Clear();
    }
}

void USnapMapDungeonBuilder::GetSnapConnectionActors(ULevel* ModuleLevel,
                                                        TArray<ASnapConnectionActor*>& OutConnectionActors) {
    OutConnectionActors.Reset();
    for (AActor* Actor : ModuleLevel->Actors) {
        if (!Actor) continue;
        if (ASnapConnectionActor* ConnectionActor = Cast<ASnapConnectionActor>(Actor)) {
            OutConnectionActors.Add(ConnectionActor);
        }
    }
}

FDungeonBuilderCapabilities USnapMapDungeonBuilder::GetCapabilities() {
    FDungeonBuilderCapabilities Capabilities;
    Capabilities.bSupportsTheming = false;
    Capabilities.bSupportsLevelStreaming = true;
    Capabilities.bSupportsOverlappingFloors = true;
    return Capabilities;
}

void USnapMapDungeonBuilder::SetDiagnostics(TSharedPtr<SnapLib::FDiagnostics> InDiagnostics) {
    Diagnostics = InDiagnostics;
}

void USnapMapDungeonBuilder::DrawDebugData(UWorld* InWorld, bool bPersistent /*= false*/, float LifeTime /*= 0*/) {
    TMap<FGuid, TArray<FSnapConnectionInstance>> ConnectionsByModuleId;
    USnapMapDungeonModel* DModel = Cast<USnapMapDungeonModel>(DungeonModel);
    if (DModel) {
        for (const FSnapConnectionInstance& Connection : DModel->Connections) {
            ConnectionsByModuleId.FindOrAdd(Connection.ModuleA).Add(Connection);
            ConnectionsByModuleId.FindOrAdd(Connection.ModuleB).Add(Connection);
        }
    }

    if (Dungeon && Dungeon->LevelStreamingModel) {
        for (const UDungeonStreamingChunk* Chunk : Dungeon->LevelStreamingModel->GetChunks()) {
            if (!Chunk) continue;

            // Draw the bounds
            FVector Center, Extent;
            Chunk->Bounds.GetCenterAndExtents(Center, Extent);
            //DrawDebugBox(InWorld, Center, Extent, FQuat::Identity, FColor::Red, false, -1, 0, 10);

            // Draw the connections to the doors
            float AvgZ = 0;
            const FVector CylinderOffset = FVector(0, 0, 10);
            TArray<FSnapConnectionInstance>& ModuleConnections = ConnectionsByModuleId.FindOrAdd(Chunk->ID);
            for (const FSnapConnectionInstance& Connection : ModuleConnections) {
                FVector Location = Connection.WorldTransform.GetLocation();
                DrawDebugCylinder(InWorld, Location, Location + CylinderOffset, 50, 8, FColor::Red, false, -1, 0, 8);
                AvgZ += Location.Z;
            }

            if (ModuleConnections.Num() > 0) {
                AvgZ /= ModuleConnections.Num();
                FVector CenterPoint = Center;
                CenterPoint.Z = AvgZ;

                DrawDebugCylinder(InWorld, CenterPoint, CenterPoint + CylinderOffset, 80, 16, FColor::Green, false, -1,
                                  0, 4);

                // Draw a point in the center of the room
                for (const FSnapConnectionInstance& Connection : ModuleConnections) {
                    FVector Start = CenterPoint + CylinderOffset / 2.0f;
                    FVector End = Connection.WorldTransform.GetLocation() + CylinderOffset / 2.0f;
                    DrawDebugLine(InWorld, Start, End, FColor::Green, false, -1, 0, 20);
                }

            }
        }
    }
}

TSubclassOf<UDungeonModel> USnapMapDungeonBuilder::GetModelClass() {
    return USnapMapDungeonModel::StaticClass();
}

TSubclassOf<UDungeonConfig> USnapMapDungeonBuilder::GetConfigClass() {
    return USnapMapDungeonConfig::StaticClass();
}

TSubclassOf<UDungeonToolData> USnapMapDungeonBuilder::GetToolDataClass() {
    return USnapMapDungeonToolData::StaticClass();
}

TSubclassOf<UDungeonQuery> USnapMapDungeonBuilder::GetQueryClass() {
    return USnapMapDungeonQuery::StaticClass();
}

bool USnapMapDungeonBuilder::SupportsProperty(const FName& PropertyName) const {
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, Themes)) return false;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, MarkerEmitters)) return false;

    return true;
}

TSharedPtr<class FDungeonSceneProvider> USnapMapDungeonBuilder::CreateSceneProvider(bool bInInstanced) {
    // Ignoring bInInstanced as we don't support instancing as we want each room to be its own level instance
    return MakeShared<FPooledDungeonSceneProvider>();
}

bool USnapMapDungeonBuilder::CanBuildDungeon(FString& OutMessage) {
    if (ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter())) {
        SnapMapConfig = Cast<USnapMapDungeonConfig>(OuterDungeon->GetConfig());

        if (!SnapMapConfig.IsValid()) {
            OutMessage = "Dungeon not initialized correctly";
            return false;
        }

        const USnapMapAsset* FlowGraph = SnapMapConfig->DungeonFlowGraph.LoadSynchronous();
        if (!FlowGraph) {
            OutMessage = "Dungeon Flow asset not assigned";
            return false;
        }

        USnapMapModuleDatabase* ModuleDatabase = SnapMapConfig->ModuleDatabase.LoadSynchronous();
        if (!ModuleDatabase) {
            OutMessage = "Module Database asset not assigned";
            return false;
        }
    }
    else {
        OutMessage = "Dungeon not initialized correctly";
        return false;
    }

    return true;
}

bool USnapMapDungeonBuilder::PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
    return false;
}

FTransform USnapMapDungeonBuilder::PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) {
    return FTransform::Identity;
}

