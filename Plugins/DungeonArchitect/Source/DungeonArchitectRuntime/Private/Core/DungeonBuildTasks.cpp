//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/DungeonBuildTasks.h"

#include "Core/Dungeon.h"
#include "Core/DungeonConfig.h"
#include "Core/DungeonEventListener.h"
#include "Core/Utils/DungeonUtils.h"
#include "Core/Volumes/DungeonThemeOverrideVolume.h"
#include "Frameworks/PCG/DungeonPCGLib.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProvider.h"

#include "Components/BoxComponent.h"
#include "Data/PCGPointData.h"
#include "PCGComponent.h"
#include "PCGGraph.h"

#define LOCTEXT_NAMESPACE "FDungeonBuildTasks"


DEFINE_LOG_CATEGORY_STATIC(LogDungeonBuildGraph, Log, All);

/////////////////// FDungeonBuilderTask ///////////////////
FDungeonBuildTaskBase::FDungeonBuildTaskBase(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings)
	: Dungeon(InDungeon)
	, BuildSettings(InBuildSettings)
{
}

////////////////////// Task - Generate Layout //////////////////////
FDungeonBuildTask_GenerateLayout::FDungeonBuildTask_GenerateLayout(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings)
	: FDungeonBuildTaskBase(InDungeon, InBuildSettings)
{
	DebugName = TEXT("GenerateLayout");	
}

void FDungeonBuildTask_GenerateLayout::Run() {
	if (Dungeon.IsValid()) {
		// Initiate the generation
		ADungeon* DungeonPtr = Dungeon.Get();
		UWorld* World = DungeonPtr ? DungeonPtr->GetWorld() : nullptr;
		if (!DungeonPtr || !World) return;

		DungeonPtr->CreateBuilderInstance();

		// Notify pre-build dungeon event
		DungeonUtils::FDungeonEventListenerNotifier::NotifyPreDungeonBuild(DungeonPtr);

		if (UDungeonBuilder* DungeonBuilder = DungeonPtr->GetBuilder()) {
			DungeonBuilder->BuildDungeon(DungeonPtr, World, BuildSettings.BuilderParameters);

			if (UDungeonCanvasComponent* CanvasComponent = DungeonPtr->GetDungeonCanvas()) {
				CanvasComponent->Initialize(DungeonPtr);
			}
		}
		
		if (IsGenerationDone()) {
			State = EDADependencyGraphTaskState::Complete;
			HandleCompleteTask();
		}
	}
	else {
		UE_LOG(LogDungeonBuildGraph, Error, TEXT("Aborting build.   Missing Dungeon reference"));
		RequestAbortGraph();
		State = EDADependencyGraphTaskState::Complete;
	}
}

void FDungeonBuildTask_GenerateLayout::Tick(float DeltaTime) {
	if (IsGenerationDone()) {
		State = EDADependencyGraphTaskState::Complete;
		HandleCompleteTask();
	}
}

FText FDungeonBuildTask_GenerateLayout::GetNotificationStatusText() const {
	static const FText StatusText = LOCTEXT("StatusText_GenLayout", "Generating Layout");
	return StatusText;
}

bool FDungeonBuildTask_GenerateLayout::IsGenerationDone() const {
	if (Dungeon.IsValid()) {
		if (const UDungeonBuilder* DungeonBuilder = Dungeon->GetBuilder()) {
			if (DungeonBuilder->IsGenerating()) {
				return false;
			}
		}
	}
	return true;
}

void FDungeonBuildTask_GenerateLayout::HandleCompleteTask() const {
	// Store a copy of the base layout markers, before the theme engine runs
	if (UDungeonModel* DungeonModel = Dungeon->GetModel()) {
		DungeonModel->CachedBaseLayoutMarkers = DungeonModel->WorldMarkers;
	}
}

void FDungeonBuildTask_GenerateLayout::Abort() {
	if (!IsGenerationDone()) {
		if (Dungeon.IsValid()) {
			if (UDungeonBuilder* DungeonBuilder = Dungeon->GetBuilder()) {
				DungeonBuilder->AbortGeneration();
			}
		}
	}
}

////////////////////// Task - Setup PCG Components //////////////////////
FDungeonBuildTask_SetupPCG::FDungeonBuildTask_SetupPCG(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings)
	: FDungeonBuildTaskBase(InDungeon, InBuildSettings)
{
}

