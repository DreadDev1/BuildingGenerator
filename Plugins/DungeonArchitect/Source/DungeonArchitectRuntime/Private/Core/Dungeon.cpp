//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Dungeon.h"

#include "Core/DungeonBuildTasks.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonConfig.h"
#include "Core/DungeonEventListener.h"
#include "Core/DungeonToolData.h"
#include "Core/Game/DungeonBuildSystem.h"
#include "Core/Utils/Debug/DungeonDebugVisualizer.h"
#include "Core/Utils/Debug/SceneDebugDataComponent.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Helpers/PCGHelpers.h"
#include "Net/UnrealNetwork.h"
#include "PCGComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(DungeonLog);

const FDungeonBuilderParameters FDungeonBuilderParameters::Default = {};

ADungeon::ADungeon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), bDrawDebugData(false)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bReplicates = true;
    bAlwaysRelevant = true;
    
    DungeonSceneRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, "SceneRoot");
    DungeonSceneRoot->SetMobility(EComponentMobility::Movable);
    RootComponent = DungeonSceneRoot;

    DungeonBoundsComponent = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, "Bounds");
    DungeonBoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DungeonBoundsComponent->SetMobility(EComponentMobility::Movable);
    DungeonBoundsComponent->PrimaryComponentTick.bCanEverTick = false;
    DungeonBoundsComponent->SetVisibility(false);
    DungeonBoundsComponent->bHiddenInGame = true;
    DungeonBoundsComponent->SetSimulatePhysics(false);
    DungeonBoundsComponent->SetGenerateOverlapEvents(false);
    DungeonBoundsComponent->SetupAttachment(RootComponent);
    
    DebugVisualizer = ObjectInitializer.CreateDefaultSubobject<UDungeonDebugVisualizer>(this, "DebugVisualizer");
    DebugVisualizer->SetupAttachment(RootComponent);
    
    PCGCache = ObjectInitializer.CreateDefaultSubobject<UDungeonPCGCache>(this, "PCGCache");
    BuildSystemComponent = ObjectInitializer.CreateDefaultSubobject<UDungeonBuildSystemDungeonComponent>(this, "DungeonBuildSystem");
    LevelStreamingModel = CreateDefaultSubobject<UDungeonLevelStreamingModel>(TEXT("LevelStreamingModel"));

    AssignNewDungeonId();
    CreateBuilderInstance(ObjectInitializer);

#if WITH_EDITORONLY_DATA
    SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

    if (!IsRunningCommandlet()) {
        // Structure to hold one-time initialization
        struct FConstructorStatics {
            ConstructorHelpers::FObjectFinderOptional<UTexture2D> DungeonSpriteObject;
            FName ID_Dungeon;
            FText NAME_Dungeon;

            FConstructorStatics()
                : DungeonSpriteObject(TEXT("/DungeonArchitect/Core/Runtime/Assets/Textures/S_DungeonActor"))
                  , ID_Dungeon(TEXT("Dungeon"))
                  , NAME_Dungeon(NSLOCTEXT("SpriteCategory", "Dungeon", "Dungeon")) {
            }
        };
        static FConstructorStatics ConstructorStatics;

        if (SpriteComponent) {
            SpriteComponent->Sprite = ConstructorStatics.DungeonSpriteObject.Get();
            SpriteComponent->SetRelativeScale3D(FVector(1, 1, 1));
            SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Dungeon;
            SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Dungeon;
            SpriteComponent->SetupAttachment(RootComponent);
            SpriteComponent->Mobility = EComponentMobility::Movable;
        }
    }

#endif //WITH_EDITORONLY_DATA

}

void ADungeon::BuildDungeon() {
    BuildDungeonWithSettings({});
}

void ADungeon::BuildDungeonWithSettings(const FDungeonBuildSettings& InBuildSettings) {
    UDungeonBuilder* DungeonBuilder = GetBuilder();
    if (!DungeonBuilder) {
        UE_LOG(DungeonLog, Error, TEXT("No builder created. Aborting dungeon build"));
        return;
    }

    ReleasePCGResources();
    ClearAllDebugVisualizations();
    
    const bool bSupportsTheming = DungeonBuilder->GetCapabilities().bSupportsTheming;
    if (bSupportsTheming && Themes.Num() == 0) {
        UE_LOG(DungeonLog, Warning, TEXT("No themes defined. Aborting dungeon build"));
        return;
    }

    if (BuildGraph.IsValid()) {
        BuildGraph->Abort();
        BuildGraph = nullptr;
    }
    
    BuildGraph = FDungeonBuildTaskUtils::CreateDungeonActorBuildGraph(this, InBuildSettings);
    BuildGraph->Start(GetWorld());
    
    Modify();
}

