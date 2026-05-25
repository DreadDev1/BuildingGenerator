//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/DungeonBuilder.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Core/Dungeon.h"
#include "Core/DungeonConfig.h"
#include "Core/DungeonEventListener.h"
#include "Core/DungeonQuery.h"
#include "Core/Markers/DungeonMarkerVisualization.h"
#include "Core/Utils/Debug/SceneDebugDataComponent.h"
#include "Core/Utils/DungeonModelHelper.h"
#include "Core/Utils/DungeonUtils.h"
#include "Core/Volumes/DAVoxelVolume.h"
#include "Core/Volumes/DungeonMarkerReplaceVolume.h"
#include "Core/Volumes/DungeonMirrorVolume.h"
#include "Core/Volumes/DungeonThemeOverrideVolume.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"
#include "Frameworks/ThemeEngine/Markers/DungeonMarkerEmitter.h"
#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerEmitter.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/ProceduralDungeonTransformLogic.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandVoxel.h"
#include "Frameworks/ThemeEngine/SceneProviders/Providers/InstancedDungeonSceneProvider.h"
#include "Frameworks/ThemeEngine/SceneProviders/Providers/PooledDungeonSceneProvider.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDataGPU.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"
#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"
#include "Frameworks/Voxel/VDB/VDBUtils.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(DungeonBuilderLog);

void UDungeonBuilder::BuildDungeon(ADungeon* InDungeon, UWorld* InWorld, const FDungeonBuilderParameters& InBuilderParameters) {
    this->Dungeon = InDungeon;
    if (!Dungeon) {
        UE_LOG(DungeonBuilderLog, Log, TEXT("Cannot build dungeon due to invalid reference"));
        return;
    }

    BuildDungeon(Dungeon->GetModel(), Dungeon->GetConfig(), Dungeon->GetQuery(), InWorld, InBuilderParameters);
}

void UDungeonBuilder::BuildDungeon(UDungeonModel* InModel, UDungeonConfig* InConfig, UDungeonQuery* InQuery, UWorld* InWorld, const FDungeonBuilderParameters& InBuilderParameters) {
    this->DungeonModel = InModel;
    this->DungeonConfig = InConfig;
    this->DungeonQuery = InQuery;
    BuilderParameters = InBuilderParameters;
    BuilderParameters.CustomParameters.SetObject(TEXT("DungeonConfig"), InConfig);
    BuilderParameters.CustomParameters.SetObject(TEXT("DungeonModel"), InModel);
    BuilderParameters.CustomParameters.SetTransform(TEXT("DungeonTransform"), GetDungeonTransform());
    
    if (DungeonQuery && DungeonQuery->UserState) {
        DungeonQuery->UserState->ClearAllState();
    }

    InitializeState();
    
    ClearMarkerList();
    if (ShouldDestroyBeforeBuild()) {
        DestroyDungeonImpl(InWorld);
    }

    Random.Initialize(DungeonConfig->Seed);
    if (DungeonModel) {
        DungeonModel->Reset();
    }
    
    BuildDungeonImpl(InWorld);

    if (GetCapabilities().bSupportsTheming) {
        if (!Dungeon) {
            Dungeon = Cast<ADungeon>(GetOuter());
        }
        
        if (Dungeon) {
            // Notify that the layout of the dungeon has been built
            DungeonUtils::FDungeonEventListenerNotifier::NotifyDungeonLayoutBuilt(Dungeon);
        }

        EmitDungeonMarkers();
        
        if (Dungeon) {
            EmitProceduralMarkers(Dungeon->ProceduralMarkerEmitters);
        }
        
        ProcessMarkerReplacementVolumes();
    }
    
    // Generate and cache the dungeon layout
    if (InModel) {
        InModel->DungeonLayout = {};
        InModel->GenerateLayoutData(InConfig, InModel->DungeonLayout);
    }

    bBuildSucceeded = IdentifyBuildSucceeded();
}