void FDungeonBuildTask_SetupPCG::Run() {
	// Setup the PCG system for use
	if (Dungeon.IsValid()) {
		Dungeon->ReleasePCGResources();

		if (UDungeonCanvasComponent* DungeonCanvas = Dungeon->GetDungeonCanvas()) {
			auto DungeonLayoutBounds = DungeonCanvas->GetDungeonLayoutBounds();
			if (Dungeon->DungeonBoundsComponent) {
				Dungeon->DungeonBoundsComponent->SetRelativeLocation(DungeonLayoutBounds.GetCenter());

				// Make the extent a square, since our textures are square
				{
					FVector Extent = DungeonLayoutBounds.GetExtent();
					const float ExtentSize = FMath::Max(Extent.X, Extent.Y);
					Extent.X = Extent.Y = ExtentSize;
					Dungeon->DungeonBoundsComponent->SetBoxExtent(Extent);
				}
			}
		}
	}
	
	State = EDADependencyGraphTaskState::Complete;
}

FText FDungeonBuildTask_SetupPCG::GetNotificationStatusText() const {
	static const FText StatusText = LOCTEXT("StatusText_SetupPCG", "Setting up PCG");
	return StatusText;
}

////////////////////// Task - Generate PCG //////////////////////

FDungeonBuildTask_GeneratePCGGraph::FDungeonBuildTask_GeneratePCGGraph(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings, UPCGGraph* InPCGGraph)
	: FDungeonBuildTaskBase(InDungeon, InBuildSettings)
	, PCGGraph(InPCGGraph)
{
	DebugName = TEXT("GeneratePCGGraph");
}

void FDungeonBuildTask_GeneratePCGGraph::Run() {
	if (!PCGGraph.IsValid()) {
		// The graph is empty, nothing to process here
		State = EDADependencyGraphTaskState::Complete;
		return;
	}
	
	// Create a PCG Component to run the graph
	UPCGComponent* PCGComponent = NewObject<UPCGComponent>(Dungeon.Get());
	PCGComponent->SetGraph(PCGGraph.Get());
	PCGComponent->RegisterComponent();
	
	Dungeon->AddInstanceComponent(PCGComponent);
	Dungeon->PCGComponents.Add(PCGComponent);
	Dungeon->PCGCache->CreateComponentCache(PCGComponent);
	
	// Setup the marker list on the component cache
	if (FDungeonPCGComponentCache* Cache = Dungeon->PCGCache->GetComponentCache(PCGComponent)) {
		if (UDungeonModel* DungeonModel = Dungeon->GetModel()) {
			Cache->InputWorldMarkers = DungeonModel->WorldMarkers;
		}
	}

	// Generate the PCG graph
	PCGComponent->Generate(true);

	PCGComponentPtr = PCGComponent;
}

void FDungeonBuildTask_GeneratePCGGraph::Tick(float DeltaTime) {
	if (PCGComponentPtr.IsValid()) {
		if (!PCGComponentPtr->IsGenerating() && !PCGComponentPtr->IsCleaningUp()) {
			HandleGenerationComplete();
		}
	}
	else {
		// Stale state. Finish the task generation
		State = EDADependencyGraphTaskState::Complete;
	}
}

FText FDungeonBuildTask_GeneratePCGGraph::GetNotificationStatusText() const {
	static const FText StatusText = LOCTEXT("StatusText_GeneratePCGGraph", "Generating PCG");
	return StatusText;
}

FText FDungeonBuildTask_GeneratePCGGraph::GetNotificationStatusSubText() const {
	static const FText SubTextTemplate = LOCTEXT("StatusText_GeneratePCGGraphTemplate", "Generating Graph: {0}");
	return FText::Format(SubTextTemplate, PCGGraph.IsValid() ? FText::FromString(PCGGraph->GetName()) : LOCTEXT("NullFallbackGraphName", "UNKNOWN"));
}