void ADungeon::ClearAllDebugVisualizations() {
    if (DebugVisualizer) {
        DebugVisualizer->Clear();
    }

    for (auto& Entry : TemporaryDebugVisualizers) {
        if (TObjectPtr<UDungeonDebugVisualizer> DebugVisualization = Entry.Value) {
            DebugVisualization->Clear();
            DebugVisualization->DestroyComponent();
        }
    }
    TemporaryDebugVisualizers.Reset();
}

void ADungeon::ReleasePCGResources() {
    // Clear out all the PCG components from the actor
    {
        TSet<TObjectPtr<UPCGComponent>> AllPCGComponents;
        AllPCGComponents.Append(PCGComponents);

        {
            TArray<UPCGComponent*> AttachedComponents;
            GetComponents(AttachedComponents);
            for (UPCGComponent* AttachedComponent : AttachedComponents) {
                AllPCGComponents.Add(AttachedComponent);
            }
        }
    
        for (UPCGComponent* PCGComponent : AllPCGComponents) {
            if (PCGComponent) {
                RemoveInstanceComponent(PCGComponent);
                PCGComponent->Cleanup();
                PCGComponent->DestroyComponent();
            }
        }
    }
    PCGComponents.Reset();

    if (PCGCache) {
        PCGCache->Clear();
    }

    // Remove any lingering PCG generated components in the dungeon actor
    {
        TSet<UActorComponent*> ActorComponents = GetComponents();
        for (UActorComponent* Component : ActorComponents) {
            if (Component && Component->ComponentHasTag(PCGHelpers::DefaultPCGTag)) {
                Component->DestroyComponent();
            }
        }
    }
}

void ADungeon::DestroyDungeon() {
    DungeonUtils::FDungeonEventListenerNotifier::NotifyPreDungeonDestroy(this);

    if (BuildGraph.IsValid()) {
        BuildGraph->Abort();
        BuildGraph = nullptr;
    }

    if (DebugVisualizer) {
        DebugVisualizer->Clear();
    }
    
    ReleasePCGResources();
    
    if (UDungeonCanvasComponent* CanvasComponent = GetDungeonCanvas()) {
        CanvasComponent->ReleaseResources();
    }

    if (UDungeonBuilder* DungeonBuilder = GetBuilder()) {
        DungeonBuilder->DestroyDungeon(GetModel(), GetConfig(), GetQuery(), this, GetWorld());
    }

    // Reset the model
    if (UDungeonModel* Model = GetModel()) {
        Model->Reset();
    }

    if (UDASceneDebugDataComponent* SceneDebugData = GetComponentByClass<UDASceneDebugDataComponent>()) {
        SceneDebugData->ClearDebugData();
    }
    
    DungeonUtils::FDungeonEventListenerNotifier::NotifyDungeonDestroyed(this);

    if (LevelStreamingModel) {
        LevelStreamingModel->Release(GetWorld());
    }
    
    // Destroy all actors that have the dungeon's unique id tag
    FDungeonUtils::DestroyAllDungeonOwnedActors(this);

    Modify();
}

void ADungeon::RandomizeSeed() {
    if (Config) {
        Config->Seed = FMath::Rand();
    }
}

UDungeonCanvasComponent* ADungeon::GetDungeonCanvas() const {
    return FindComponentByClass<UDungeonCanvasComponent>();
}