void UDungeonBuilder::DestroyDungeon(UDungeonModel* InModel, UDungeonConfig* InConfig, UDungeonQuery* InQuery, ADungeon* InDungeon, UWorld* InWorld) {
    this->DungeonModel = InModel;
    this->DungeonConfig = InConfig;
    this->DungeonQuery = InQuery;
    this->Dungeon = InDungeon;

    DestroyDungeonImpl(InWorld);
    
    if (InModel) {
        InModel->DungeonLayout = {};
    }
}

void UDungeonBuilder::InitializeState() {
    if (!!DungeonModel || !DungeonConfig) {
        if (!Dungeon) {
            Dungeon = Cast<ADungeon>(GetOuter());
        }
        
        if (Dungeon) {
            if (!DungeonModel) {
                DungeonModel = Dungeon->GetModel();
            }
            if (!DungeonConfig) {
                DungeonConfig = Dungeon->GetConfig();
            }
        }
    }
}

bool UDungeonBuilder::ShouldDestroyBeforeBuild() const {
    if (Dungeon && Dungeon->bCarveVoxels) {
        return true;
    }
    
    return false;
}

void UDungeonBuilder::ApplyDungeonTheme(const FDungeonThemeEngineSettings& InThemeSettings) {
    InitializeState();
    
    // We want to reset the seed here
    Random.Reset();
    
    if (!DungeonModel || !DungeonConfig) {
        return;
    }
    
    // Prepare the Theme Engine callback handlers
    FDungeonThemeEngineEventHandlers EventHandlers;
    EventHandlers.PerformSelectionLogic = [this](const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarkerInfo) {
        return PerformSelectionLogic(SelectionLogics, InMarkerInfo);
    };

    EventHandlers.PerformGenericSelectionLogic = [this](const TArray<UGenericDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarkerInfo) {
        return PerformGenericSelectionLogic(SelectionLogics, InMarkerInfo);
    };
    
    EventHandlers.PerformTransformLogic = [this](const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarkerInfo) {
        return PerformTransformLogic(TransformLogics, InMarkerInfo);
    };

    EventHandlers.PerformProceduralTransformLogic = [this](const TArray<UProceduralDungeonTransformLogic*>& ProceduralTransformLogics, const FDungeonMarkerInstance& InMarkerInfo) {
        return PerformProceduralTransformLogic(ProceduralTransformLogics, InMarkerInfo);
    };

    EventHandlers.HandlePostMarkersEmit = [this](TArray<FDungeonMarkerBuildData>& MarkersToEmit) {
        DungeonUtils::FDungeonEventListenerNotifier::NotifyMarkersEmitted(Dungeon, MarkersToEmit);
    };

    EventHandlers.HandleEmitCustomSceneObjects = [this](FDungeonSceneProvider& SceneProvider) {
        EmitCustomSceneObjects(SceneProvider);
    };
    
    // Invoke the Theme Engine
    FDungeonThemeEngine::Apply(DungeonModel->WorldMarkers, Random, InThemeSettings, EventHandlers);
}

void UDungeonBuilder::MirrorDungeon() {
    if (Dungeon) {
        for (TObjectIterator<ADungeonMirrorVolume> Volume; Volume; ++Volume) {
            if (!Volume || !IsValid(*Volume) || !Volume->IsValidLowLevel()) {
                continue;
            }
            if (Volume->Dungeon == Dungeon) {
                // Build a lookup of the theme for faster access later on
                MirrorDungeonWithVolume(*Volume);
            }
        }
    }
}

TSharedPtr<FDungeonSceneProvider> UDungeonBuilder::CreateSceneProvider(bool bInInstanced) {
    if (bInInstanced) {
        return MakeShared<FInstancedDungeonSceneProvider>();
    }
    else {
        return MakeShared<FPooledDungeonSceneProvider>();
    }
}

FDungeonMarkerInstance& UDungeonBuilder::AddMarker(const FString& InMarkerName, const FTransform& InTransform) {
    check(DungeonModel);
    
    FDungeonMarkerInstance& Marker = DungeonModel->WorldMarkers.AddDefaulted_GetRef();
    Marker.Id = ++_MarkerIdCounter;
    Marker.MarkerName = InMarkerName;
    Marker.Transform = InTransform;
    return Marker;
}

void UDungeonBuilder::EmitDungeonMarkers_Implementation() {
    Random.Initialize(DungeonConfig->Seed);
}

