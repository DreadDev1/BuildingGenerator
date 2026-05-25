//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SnapGridFlow/SnapGridFlowDungeon.h"

#include "Builders/SnapGridFlow/SnapGridFlowAsset.h"
#include "Core/Dungeon.h"
#include "Core/DungeonEventListener.h"
#include "Core/Utils/DungeonUtils.h"
#include "Core/Utils/MathUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Utils/FlowLayoutGraphVisualization.h"
#include "Frameworks/Flow/ExecGraph/FlowExecGraphScript.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "Frameworks/Flow/FlowProcessor.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraph.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraphDomain.h"
#include "Frameworks/Snap/Lib/Theming/SnapTheme.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowGraphSerialization.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowLibrary.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleBounds.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleResolver.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowStats.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"
#include "Frameworks/ThemeEngine/Markers/PlaceableMarker.h"

#include "EngineUtils.h"

class USnapGridFlowAbstractGraph;
DEFINE_LOG_CATEGORY_STATIC(SnapGridDungeonBuilderLog, Log, All);

///////////////////////////// USnapGridFlowBuilder /////////////////////////////
void USnapGridFlowBuilder::BuildDungeonImpl(UWorld* World) {
    SnapGridModel = Cast<USnapGridFlowModel>(DungeonModel);
    SnapGridConfig = Cast<USnapGridFlowConfig>(DungeonConfig);
    SnapGridQuery = Cast<USnapGridFlowQuery>(DungeonQuery);

    if (!World || !SnapGridConfig.IsValid() || !SnapGridModel.IsValid()) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Invalid reference passed to the snap builder"));
        return;
    }

    USnapGridFlowAsset* FlowAsset = SnapGridConfig->FlowGraph.LoadSynchronous();
    if (!FlowAsset) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Dungeon Flow asset not specified"));
        return;
    }

    USnapGridFlowModuleDatabase* ModuleDatabase = SnapGridConfig->ModuleDatabase.LoadSynchronous();
    if (!ModuleDatabase) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Module Database asset is not specified"));
        return;
    }

    if (FlowAsset->Version != static_cast<int>(ESnapGridFlowAssetVersion::LatestVersion)) {
        UE_LOG(SnapGridDungeonBuilderLog, Warning, TEXT("SGF Flow Graph asset is of an older version. Open and Save it once in the SGF Flow Graph editor to improve performance"));
        FSnapGridFlowAssetRuntimeUpgrader::Upgrade(FlowAsset);
    }
    
    TArray<SnapLib::FModuleNodePtr> ModuleGraphNodes;
    bool bGenerationSuccessful{};
    if (ExecuteFlowGraph()) {
        if (GenerateModuleNodeGraph(ModuleGraphNodes)) {
            bGenerationSuccessful = true;
        }
        else {
            UE_LOG(SnapGridDungeonBuilderLog, Warning, TEXT("Failed to resolve SGF Modules"));
        }
    }
    else {
        UE_LOG(SnapGridDungeonBuilderLog, Warning, TEXT("Failed to generate SGF Flow Graph"));
    }

    {
        const FSGFSnapConnectionSerializePolicy ConnectionSerializePolicy(SnapGridModel->AbstractGraph);
        FSnapGridFlowGraphSerializer::Serialize(ModuleGraphNodes, ConnectionSerializePolicy, SnapGridModel->ModuleInstances, SnapGridModel->Connections, SnapGridModel->Walls);
    }

    if (!BuilderParameters.bLayoutPreviewBuildOnly) {
        TSet<FGuid> SpawnRoomNodes;
        if (SnapGridModel->AbstractGraph) {
            for (UFlowAbstractNode* Node : SnapGridModel->AbstractGraph->GraphNodes) {
                if (FFlowAbstractGraphUtils::ContainsItem(Node->NodeItems, EFlowGraphItemType::Entrance)) {
                    SpawnRoomNodes.Add(Node->NodeId);
                }
            } 
        }
        
        if (Dungeon->LevelStreamingModel) {
            Dungeon->LevelStreamingModel->OnChunkVisible.RemoveAll(this);
            Dungeon->LevelStreamingModel->OnChunkVisible.AddDynamic(this, &USnapGridFlowBuilder::HandleChunkVisible);

            
            Dungeon->LevelStreamingModel->OnChunkLoaded.RemoveAll(this);
            Dungeon->LevelStreamingModel->OnChunkLoaded.AddDynamic(this, &USnapGridFlowBuilder::HandleChunkLoaded);
        }

        if (UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
            LevelStreamingModel->Release(GetWorld());
        }

        FSnapStreamingLib::GenerateLevelStreamingModel(World, ModuleGraphNodes, Dungeon->LevelStreamingModel, USnapGridFlowStreamingChunk::StaticClass(),
            [this, &SpawnRoomNodes](UDungeonStreamingChunk* InChunk, SnapLib::FModuleNodePtr Node)  {
                USnapStreamingChunkBase* Chunk = Cast<USnapStreamingChunkBase>(InChunk);
                if (Chunk) {
                    Chunk->ModuleTransform = Node->WorldTransform;
                    if (SpawnRoomNodes.Contains(Node->ModuleInstanceId)) {
                        Chunk->bAlwaysResident = true;
                    }
                }
            });
    
        // Create the debug draw actor
        if (Dungeon) {
            if (Dungeon->bDrawDebugData) {
                CreateDebugVisualizations(Dungeon->Uid, Dungeon->GetActorTransform());
            }
            else {
                DestroyDebugVisualizations(Dungeon->Uid);
            }
        }
    }
}