void FDungeonBuildTask_GeneratePCGGraph::HandleGenerationComplete() {
	if (PCGComponentPtr.IsValid()) {
		if (UDungeonModel* DungeonModel = Dungeon.IsValid() ? Dungeon->GetModel() : nullptr) {
			// Get the data and convert it to marker list. Then override the builder's marker list with the PCG driven marker list
			const FPCGDataCollection& GeneratedOutput = PCGComponentPtr->GetGeneratedGraphOutput();
			TArray<FDungeonMarkerInstance> NewMarkerList;
    
			for (const FPCGTaggedData& TaggedData : GeneratedOutput.TaggedData) {
				if (const UPCGPointData* PointData = Cast<UPCGPointData>(TaggedData.Data)) {
					FDungeonPCGLib::ReadMarkersFromPCGPointData(PointData, NewMarkerList);
				}
			}

			// Override the builder's marker list with the new PCG-driven marker list
			if (DungeonModel && NewMarkerList.Num() > 0) {
				DungeonModel->WorldMarkers = NewMarkerList;
			}
		}
	}
	
	// Store a copy of the base layout markers, before the theme engine runs
	if (UDungeonModel* DungeonModel = Dungeon->GetModel()) {
		DungeonModel->CachedBaseLayoutMarkers = DungeonModel->WorldMarkers;
	}
	
	State = EDADependencyGraphTaskState::Complete;
}

////////////////////// Task - Spawn Scene Items //////////////////////
FDungeonBuildTask_SpawnSceneItems::FDungeonBuildTask_SpawnSceneItems(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings)
	: FDungeonBuildTaskBase(InDungeon, InBuildSettings)
{
	DebugName = TEXT("SpawnSceneItems");
}

void FDungeonBuildTask_SpawnSceneItems::Run() {
	SceneProvider = nullptr;

	FDungeonState DungeonState = GetDungeonState();
	
	if (!DungeonState.IsValid()) {
		State = EDADependencyGraphTaskState::Complete;
		return;
	}

	// Restore the cached base markers (if we're rebuilding only the theme modifications)
	if (UDungeonModel* DungeonModel = DungeonState.Model.Get()) {
		if (DungeonModel->CachedBaseLayoutMarkers.Num() > 0) {
			DungeonModel->WorldMarkers = DungeonModel->CachedBaseLayoutMarkers;
		}
	}
	
	UDungeonBuilder* DungeonBuilder = DungeonState.Builder.Get();
	UDungeonConfig* DungeonConfig = DungeonState.Config.Get();
	ADungeon* DungeonActor = DungeonState.Dungeon.Get();
	
	const bool bSupportsTheming = DungeonBuilder && DungeonBuilder->GetCapabilities().bSupportsTheming;
	if (DungeonBuilder && DungeonConfig && bSupportsTheming) {
		// Create a scene provider and run the theme engine. The wait for the build commands to complete in HandleItemSpawning()
		bool bInstanced = DungeonConfig ? DungeonConfig->Instanced : false;
		if (BuildSettings.bOverrideInstancedSetting) {
			bInstanced = BuildSettings.bInstanced;
		}
		
		SceneProvider = DungeonBuilder->CreateSceneProvider(bInstanced);

		if (SceneProvider.IsValid()) {
			FVector BuildPriorityLocation = DungeonActor ? DungeonActor->BuildPriorityLocation : FVector::Zero();
			UWorld* World = DungeonState.World.Get();
			if (World && World->IsGameWorld()) {
				BuildPriorityLocation = FDungeonUtils::GetPlayerViewportLocation(World);	
			}
			
			SceneProvider->SetBuildPriorityLocation(BuildPriorityLocation);
			TArray<FClusterThemeInfo> ClusterThemes = {};
			const FTransform DungeonTransform = DungeonActor ? DungeonActor->GetActorTransform() : FTransform::Identity;
			
			// Prepare the Theme Engine settings
			FDungeonThemeEngineSettings ThemeEngineSettings;
			ThemeEngineSettings.Themes = DungeonState.Themes;
			ThemeEngineSettings.ClusteredThemes = ClusterThemes;
			ThemeEngineSettings.SceneProvider = SceneProvider;
			ThemeEngineSettings.MarkerEmitters = IsValid(DungeonActor) ? DungeonActor->MarkerEmitters : TArray<TObjectPtr<UDungeonMarkerEmitter>>{};
			ThemeEngineSettings.Dungeon = DungeonActor;
			ThemeEngineSettings.ChunkId = DungeonState.ChunkTag;
			ThemeEngineSettings.LevelOverride = DungeonActor ? DungeonActor->GetLevel() : nullptr;
			ThemeEngineSettings.MarkerGenerator = DungeonBuilder->CreateMarkerGenProcessor(DungeonTransform);
			
			if (DungeonActor) {
				ThemeEngineSettings.bRoleAuthority = DungeonActor->HasAuthority(); 
			}

			// Grab the theme override volumes
			if (World) {
				for (TActorIterator<ADungeonThemeOverrideVolume> VolumeIt(World); VolumeIt; ++VolumeIt) {
					ADungeonThemeOverrideVolume* ThemeOverrideVolume = *VolumeIt;
					if (!IsValid(ThemeOverrideVolume) || !ThemeOverrideVolume->IsValidLowLevel()) {
						continue;
					}
					const bool bValid = !DungeonActor || (ThemeOverrideVolume->Dungeon == DungeonActor);
					if (bValid && ThemeOverrideVolume->ThemeOverride) {
						ThemeEngineSettings.ThemeOverrideVolumes.Add(ThemeOverrideVolume);
					}
				}
			}
			
			DungeonBuilder->ApplyDungeonTheme(ThemeEngineSettings);
		}
		else {
			State = EDADependencyGraphTaskState::Complete;
		}
	}
	else {
		State = EDADependencyGraphTaskState::Complete;
	}
}