void UDungeonBuilder::EmitMarker(const FString& InMarkerName, const FTransform& InTransform) {
    AddMarker(InMarkerName, InTransform);
}

void UDungeonBuilder::EmitCustomMarkers(TArray<UDungeonMarkerEmitter*> MarkerEmitters, EDungeonMarkerEmitterExecStage InExecutionStage) {
    for (UDungeonMarkerEmitter* MarkerEmitter : MarkerEmitters) {
        if (MarkerEmitter && MarkerEmitter->ExecutionStage == InExecutionStage) {
            MarkerEmitter->EmitMarkers(this, DungeonModel, DungeonConfig, DungeonQuery);
        }
    }
}

void UDungeonBuilder::EmitProceduralMarkers(const TArray<UProceduralMarkerEmitter*>& InProceduralMarkerEmitters) {
    for (const UProceduralMarkerEmitter* ProcMarkerEmitter : InProceduralMarkerEmitters) {
        if (!ProcMarkerEmitter) continue;
        const bool bHandled = EmitProceduralMarkers(ProcMarkerEmitter);
        if (!bHandled) {
            // Give a chance to the proc emitter to handle it
            if (Dungeon) {
                ProcMarkerEmitter->HandleUnsupportedBuilder(Dungeon);
            }
        }
    }
}

UClass* UDungeonBuilder::DefaultBuilderClass() {
    return UGridDungeonBuilder::StaticClass();
}

void UDungeonBuilder::ProcessMarkerReplacementVolumes() const {
    UWorld* World = Dungeon ? Dungeon->GetWorld() : nullptr;
    if (World) {
        for (TActorIterator<ADungeonMarkerReplaceVolume> VolumeIt(World); VolumeIt; ++VolumeIt) {
            ADungeonMarkerReplaceVolume* Volume = *VolumeIt;
            if (Volume && Volume->Dungeon == Dungeon) {
                ProcessMarkerReplacementVolume(Volume);
            }
        }
    }
}

void UDungeonBuilder::ProcessMarkerReplacementVolume(class ADungeonMarkerReplaceVolume* MarkerReplaceVolume) const {
    if (!MarkerReplaceVolume) return;
    FTransform InverseDungeonTransform = IsValid(Dungeon) ? Dungeon->GetActorTransform().Inverse() : FTransform::Identity;
    FBox LocalVolumeBounds = MarkerReplaceVolume->CalcLocalVolumeBounds();
    for (FDungeonMarkerInstance& Marker : DungeonModel->WorldMarkers) {
        const FVector MarkerLocation = Marker.Transform.GetLocation();
        const FVector LocalMarkerLocation = InverseDungeonTransform.TransformPosition(MarkerLocation); 

        if (LocalVolumeBounds.IsInside(LocalMarkerLocation)) {
            for (const FMarkerReplaceEntry& Entry : MarkerReplaceVolume->Replacements) {
                if (Marker.MarkerName == Entry.MarkerName) {
                    Marker.MarkerName = Entry.ReplacementName;
                }
            }
        }
    }
}

UDungeonLevelStreamingModel* UDungeonBuilder::GetLevelStreamingModel() {
    if (!Dungeon) {
        Dungeon = Cast<ADungeon>(GetOuter());
    }
    return Dungeon ? Dungeon->LevelStreamingModel : nullptr; 
}

FTransform UDungeonBuilder::GetDungeonTransform() const {
    if (BuilderParameters.bOverrideDungeonTransform) {
        return BuilderParameters.DungeonTransformOverride;
    }
    else {
        return IsValid(Dungeon) ? Dungeon->GetActorTransform() : FTransform::Identity;
    }
}

FTransform UDungeonBuilder::PerformProceduralTransformLogic(const TArray<UProceduralDungeonTransformLogic*>& ProceduralTransformLogics, const FDungeonMarkerInstance& InMarkerInfo) {
    FTransform Result{FTransform::Identity};

    for (UProceduralDungeonTransformLogic* Logic : ProceduralTransformLogics) {
        if (Logic) {
            Result = Logic->Execute(Dungeon, Random, InMarkerInfo) * Result;
        }
    }
    
    return Result;
}