bool USnapGridFlowBuilder::GenerateModuleNodeGraph(TArray<SnapLib::FModuleNodePtr>& OutNodes) const {
    SCOPE_CYCLE_COUNTER(STAT_SGFResolve);
    if (!SnapGridConfig.IsValid()) {
        return false;
    }
    
    USnapGridFlowModuleDatabase* ModuleDatabase = SnapGridConfig->ModuleDatabase.LoadSynchronous();
    const USnapGridFlowModuleBoundsAsset* BoundsAsset = ModuleDatabase ? ModuleDatabase->ModuleBoundsAsset.LoadSynchronous() : nullptr;
    if (!ModuleDatabase || !BoundsAsset) {
        return false;
    }
    
    if (!SnapGridModel->AbstractGraph) {
        return false;
    }
    
    FSnapGridFlowModuleResolverSettings ResolveSettings;
    ResolveSettings.Seed = SnapGridConfig->Seed;
    ResolveSettings.ChunkSize = BoundsAsset->ChunkSize;
    ResolveSettings.ModulesWithMinimumDoorsProbability = SnapGridConfig->PreferModulesWithMinimumDoors;
    ResolveSettings.BaseTransform = Dungeon ? Dungeon->GetActorTransform() : FTransform::Identity;
    ResolveSettings.MaxResolveFrames = SnapGridConfig->MaxResolveFrames;
    ResolveSettings.NonRepeatingRooms = SnapGridConfig->NonRepeatingRooms;
    ResolveSettings.ModuleResolveRules = SnapGridConfig->ModuleResolveRules;
    
    const FSnapGridFlowModuleDatabaseImplPtr ModDB = MakeShareable(new FSnapGridFlowModuleDatabaseImpl(ModuleDatabase));
    const FSnapGridFlowModuleResolver ModuleResolver(ModDB, ResolveSettings);
    
    return ModuleResolver.Resolve(SnapGridModel->AbstractGraph, OutNodes);
}

