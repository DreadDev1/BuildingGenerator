//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Widgets/SThemePreviewViewport.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/GridFlow/GridFlowConfig.h"
#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/ThemeGraphAppMode.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/Visualization/ThemeEditorMarkerVisualizer.h"
#include "Core/Editors/ThemeEditor/Widgets/DungeonEditorViewportClient.h"
#include "Core/Editors/ThemeEditor/Widgets/DungeonThemePreviewScene.h"
#include "Core/Editors/ThemeEditor/Widgets/SDungeonEditorViewportToolbar.h"
#include "Core/Utils/DungeonModelHelper.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"

#include "AdvancedPreviewScene.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/LightComponentBase.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "PreviewScene.h"
#include "Widgets/Docking/SDockTab.h"

void SThemePreviewViewport::Construct(const FArguments& InArgs) {
    bRequestRebuildPreviewMesh = false;
    ThemeEditorPtr = InArgs._DungeonEditor;
    ObjectToEdit = InArgs._ObjectToEdit;
    bDrawDebugData = false;

    {
        FAdvancedPreviewScene::ConstructionValues CVS;
        CVS.bCreatePhysicsScene = false;
        CVS.LightBrightness = 3;
        CVS.SkyBrightness = 1;
        //PreviewScene = MakeShareable(new FAdvancedPreviewScene(CVS));
        PreviewScene = MakeShareable(new FDungeonThemePreviewScene(CVS));
        
        PreviewScene->SetFloorVisibility(false);

        // Make sure the floor is not visible even if enabled from the properties tab
        PreviewScene->SetFloorOffset(-100000);
    }
    SEditorViewport::Construct(SEditorViewport::FArguments());

    UWorld* World = PreviewScene->GetWorld();
    
    // Spawn the dungeon actor.  The details panel shows the dungeon actor, so no need to set the config. it's set by the user. we'll have to save those values back to PreviewViewportProperties
    Dungeon = World->SpawnActor<AThemeEditorDungeonActor>();
    Dungeon->Themes = { ObjectToEdit };
    Dungeon->InitFromPreviewViewportSettings(ObjectToEdit->PreviewViewportProperties);
    
    // Request a dungeon build in the next tick update
    bRequestRebuildPreviewMesh = true;
    
    Skylight = NewObject<USkyLightComponent>();
    //PreviewScene->AddComponent(Skylight, FTransform::Identity);

    AtmosphericFog = NewObject<USkyAtmosphereComponent>();
    PreviewScene->AddComponent(AtmosphericFog, FTransform::Identity);
    PreviewScene->DirectionalLight->SetMobility(EComponentMobility::Movable);
    PreviewScene->DirectionalLight->CastShadows = true;
    PreviewScene->DirectionalLight->CastStaticShadows = true;
    PreviewScene->DirectionalLight->CastDynamicShadows = true;
    PreviewScene->DirectionalLight->SetIntensity(3);
    PreviewScene->SetSkyBrightness(1.0f);

    // Add the visualizer actor
    Visualizer = World->SpawnActor<AThemeEditorMarkerVisualizer>();
}

SThemePreviewViewport::~SThemePreviewViewport() {
    FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
    if (EditorViewportClient.IsValid()) {
        EditorViewportClient->Viewport = nullptr;
    }
}

void SThemePreviewViewport::AddReferencedObjects(FReferenceCollector& Collector) {
    Collector.AddReferencedObject(ObjectToEdit);
    Collector.AddReferencedObject(Visualizer);
    Collector.AddReferencedObject(Dungeon);
}

FString SThemePreviewViewport::GetReferencerName() const {
    static const FString NameString = TEXT("SThemePreviewViewport");
    return NameString;
}

void SThemePreviewViewport::SetParentTab(const FName& InAppMode, TSharedRef<SDockTab> InParentTab) {
    if (!ParentTabs.Contains(InAppMode)) {
        ParentTabs.Add(InAppMode, nullptr);
    }
    
    TWeakPtr<SDockTab>& ParentTabRef = ParentTabs.FindOrAdd(InAppMode);
    ParentTabRef = InParentTab;
}