FDungeonBuildStatusMessage ADungeon::GetBuildStatusMessage() {
    FDungeonBuildStatusMessage Message;
    if (BuildGraph.IsValid()) {
        Message.bBuildInProgress = BuildGraph->IsRunning();

        TArray<TSharedPtr<FDADependencyGraphTask>> RunningTasks;
        BuildGraph->GetTasks(EDADependencyGraphTaskState::Running, RunningTasks);
        for (const TSharedPtr<FDADependencyGraphTask>& Task : RunningTasks) {
            TSharedPtr<FDungeonBuildTaskBase> BuildTask = StaticCastSharedPtr<FDungeonBuildTaskBase>(Task);
            if (BuildTask.IsValid() && BuildTask->SupportsProgressBar()) {
                Message.bShowProgressBar = BuildTask->SupportsProgressBar();
                Message.TotalWorkUnits += BuildTask->GetProgressTotalWorkUnits();
                Message.CompletedWorkUnits += BuildTask->GetProgressCompletedWorkUnits();;
            }
            Message.ProgressText = BuildTask->GetNotificationStatusText();
            Message.ProgressSubText = BuildTask->GetNotificationStatusSubText();
        }
    }
    return Message;
}

UDungeonBuilder* ADungeon::SetBuilderClass(TSubclassOf<UDungeonBuilder> InBuilderClass) {
    BuilderClass = InBuilderClass;
    CreateBuilderInstance();
    return Builder;
}

void ADungeon::Destroyed() {
    Super::Destroyed();

}

void ADungeon::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    if (BuildGraph.IsValid()) {
        //BuildGraph->Tick(DeltaSeconds);
        if (!BuildGraph->IsRunning()) {
            BuildGraph = nullptr;
        }
    }
    
    if (bDrawDebugData) {
        auto DungeonBuilder = GetBuilder();
        auto World = GetWorld();
        if (DungeonBuilder && World) {
            DungeonBuilder->DrawDebugData(World);
        }
    }

    ProcessLevelStreaming();
}

#if WITH_EDITOR
void ADungeon::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    const FName PropertyName = (e.Property != nullptr) ? e.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, BuilderClass)) {
        CreateBuilderInstance();

        // If we have the Editor Mode opened, broadcast that event
        OnDungeonPropertyChanged.Broadcast(this, PropertyName);
    }

    Super::PostEditChangeProperty(e);
}

void ADungeon::PostDuplicate(EDuplicateMode::Type DuplicateMode) {
    if (DuplicateMode == EDuplicateMode::Normal) {
        AssignNewDungeonId();
    }

    Super::PostDuplicate(DuplicateMode);
}

#endif

void ADungeon::PostDungeonBuild() {
    MarkPackageDirty();

    const bool bSuccess = Builder ? Builder->HasBuildSucceeded() : false;

    // Check if this is a streaming dungeon that needs to wait for initial chunks
    const bool bWaitForStreaming = bSuccess
        && LevelStreamingConfig.bEnabledLevelStreaming
        && LevelStreamingModel
        && LevelStreamingModel->GetChunks().Num() > 0;

    if (bWaitForStreaming) {
        // Defer the build complete event until initial chunks are loaded
        // The event will be fired in NotifyInitialChunksLoaded via FireDeferredBuildCompleteEvent
        bPendingBuildCompleteEvent = true;
    }
    else {
        // Non-streaming dungeon: fire immediately
        OnDungeonBuildComplete.Broadcast(this, bSuccess);
        DungeonUtils::FDungeonEventListenerNotifier::NotifyPostDungeonBuild(this);
    }
}

void ADungeon::FireDeferredBuildCompleteEvent() {
    if (bPendingBuildCompleteEvent) {
        bPendingBuildCompleteEvent = false;
        const bool bSuccess = Builder ? Builder->HasBuildSucceeded() : false;
        OnDungeonBuildComplete.Broadcast(this, bSuccess);
        DungeonUtils::FDungeonEventListenerNotifier::NotifyPostDungeonBuild(this);
    }
}

void ADungeon::InitializeQueryObject() {
    if (!Query) {
        UE_LOG(DungeonLog, Error, TEXT("Cannot initialize query object. Invalid state"));
        return;
    }
    Query->Initialize(GetConfig(), GetModel(), GetActorTransform());
}

void GetPlayerLocations(UWorld* World, TArray<FVector>& OutPlayerLocations) {
    
}

void ADungeon::ProcessLevelStreaming() const {
    if (LevelStreamingModel) {
        UWorld* World = GetWorld();
        //if (World->IsGameWorld()) {
            FDungeonLevelStreamer::Process(World, LevelStreamingConfig, LevelStreamingModel);
        //}
    }
}