bool USnapGridFlowBuilder::ExecuteFlowGraph() {
    SCOPE_CYCLE_COUNTER(STAT_SGFBuild);
    SnapGridModel->AbstractGraph = nullptr;

    USnapGridFlowAsset* SnapGridFlowAsset = SnapGridConfig->FlowGraph.LoadSynchronous();
    USnapGridFlowModuleDatabase* ModuleDatabase = SnapGridConfig->ModuleDatabase.LoadSynchronous();

    if (!SnapGridFlowAsset) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Missing Grid Flow graph"));
        return false;
    }

    if (!SnapGridFlowAsset->ExecScript) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Invalid Grid Flow graph state. Please resave in editor"));
        return false;
    }

    FFlowProcessor FlowProcessor;

    // Register the domains
    {
        FSnapGridFlowProcessDomainExtender Extender(ModuleDatabase, SnapGridConfig->bSupportDoorCategories);
        Extender.ExtendDomains(FlowProcessor);
    }
	
    const int32 MAX_RETRIES = FMath::Max(1, SnapGridConfig->NumLayoutBuildRetries);
    int32 NumTries = 0;
    int32 NumTimeouts = 0;
    FFlowProcessorResult Result;
    while (NumTries < MAX_RETRIES) {
        FFlowProcessorSettings FlowProcessorSettings;
        FlowProcessorSettings.AttributeList = AttributeList;
        FlowProcessorSettings.SerializedAttributeList = SnapGridConfig->ParameterOverrides;
        FlowProcessorSettings.Dungeon = Dungeon;
        Result = FlowProcessor.Process(SnapGridFlowAsset->ExecScript, Random, FlowProcessorSettings);
        NumTries++;
        if (Result.ExecResult == EFlowTaskExecutionResult::Success) {
            break;
        }

        if (Result.ExecResult == EFlowTaskExecutionResult::FailHalt) {
            bool bHalt = true;

            if (Result.FailReason == EFlowTaskExecutionFailureReason::Timeout) {
                NumTimeouts++;
                if (NumTimeouts <= SnapGridConfig->NumTimeoutsRetriesAllowed) {
                    // Continue despite the timeout
                    bHalt = false;
                }
            }

            if (bHalt) {
                break;
            }
        }
    }

    if (Result.ExecResult != EFlowTaskExecutionResult::Success) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Failed to generate grid flow graph"));
        return false;
    }

    if (!SnapGridFlowAsset->ExecScript->ResultNode) {
        UE_LOG(SnapGridDungeonBuilderLog, Error,
               TEXT(
                   "Cannot find result node in the grid flow exec graph. Please resave the grid flow asset in the editor"
               ));
        return false;
    }

    const FGuid ResultNodeId = SnapGridFlowAsset->ExecScript->ResultNode->NodeId;
    if (FlowProcessor.GetNodeExecStage(ResultNodeId) != EFlowTaskExecutionStage::Executed) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Grid Flow Graph execution failed"));
        return false;
    }

    FFlowExecutionOutput ResultNodeState;
    FlowProcessor.GetNodeState(ResultNodeId, ResultNodeState);
    if (ResultNodeState.ExecutionResult != EFlowTaskExecutionResult::Success) {
        UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Grid Flow Result node execution did not succeed"));
        return false;
    }

    // Save a copy in the model
    const USnapGridFlowAbstractGraph* TemplateGraph = ResultNodeState.State->GetState<USnapGridFlowAbstractGraph>(UFlowAbstractGraphBase::StateTypeID);
    
    // Make a deep copy of the template graph
    SnapGridModel->AbstractGraph = NewObject<USnapGridFlowAbstractGraph>(SnapGridModel.Get(), "AbstractGraph", RF_NoFlags);
    if (TemplateGraph) {
        SnapGridModel->AbstractGraph->CloneFromStateObject(TemplateGraph);
    }
    return true;
}

void USnapGridFlowBuilder::DestroyDungeonImpl(UWorld* World) {
    Super::DestroyDungeonImpl(World);
    
    if (!World) {
        return;
    }
    
    SnapGridModel = Cast<USnapGridFlowModel>(DungeonModel);
    SnapGridConfig = Cast<USnapGridFlowConfig>(DungeonConfig);

    if (UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
        LevelStreamingModel->Release(GetWorld());
    }

    // Create the debug draw actor of specified
    if (Dungeon) {
        DestroyDebugVisualizations(Dungeon->Uid);
    }
}