void FDungeonBuildTask_SpawnSceneItems::Tick(float DeltaTime) {
	FDungeonState DungeonState = GetDungeonState();
	if (SceneProvider.IsValid() && DungeonState.IsValid()) {
		UDungeonConfig* Config = DungeonState.Config.Get();
		UWorld* World = DungeonState.World.Get();
		float MaxBuildTimePerFrameMs = BuildSettings.bOverrideFrameBuildTimeMs
			? BuildSettings.FrameBuildTimeMs
			: (Config ? Config->MaxBuildTimePerFrameMs : 0);
		
		SceneProvider->RunGameThreadCommands(World, MaxBuildTimePerFrameMs);
		if (!SceneProvider->IsRunningGameThreadCommands()) {
			State = EDADependencyGraphTaskState::Complete;
		}
	}
	else {
		State = EDADependencyGraphTaskState::Complete;
	}
}

FText FDungeonBuildTask_SpawnSceneItems::GetNotificationStatusText() const {
	static const FText StatusText = LOCTEXT("StatusText_SpawnItems", "Spawning Actors");
	return StatusText;
}

FText FDungeonBuildTask_SpawnSceneItems::GetNotificationStatusSubText() const {
	FFormatNamedArguments Args;
	Args.Add(TEXT("Complete"), GetProgressCompletedWorkUnits());
	Args.Add(TEXT("Total"), GetProgressTotalWorkUnits());
	return FText::Format(LOCTEXT("ProgressLabel", "Spawning Items: {Complete} / {Total}"), Args);
}

int32 FDungeonBuildTask_SpawnSceneItems::GetProgressTotalWorkUnits() const {
	return SceneProvider.IsValid() ? SceneProvider->GetTotalWorkUnits() : 0;
}

int32 FDungeonBuildTask_SpawnSceneItems::GetProgressCompletedWorkUnits() const {
	return SceneProvider.IsValid() ? SceneProvider->GetCompletedWorkUnits() : 0;
}

void FDungeonBuildTask_SpawnSceneItems::SetDungeonStateOverride(const FDungeonState& InStateOverride) {
	check(InStateOverride.IsValid());
	DungeonStateOverride = InStateOverride;
	bUsesDungeonStateOverride = true;
}

FDungeonBuildTask_SpawnSceneItems::FDungeonState FDungeonBuildTask_SpawnSceneItems::GetDungeonState() {
	FDungeonBuildTask_SpawnSceneItems::FDungeonState NewState;
	if (bUsesDungeonStateOverride || !Dungeon.IsValid()) {
		NewState = DungeonStateOverride;
	}
	else {
		NewState.Dungeon = Dungeon;
		NewState.Builder = Dungeon->GetBuilder();
		NewState.Config = Dungeon->GetConfig();
		NewState.Model = Dungeon->GetModel();
		NewState.World = Dungeon->GetWorld();
		NewState.Themes = Dungeon->Themes;
		if (Dungeon->bCarveVoxels) {
			if (UDungeonThemeAsset* VoxelShapeTheme = Dungeon->VoxelMeshSettings.VoxelShapeTheme.LoadSynchronous()) {
				NewState.Themes.Add(VoxelShapeTheme);
			}
		}
	}
	return NewState;
}