void ADungeon::ResetDungeonId()
{
    AssignNewDungeonId();
}

void ADungeon::AssignNewDungeonId()
{
    Uid = FGuid::NewGuid();
}

template <typename T>
T* CreateNewObject(const FObjectInitializer& ObjectInitializer, UObject* Outer, UClass* ImplClass) {
    if (!ImplClass) {
        return nullptr;
    }
    UObject* Instance = ObjectInitializer.CreateDefaultSubobject(Outer, FName(*ImplClass->GetName()), T::StaticClass(),
                                                                 ImplClass, true, false);
    return Cast<T>(Instance);
}

template <typename T>
T* CreateNewObject(UObject* Outer, UClass* ImplClass) {
    return NewObject<T>(Outer, ImplClass, FName(*ImplClass->GetName()));
}

void ADungeon::GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent, bool bIncludeFromChildActors) const {
    Super::GetActorBounds(bOnlyCollidingComponents, Origin, BoxExtent, bIncludeFromChildActors);
    
}

void ADungeon::CreateBuilderInstance() {
    if (BuilderClass == nullptr) {
        // Set to the default builder
        BuilderClass = UDungeonBuilder::DefaultBuilderClass();
    }

    if (Builder == nullptr || !Builder->IsValidLowLevel() || Builder->GetClass() != BuilderClass) {
        Builder = CreateNewObject<UDungeonBuilder>(this, BuilderClass);
        DungeonModel = CreateNewObject<UDungeonModel>(this, Builder->GetModelClass());
        Config = CreateNewObject<UDungeonConfig>(this, Builder->GetConfigClass());
        ToolData = CreateNewObject<UDungeonToolData>(this, Builder->GetToolDataClass());
        Query = CreateNewObject<UDungeonQuery>(this, Builder->GetQueryClass());
        
        InitializeQueryObject();
    }

    LevelStreamingModel->Release(GetWorld());
}

void ADungeon::CreateBuilderInstance(const FObjectInitializer& ObjectInitializer) {
    if (BuilderClass == nullptr) {
        // Set to the default builder
        BuilderClass = UDungeonBuilder::DefaultBuilderClass();
    }

    if (Builder == nullptr || !Builder->IsValidLowLevel() || Builder->GetClass() != BuilderClass) {
        Builder = CreateNewObject<UDungeonBuilder>(ObjectInitializer, this, BuilderClass);
        DungeonModel = CreateNewObject<UDungeonModel>(ObjectInitializer, this, Builder->GetModelClass());
        Config = CreateNewObject<UDungeonConfig>(ObjectInitializer, this, Builder->GetConfigClass());
        ToolData = CreateNewObject<UDungeonToolData>(ObjectInitializer, this, Builder->GetToolDataClass());
        Query = CreateNewObject<UDungeonQuery>(ObjectInitializer, this, Builder->GetQueryClass());
        InitializeQueryObject();
    }

}

TObjectPtr<UDungeonBuildSystemDungeonComponent> ADungeon::GetBuildSystemComponent() const {
    return BuildSystemComponent;
}

UDungeonQuery* ADungeon::GetQuery() {
    if (!Query || (Builder && Query->GetClass() != Builder->GetQueryClass())) {
        if (Builder) {
            Query = CreateNewObject<UDungeonQuery>(this, Builder->GetQueryClass());
            InitializeQueryObject();
        }
        else {
            UE_LOG(DungeonLog, Error,
                   TEXT("No builder created. Cannot fetch query object.  Rebuild the dungeon to fix this"));
        }
    }
    return Query;
}

void ADungeon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ADungeon, BuildSystemComponent);
}

#if WITH_EDITORONLY_DATA
/** Returns SpriteComponent subobject **/
UBillboardComponent* ADungeon::GetSpriteComponent() const { return SpriteComponent; }
#endif


TWeakPtr<const FDADependencyGraph> ADungeon::GetDependencyGraph() const {
    return BuildGraph;
}

UDungeonSpawnDataComponent::UDungeonSpawnDataComponent()
    : MarkerIndex(INDEX_NONE)
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = false;
    bAutoActivate = false;
}