void USnapGridFlowBuilder::CreateDebugVisualizations(const FGuid& DungeonID, const FTransform& InTransform) const {
    DestroyDebugVisualizations(DungeonID);

    if (!SnapGridModel.IsValid() || !SnapGridConfig.IsValid()) {
        return;
    }
    USnapGridFlowModuleDatabase* ModuleDatabase = SnapGridConfig->ModuleDatabase.LoadSynchronous();
    const USnapGridFlowModuleBoundsAsset* BoundsAsset = ModuleDatabase ? ModuleDatabase->ModuleBoundsAsset.LoadSynchronous() : nullptr;
    
    if (SnapGridModel->AbstractGraph && ModuleDatabase && BoundsAsset) {
        const FVector ModuleSize = BoundsAsset->ChunkSize;
    
        UWorld* World = GetWorld();
        AFlowLayoutGraphVisualizer* Visualizer = World->SpawnActor<AFlowLayoutGraphVisualizer>();
        Visualizer->DungeonID = DungeonID;
        Visualizer->SetAutoAlignToLevelViewport(true);
        
        // Set the transform
        {
            FTransform VisualizerTransform = InTransform;
            VisualizerTransform.SetScale3D(FVector::OneVector);
            Visualizer->SetActorTransform(VisualizerTransform);
        }
        
        if (SnapGridModel.IsValid() && SnapGridModel->AbstractGraph) {
            FDAAbstractGraphVisualizerSettings Settings;
            //const float ModuleWidth = FMath::Min(ModuleSize.X, ModuleSize.Y);
            Settings.NodeRadius = 60.0f; // ModuleWidth * 0.05;
            Settings.LinkThickness = Settings.NodeRadius * 0.2f;
            Settings.LinkRefThickness = Settings.LinkThickness * 0.5f;
            Settings.NodeSeparationDistance = ModuleSize;
            Visualizer->Generate(SnapGridModel->AbstractGraph, Settings);
        }
    }
}

void USnapGridFlowBuilder::DestroyDebugVisualizations(const FGuid& DungeonID) const {
    UWorld* World = GetWorld();
    for (TActorIterator<AFlowLayoutGraphVisualizer> It(World); It; ++It) {
        AFlowLayoutGraphVisualizer* Visualizer = *It;
        if (Visualizer && Visualizer->DungeonID == DungeonID) {
            Visualizer->Destroy();
        }
    }
}

TArray<AActor*> USnapGridFlowBuilder::SpawnItemWithThemeEngine(UFlowGraphItem* ItemInfo, const APlaceableMarkerActor* InMarkerActor) {
    TArray<AActor*> SpawnedActors;
    if (SnapGridConfig.IsValid() && ItemInfo && Dungeon) {
        const FString MarkerName = ItemInfo->MarkerName.TrimStartAndEnd();
    
        TArray<FDungeonMarkerInstance> MarkersToEmit;
        FDungeonMarkerInstance& Marker = MarkersToEmit.AddDefaulted_GetRef();
        Marker.Id = 0;
        Marker.MarkerName = MarkerName;
        Marker.Transform = InMarkerActor->GetActorTransform();

        const TSharedPtr<FSnapThemeSceneProvider> SceneProvider = MakeShared<FSnapThemeSceneProvider>();
    
        FDungeonThemeEngineSettings ThemeEngineSettings;
        ThemeEngineSettings.Themes = { SnapGridConfig->ItemTheme.LoadSynchronous() };
        ThemeEngineSettings.SceneProvider = SceneProvider;
        ThemeEngineSettings.LevelOverride = InMarkerActor->GetLevel();
        ThemeEngineSettings.Dungeon = Dungeon;
        if (Dungeon) {
            ThemeEngineSettings.bRoleAuthority = Dungeon->HasAuthority(); 
        }

        FDungeonThemeEngineEventHandlers ThemeEventHandlers;
        ThemeEventHandlers.PerformSelectionLogic = [this](const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
            return PerformSelectionLogic(SelectionLogics, InMarker);
        };
        
        ThemeEventHandlers.PerformGenericSelectionLogic = [this](const TArray<UGenericDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
            return PerformGenericSelectionLogic(SelectionLogics, InMarker);
        };
        
        ThemeEventHandlers.PerformTransformLogic = [this](const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarker) {
            return PerformTransformLogic(TransformLogics, InMarker);
        };

        ThemeEventHandlers.HandlePostMarkersEmit = [this](TArray<FDungeonMarkerBuildData>& MarkersToEmit) {
            DungeonUtils::FDungeonEventListenerNotifier::NotifyMarkersEmitted(Dungeon, MarkersToEmit);
        };
        
        // Invoke the Theme Engine
        FDungeonThemeEngine::Apply(MarkersToEmit, Random, ThemeEngineSettings, ThemeEventHandlers);
        SceneProvider->ExecuteAllCommands(GetWorld());

        for (TWeakObjectPtr<AActor> SpawnedActor : SceneProvider->GetSpawnedActors()) {
            if (SpawnedActor.IsValid()) {
                SpawnedActors.Add(SpawnedActor.Get());
            }
        }
    }
    return SpawnedActors;
}