void UDungeonBuilder::ProcessThemeItemUserData(TSharedPtr<IDungeonMarkerUserData> UserData, AActor* SpawnedActor) {
}

void UDungeonBuilder::ClearMarkerList() {
    _MarkerIdCounter = 0;
    DungeonModel->WorldMarkers.Reset();

    if (const ADungeon* OwningDungeon = Cast<ADungeon>(GetOuter())) {
        if (UDASceneDebugDataComponent* SceneDebugData = OwningDungeon->GetComponentByClass<UDASceneDebugDataComponent>()) {
            SceneDebugData->ClearDebugData();
        }
    }
}

bool UDungeonBuilder::PerformGenericSelectionLogic(const TArray<UGenericDungeonSelectorLogic*>& GenericSelectionLogics, const FDungeonMarkerInstance& InMarker) {
    for (UGenericDungeonSelectorLogic* GenericSelectionLogic : GenericSelectionLogics) {
        if (GenericSelectionLogic->SelectNode(DungeonModel, DungeonConfig, this, DungeonQuery, Random, InMarker)) {
            return true;
        }
    } 
    return false;
}

void UDungeonBuilder::CreateMarkerVisualizationRules(FDungeonMarkerVisualizationRules& OutRules) {
    OutRules = {};
}

void UDungeonBuilder::EmitCustomSceneObjects(FDungeonSceneProvider& SceneProvider) {
    ADungeon* DungeonActor = Cast<ADungeon>(GetOuter());
    // Carve voxels
    if (DungeonActor && DungeonActor->bCarveVoxels) {
        if (DungeonActor && DungeonModel && DungeonConfig) {
            FDungeonSceneProviderCommandList CommandList;
            CreateVoxelChunkCommands(DungeonActor->VoxelMeshSettings, DungeonActor->VoxelNoiseSettings, DungeonActor->VoxelSDFModel, CommandList);
        
            // Add commands to scene provider
            for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : CommandList.GetCommands()) {
                SceneProvider.AddCustomCommand(Command);
            }
        }
    }
}

void UDungeonBuilder::GetRandomStream(FRandomStream& OutRandomStream) {
    OutRandomStream = Random;
}

void UDungeonBuilder::CreateVoxelChunkCommands(const FDAVoxelMeshGenerationSettings& InVoxelMeshSettings, const FDAVoxelNoiseSettings& InVoxelNoiseSettings,
        UDungeonVoxelSDFModel* InSDFModel, FDungeonSceneProviderCommandList& OutCommands)
{
    if (!DungeonModel || !DungeonConfig) {
        return;
    }
    
    // Create voxel chunk descriptor from layout data (using shared pointer for proper memory management)
    TSharedPtr<FDAVoxelChunkDescriptor> ChunkDescriptor = MakeShareable(new FDAVoxelChunkDescriptor());
    
    // Build chunk descriptor from layout and additional shapes
    ChunkDescriptor->VoxelMeshSettings = InVoxelMeshSettings;
    ChunkDescriptor->NoiseSettings = InVoxelNoiseSettings;
    ChunkDescriptor->SDFModel = InSDFModel;

    // Initialize SDF model to Cave if not set
    if (!ChunkDescriptor->SDFModel) {
        ChunkDescriptor->SDFModel = NewObject<UDungeonVoxelSDFModel_Cave>(GetTransientPackage());
    }
    
    // Get layout data
    const FDungeonLayoutData& LayoutData = DungeonModel->DungeonLayout;
    ChunkDescriptor->BuildFromLayoutData(LayoutData, GetDungeonTransform(), {}, &Random);
    
    // Create context for commands  
    FDungeonSceneProviderContext Context;
    Context.Transform = FTransform::Identity;
    Context.NodeId = FGuid();
    Context.RandomStream = &Random;
    Context.Dungeon = Cast<ADungeon>(GetOuter());
    Context.LevelOverride = Context.Dungeon.IsValid() ? Context.Dungeon->GetLevel() : nullptr;

    OutCommands.Add(MakeShared<FDungeonSceneProviderCommand_GenerateVoxelMasterWorld>(Context, ChunkDescriptor));
}