////////////////////// Task - Finalize Build //////////////////////
FDungeonBuildTask_FinalizeBuild::FDungeonBuildTask_FinalizeBuild(ADungeon* Dungeon, const FDungeonBuildSettings& InBuildSettings)
	: FDungeonBuildTaskBase(Dungeon, InBuildSettings)
{
	DebugName = TEXT("FinalizeBuild");
}

void FDungeonBuildTask_FinalizeBuild::Run() {
	if (Dungeon.IsValid()) {
		Dungeon->PostDungeonBuild();
	}
	State = EDADependencyGraphTaskState::Complete;
}

////////////////////// FDungeonBuildTaskUtils //////////////////////
TSharedPtr<FDADependencyGraph> FDungeonBuildTaskUtils::CreateDungeonActorBuildGraph(ADungeon* InDungeon, const FDungeonBuildSettings& InBuildSettings) {
	TSharedPtr<FDADependencyGraph> Graph = MakeShared<FDADependencyGraph>();
	if (InDungeon) {
		TSharedPtr<FDADependencyGraphTask> PreviousTask;

		auto CanExecutePhase = [&](EDungeonBuildGraphPhase InPhase) {
			return InBuildSettings.StartPhase <= InPhase && InBuildSettings.EndPhase >= InPhase;
		};
		
		// Generate the Layout
		if (CanExecutePhase(EDungeonBuildGraphPhase::GenerateLayout)) {
			// Create the build task that will eventually run this task
			TSharedPtr<FDungeonBuildTask_GenerateLayout> TaskGenerateLayout = MakeShared<FDungeonBuildTask_GenerateLayout>(InDungeon, InBuildSettings);
			Graph->RegisterTask(TaskGenerateLayout);
			PreviousTask = TaskGenerateLayout;
		}
		
		// Setup the PCG components
		if (CanExecutePhase(EDungeonBuildGraphPhase::SetupPCG)) {
			TSharedPtr<FDungeonBuildTask_SetupPCG> TaskSetupPCG = MakeShared<FDungeonBuildTask_SetupPCG>(InDungeon, InBuildSettings);
			TaskSetupPCG->AddDependency(PreviousTask);
			Graph->RegisterTask(TaskSetupPCG);
			PreviousTask = TaskSetupPCG;
		}

		// Generate the PCG graph
		if (CanExecutePhase(EDungeonBuildGraphPhase::GeneratePCG)) {
			for (UPCGGraph* PCGGraph : InDungeon->PCGGraphs) {
				if (PCGGraph) {
					TSharedPtr<FDungeonBuildTask_GeneratePCGGraph> TaskGeneratePCG = MakeShared<FDungeonBuildTask_GeneratePCGGraph>(InDungeon, InBuildSettings, PCGGraph);
					TaskGeneratePCG->AddDependency(PreviousTask);
					Graph->RegisterTask(TaskGeneratePCG);
					PreviousTask = TaskGeneratePCG;
				}
			}
		}

		// Spawn the item
		if (CanExecutePhase(EDungeonBuildGraphPhase::SpawnItems)) {
			TSharedPtr<FDungeonBuildTask_SpawnSceneItems> TaskGenerateSpawnItems = MakeShared<FDungeonBuildTask_SpawnSceneItems>(InDungeon, InBuildSettings);
			TaskGenerateSpawnItems->AddDependency(PreviousTask);
			Graph->RegisterTask(TaskGenerateSpawnItems);
			PreviousTask = TaskGenerateSpawnItems;
		}

		// Finalize
		//if (CanExecutePhase(EDungeonBuildGraphPhase::Finalize))
		{
			TSharedPtr<FDungeonBuildTask_FinalizeBuild> TaskFinalizeBuild = MakeShared<FDungeonBuildTask_FinalizeBuild>(InDungeon, InBuildSettings);
			TaskFinalizeBuild->AddDependency(PreviousTask);
			Graph->RegisterTask(TaskFinalizeBuild);
			PreviousTask = TaskFinalizeBuild;
		}
	}

	return Graph;
}

#undef LOCTEXT_NAMESPACE