TArray<AActor*> USnapGridFlowBuilder::SpawnChunkPlaceableMarkers(const TArray<APlaceableMarkerActor*>& PlaceableMarkers, const FGuid& AbstractNodeId, const FBox& ChunkBounds, const FString& ChunkName) {
    // Populate the requested marker list from the abstract node 
    TArray<UFlowGraphItem*> Items;
    {
        // Grab the node from the abstract graph. The chunk id will be the same as the abstract node id
        UFlowAbstractNode* AbstractNode = SnapGridModel->AbstractGraph->GetNode(AbstractNodeId);

        // Check if abstract node has any items to spawn
        if (AbstractNode) {
            for (UFlowGraphItem* Item : AbstractNode->NodeItems) {
                if (Item) {
                    Items.Add(Item);
                }
            }
        }
    }

    TArray<AActor*> AllSpawnedActors;
    
    TMap<APlaceableMarkerActor*, FString> MarkersToEmit;
    const int32 MarkerRandomSeed = HashCombine(SnapGridConfig->Seed, GetTypeHash(ChunkBounds.GetCenter()));
    const FRandomStream RandomMarkerSelection(MarkerRandomSeed);
    TArray<APlaceableMarkerActor*> ShuffledPlaceableMarkers = PlaceableMarkers;
    FDAMathUtils::Shuffle(ShuffledPlaceableMarkers, RandomMarkerSelection);
    for (UFlowGraphItem* ItemInfo : Items) {
        FString Marker = ItemInfo->MarkerName.TrimStartAndEnd();
        if (Marker.Len() == 0) continue;
    
        APlaceableMarkerActor* MarkerActor = nullptr;
        for (APlaceableMarkerActor* PlaceableMarker : ShuffledPlaceableMarkers) {
            if (PlaceableMarker->GetAllowedMarkerNames().Contains(Marker)) {
                MarkerActor = PlaceableMarker;
                break;
            }
        }

        if (MarkerActor) {
            ShuffledPlaceableMarkers.Remove(MarkerActor);
            TArray<AActor*> SpawnedActors = SpawnItemWithThemeEngine(ItemInfo, MarkerActor);
            AllSpawnedActors.Append(SpawnedActors);
            
            // If it contains a item metadata component, then assign the item
            for (const TWeakObjectPtr<AActor> SpawnedActor : SpawnedActors) {
                if (UDungeonFlowItemMetadataComponent* ItemComponent = SpawnedActor->FindComponentByClass<UDungeonFlowItemMetadataComponent>()) {
                    ItemComponent->SetFlowItem(ItemInfo);
                }
            }
        } else {
            UE_LOG(SnapGridDungeonBuilderLog, Error, TEXT("Cannot spawn marker \"%s\" in module chunk: %s"), *Marker, *ChunkName);
        }
    }

    return AllSpawnedActors;
}