void SThemePreviewViewport::RebuildDungeon(const FDungeonBuildSettings& InBuildSettings) {
    bRequestRebuildPreviewMesh = true;
    BuildSettings = InBuildSettings;
}

void SThemePreviewViewport::OnShowPropertyDungeon() const {
    ShowObjectProperties(Dungeon);
}

void SThemePreviewViewport::OnShowPropertySkylight() const {
    ShowObjectProperties(Skylight);
}

void SThemePreviewViewport::OnShowPropertyDirectionalLight() const {
    ShowObjectProperties(PreviewScene->DirectionalLight);
}

void SThemePreviewViewport::OnShowPropertyAtmosphericFog() const {
    ShowObjectProperties(AtmosphericFog);
}

void SThemePreviewViewport::OnToggleDebugData() {
    bDrawDebugData = !bDrawDebugData;
}

bool SThemePreviewViewport::HandleToggleDebugDataIsChecked() const {
    return bDrawDebugData;
}

void SThemePreviewViewport::ShowObjectProperties(UObject* Object, bool bForceRefresh) const {
    const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
    if (ThemeEditor.IsValid()) {
        const TSharedPtr<FThemeEditorAppModeBase> AppMode = StaticCastSharedPtr<FThemeEditorAppModeBase>(ThemeEditor->GetCurrentModePtr());
        if (AppMode.IsValid()) {
            AppMode->ShowObjectDetails(Object, bForceRefresh);
        }
    }
}

void SThemePreviewViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
                                  const float InDeltaTime) {
    SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    if (bRequestRebuildPreviewMesh) {
        bRequestRebuildPreviewMesh = false;
        if (Dungeon) {
            Dungeon->BuildDungeonWithSettings(BuildSettings);
        }
    }
}

EVisibility SThemePreviewViewport::GetToolbarVisibility() const {
    return EVisibility::Visible;
}

void SThemePreviewViewport::VisualizeMarkers(const TSet<FName>& InMarkerNames) const {
    UDungeonConfig* DungeonConfig = Dungeon->GetConfig();
    UDungeonBuilder* DungeonBuilder = Dungeon->GetBuilder();
    UDungeonModel* DungeonModel = Dungeon->GetModel();
    if (Visualizer && DungeonBuilder && DungeonConfig && DungeonModel) {
        Visualizer->BuildVisualization(DungeonConfig, DungeonModel, DungeonBuilder, InMarkerNames);
    }
}

void SThemePreviewViewport::ClearMarkerVisualizations() const {
    if (Visualizer) {
        Visualizer->ClearVisualization();
    }
}

void SThemePreviewViewport::ZoomOnNearestNodeMesh(const FName& InNodeId) const {
    if (GetViewportClient().IsValid()) {
        FEditorViewportClient* ViewportClient = GetViewportClient().Get();
        if (AActor* NearestMeshActor = FDungeonLevelEditorUtils::FindNearestNodeMesh(ViewportClient, InNodeId)) {
            FDungeonLevelEditorUtils::FocusViewportOnActor(ViewportClient, NearestMeshActor);
        }
    }
}

TSharedRef<FEditorViewportClient> SThemePreviewViewport::MakeEditorViewportClient() {
    EditorViewportClient = MakeShareable(
        new FDungeonEditorViewportClient(SharedThis(this), *PreviewScene, ObjectToEdit));

    EditorViewportClient->bSetListenerPosition = false;
    EditorViewportClient->SetRealtime(true); // TODO: Check if real-time is needed
    EditorViewportClient->VisibilityDelegate.BindSP(this, &SThemePreviewViewport::IsVisible);

    return EditorViewportClient.ToSharedRef();
}

EVisibility SThemePreviewViewport::OnGetViewportContentVisibility() const {
    return EVisibility::Visible;
}