void USnapGridFlowBuilder::HandleChunkLoaded(ADungeon* InDungeon, UDungeonStreamingChunk* InChunk) {
    if (!InChunk) return;
    
    UWorld* World = GetWorld();
    if (World && !World->IsGameWorld()) {
        // We are building this in the editor.
        // Disable the module bounds rendering

        const ULevel* Level = InChunk ? InChunk->GetLoadedLevel() : nullptr;
        if (Level) {
            for (AActor* Actor : Level->Actors) {
                if (ASnapGridFlowModuleBoundsActor* BoundsActor = Cast<ASnapGridFlowModuleBoundsActor>(Actor)) {
                    if (BoundsActor->BoundsComponent) {
                        BoundsActor->BoundsComponent->bRenderBounds = false;
                    }
                }
            }
        }
    }

    if (USnapGridFlowStreamingChunk* SGFChunk = Cast<USnapGridFlowStreamingChunk>(InChunk)) {
        SGFChunk->bPlaceableMarkersSpawned = false;
    }
}

void USnapGridFlowBuilder::HandleChunkVisible(ADungeon* InDungeon, UDungeonStreamingChunk* InChunk) {
    if (!InChunk || !InChunk->GetLoadedLevel()) return;
    if (!SnapGridModel.IsValid() || !SnapGridModel->AbstractGraph) return;
    
    TrySpawnPlaceableMarkers(InChunk);
}

void USnapGridFlowBuilder::TrySpawnPlaceableMarkers(UDungeonStreamingChunk* InChunk) {
    
    if (USnapGridFlowStreamingChunk* SGFChunk = Cast<USnapGridFlowStreamingChunk>(InChunk)) {
        if (!SGFChunk->bPlaceableMarkersSpawned) {
            SGFChunk->bPlaceableMarkersSpawned = true;
            
            const TArray<AActor*>& ChunkActors = InChunk->GetLoadedLevel()->Actors;
            const FGuid AbstractNodeId = InChunk->ID;
            const FBox ChunkBounds = InChunk->Bounds;
            const FString ChunkName = InChunk->GetLoadedLevel()->GetFullName();

            TArray<APlaceableMarkerActor*> PlaceableMarkers;
            for (AActor* Actor : ChunkActors) {
                if (APlaceableMarkerActor* PlaceableMarker = Cast<APlaceableMarkerActor>(Actor)) {
                    PlaceableMarkers.Add(PlaceableMarker);
                }
            }
            TArray<AActor*> SpawnedActors = SpawnChunkPlaceableMarkers(PlaceableMarkers, AbstractNodeId, ChunkBounds, ChunkName);
    
            // Register the spawned actors so they can be cleaned up when the chunk is destroyed
            if (InChunk) {
                for (TWeakObjectPtr<AActor> SpawnedActor : SpawnedActors) {
                    if (SpawnedActor.IsValid()) {
                        InChunk->RegisterManagedActor(SpawnedActor.Get());
                    }
                }
            }
        }
    }
}

bool USnapGridFlowBuilder::IdentifyBuildSucceeded() const {
    return SnapGridModel.IsValid() && SnapGridModel->AbstractGraph && SnapGridModel->ModuleInstances.Num() > 0; 
}

void USnapGridFlowBuilder::DrawDebugData(UWorld* InWorld, bool bPersistent, float LifeTime) {
    /*
    if (!SnapGridModel.IsValid() || !SnapGridConfig.IsValid()) {
        return;
    }

    if (!SnapGridModel->AbstractGraph) {
        return;
    }
    
    if (!SnapGridConfig->ModuleDatabase || !SnapGridConfig->ModuleDatabase->ModuleBoundsAsset) {
        return;
    }

    FVector BaseOffset = Dungeon ? Dungeon->GetActorLocation() : FVector::ZeroVector;
    const FColor BoxColor = FColor(128, 0, 0);
    const FVector ModuleSize = SnapGridConfig->ModuleDatabase->ModuleBoundsAsset->ChunkSize;
    const FIntVector LayoutGridSize = SnapGridModel->AbstractGraph->GridSize;
    for (int z = 0; z < LayoutGridSize.Z; z++) {
        for (int y = 0; y < LayoutGridSize.Y; y++) {
            for (int x = 0; x < LayoutGridSize.X; x++) {
                const FVector BoxMin = BaseOffset + FVector(x, y, z) * ModuleSize;
                const FVector BoxExtent = ModuleSize * 0.5f;
                const FVector BoxCenter = BoxMin + BoxExtent;
                DrawDebugBox(InWorld, BoxCenter, BoxExtent, BoxColor, bPersistent, LifeTime);
            }
        }
    }
    */
}

bool USnapGridFlowBuilder::SupportsProperty(const FName& PropertyName) const {
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, Themes)) return false;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, MarkerEmitters)) return false;
    //if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, EventListeners)) return false;

    return  true;
}

bool USnapGridFlowBuilder::CanBuildDungeon(FString& OutMessage) {
    ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter());
    if (OuterDungeon) {
        SnapGridConfig = Cast<USnapGridFlowConfig>(OuterDungeon->GetConfig());

        if (!SnapGridConfig.IsValid()) {
            OutMessage = "Dungeon not initialized correctly";
            return false;
        }

        USnapGridFlowAsset* FlowAsset = SnapGridConfig->FlowGraph.LoadSynchronous();
        if (!FlowAsset) {
            OutMessage = "Dungeon Flow asset not assigned";
            return false;
        }

        UDungeonThemeAsset* ItemThemeAsset = SnapGridConfig->ItemTheme.LoadSynchronous();
        if (!ItemThemeAsset) {
            OutMessage = "Item Theme asset not assigned";
            return false;
        }

        USnapGridFlowModuleDatabase* ModuleDatabase = SnapGridConfig->ModuleDatabase.LoadSynchronous();
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

bool USnapGridFlowBuilder::PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarker) {
    for (UDungeonSelectorLogic* SelectionLogic : SelectionLogics) {
        USnapGridFlowSelectorLogic* SGFSelectionLogic = Cast<USnapGridFlowSelectorLogic>(SelectionLogic);
        if (!SGFSelectionLogic) {
            UE_LOG(SnapGridDungeonBuilderLog, Warning, TEXT("Invalid selection logic specified.  SnapGridFlowSelectorLogic expected"));
            return false;
        }

        // Perform blueprint based selection logic
        const bool bSelected = SGFSelectionLogic->SelectNode(SnapGridModel.Get(), SnapGridConfig.Get(), this, SnapGridQuery.Get(), Random, InMarker.Transform);
        if (!bSelected) {
            return false;
        }
    }
    return true;
}

FTransform USnapGridFlowBuilder::PerformTransformLogic(const TArray<UDungeonTransformLogic*>& DungeonTransformLogics, const FDungeonMarkerInstance& InMarker) {
    FTransform result = FTransform::Identity;

    for (UDungeonTransformLogic* TransformLogic : DungeonTransformLogics) {
        USnapGridFlowTransformLogic* GridFlowTransformLogic = Cast<USnapGridFlowTransformLogic>(TransformLogic);
        if (!GridFlowTransformLogic) {
            UE_LOG(SnapGridDungeonBuilderLog, Warning, TEXT("Invalid transform logic specified.  SnapGridFlowTransformLogic expected"));
            continue;
        }

        FTransform LogicOffset;
        if (TransformLogic) {
            GridFlowTransformLogic->GetNodeOffset(SnapGridModel.Get(), SnapGridConfig.Get(), SnapGridQuery.Get(), Random,  LogicOffset);
        }
        else {
            LogicOffset = FTransform::Identity;
        }

        FTransform out;
        FTransform::Multiply(&out, &LogicOffset, &result);
        result = out;
    }
    return result;
}