void SThemePreviewViewport::BindCommands() {
    SEditorViewport::BindCommands();

    const FDungeonEditorViewportCommands& ViewportActions = FDungeonEditorViewportCommands::Get();
    CommandList->MapAction(
        ViewportActions.ShowPropertyDungeon,
        FExecuteAction::CreateSP(this, &SThemePreviewViewport::OnShowPropertyDungeon));

    CommandList->MapAction(
        ViewportActions.ShowPropertySkylight,
        FExecuteAction::CreateSP(this, &SThemePreviewViewport::OnShowPropertySkylight));

    CommandList->MapAction(
        ViewportActions.ShowPropertyDirectionalLight,
        FExecuteAction::CreateSP(this, &SThemePreviewViewport::OnShowPropertyDirectionalLight));

    CommandList->MapAction(
        ViewportActions.ShowPropertyAtmosphericFog,
        FExecuteAction::CreateSP(this, &SThemePreviewViewport::OnShowPropertyAtmosphericFog));

    CommandList->MapAction(
        ViewportActions.ToggleDebugData,
        FExecuteAction::CreateSP(this, &SThemePreviewViewport::OnToggleDebugData));

    CommandList->MapAction(
        ViewportActions.DrawDebugData,
        FExecuteAction::CreateSP(this, &SThemePreviewViewport::OnToggleDebugData),
        FCanExecuteAction(),
        FIsActionChecked::CreateSP(this, &SThemePreviewViewport::HandleToggleDebugDataIsChecked));
}

void SThemePreviewViewport::OnFocusViewportToSelection() {
    SEditorViewport::OnFocusViewportToSelection();
}

TSharedPtr<SWidget> SThemePreviewViewport::BuildViewportToolbar() {
    // Build our toolbar level toolbar
    TSharedRef<SDungeonEditorViewportToolBar> ToolBar =
        SNew(SDungeonEditorViewportToolBar)
		.Viewport(SharedThis(this))
		.Visibility(this, &SThemePreviewViewport::GetToolbarVisibility)
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute());

    return
        SNew(SVerticalBox)
        .Visibility(EVisibility::SelfHitTestInvisible)
        + SVerticalBox::Slot()
          .AutoHeight()
          .VAlign(VAlign_Top)
        [
            ToolBar
        ];
}

bool SThemePreviewViewport::IsVisible() const {
    if (ViewportWidget.IsValid()) {
        for (const auto& Entry : ParentTabs) {
            TWeakPtr<SDockTab> ParentTabPtr = Entry.Value;
            TSharedPtr<SDockTab> ParentTab = ParentTabPtr.Pin();
            if (ParentTab.IsValid()) {
                if (ParentTab->IsForeground()) {
                    return true;
                }
            }
        }
    }
    return false;
}

void SThemePreviewViewport::InitializeCamera() const {
    if (IsValid(ObjectToEdit) && IsValid(ObjectToEdit->PreviewViewportProperties)
            && ObjectToEdit->PreviewViewportProperties->bViewTransformValid)
    {
        const FDungeonEditorViewportCamTransform& CamTransform = ObjectToEdit->PreviewViewportProperties->ViewTransform;
        EditorViewportClient->SetInitialViewTransform(LVT_Perspective,
            CamTransform.Location, CamTransform.Rotation, 1.0f);

        EditorViewportClient->GetViewTransform().TransitionToLocation(CamTransform.Location, nullptr, true);
        EditorViewportClient->SetLookAtLocation(CamTransform.LookAt, true);
    }
    else {
        FVector Extent(1000, 1000, 400);
        EditorViewportClient->FocusViewportOnBox(FBox(FVector(-Extent), FVector(Extent)));
    }
}


void SThemePreviewViewport::CleanupModifiedNodeObjects() {
    const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(nullptr);
    for (const FName& NodeId : NodeObjectsToRebuild) {
        const FName NodeTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(NodeId);
        // Destroy all actors that have a "Dungeon" tag
        for (TActorIterator<AActor> ActorItr(PreviewScene->GetWorld()); ActorItr; ++ActorItr) {
            if (ActorItr->ActorHasTag(DungeonTag) && ActorItr->ActorHasTag(NodeTag)) {
                ActorItr->Destroy();
            }
        }
    }
    NodeObjectsToRebuild.Reset();
}

void SThemePreviewViewport::SetNodesToRebuild(const TSet<FName>& NodeIds) {
    NodeObjectsToRebuild.Append(NodeIds);
}