FDungeonBuilderCapabilities USnapGridFlowBuilder::GetCapabilities() {
    FDungeonBuilderCapabilities Capabilities;
    Capabilities.bSupportsTheming = false;
    Capabilities.bSupportsLevelStreaming = true;
    Capabilities.bSupportsOverlappingFloors = true;
    return Capabilities;
}

TSubclassOf<UDungeonModel> USnapGridFlowBuilder::GetModelClass() {
    return USnapGridFlowModel::StaticClass();
}

TSubclassOf<UDungeonConfig> USnapGridFlowBuilder::GetConfigClass() {
    return USnapGridFlowConfig::StaticClass();
}

TSubclassOf<UDungeonToolData> USnapGridFlowBuilder::GetToolDataClass() {
    return USnapGridFlowToolData::StaticClass();
}

TSubclassOf<UDungeonQuery> USnapGridFlowBuilder::GetQueryClass() {
    return USnapGridFlowQuery::StaticClass();
}

///////////////////////////// USnapGridFlowModel /////////////////////////////
void USnapGridFlowModel::Reset() {
	Super::Reset();
    Connections.Reset();
    Walls.Reset();
    ModuleInstances.Reset();
    AbstractGraph = nullptr;
}

FDungeonFloorSettings USnapGridFlowModel::CreateFloorSettings(const UDungeonConfig* InConfig) const {
    FDungeonFloorSettings Settings = {};
    if (const USnapGridFlowConfig* SGFConfig = Cast<USnapGridFlowConfig>(InConfig)) {
        if (const USnapGridFlowModuleDatabase* ModDB = SGFConfig->ModuleDatabase.LoadSynchronous()) {
            if (const USnapGridFlowModuleBoundsAsset* BoundsAsset = ModDB->ModuleBoundsAsset.LoadSynchronous()) {
                Settings.FloorHeight = BoundsAsset->ChunkSize.Z;
                Settings.FloorCaptureHeight = Settings.FloorHeight - 1;
                Settings.GlobalHeightOffset = -0.5f * Settings.FloorHeight;
            }
        }
    }

    GetFloorIndexRange(Settings.FloorHeight, Settings.GlobalHeightOffset, Settings.FloorLowestIndex, Settings.FloorHighestIndex);

    return Settings;
}

///////////////////////////// FSnapGridFlowStreamingChunkHandler /////////////////////////////
void FSnapGridFlowProcessDomainExtender::ExtendDomains(FFlowProcessor& InProcessor) {
    // Register the Abstract Layout Graph Domain
    const TSharedPtr<FSnapGridFlowAbstractGraphDomain> SGFLayoutGraphDomain = MakeShareable(new FSnapGridFlowAbstractGraphDomain);
    SGFLayoutGraphDomain->SetModuleDatabase(ModuleDatabase);
    SGFLayoutGraphDomain->SetSupportsDoorCategory(bSupportsDoorCategory);
    InProcessor.RegisterDomain(SGFLayoutGraphDomain);
}


///////////////////////////// USnapGridFlowSelectorLogic /////////////////////////////
bool USnapGridFlowSelectorLogic::SelectNode_Implementation(USnapGridFlowModel* Model, USnapGridFlowConfig* Config, USnapGridFlowBuilder* Builder,
            USnapGridFlowQuery* Query, const FRandomStream& RandomStream, const FTransform& MarkerTransform) {
    return false;
}

///////////////////////////// USnapGridFlowTransformLogic /////////////////////////////
void USnapGridFlowTransformLogic::GetNodeOffset_Implementation(USnapGridFlowModel* Model, USnapGridFlowConfig* Config, USnapGridFlowQuery* Query,
            const FRandomStream& RandomStream, FTransform& Offset) {
    
}

