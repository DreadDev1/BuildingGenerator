//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Core/Common/DungeonArchitectCommands.h"
#include "Core/Common/Utils/DungeonEditorConstants.h"
#include "Core/DungeonBuilder.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/MarkerGeneratorAppMode.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/MarkerGeneratorAppModeCommands.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeEditorAppModeBase.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/ThemeGraphAppMode.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Core/Editors/ThemeEditor/Widgets/SThemePreviewViewport.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "EdGraph/EdGraph.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/SubclassOf.h"
#include "Toolkits/IToolkitHost.h"
#include "Widgets/Layout/SSpacer.h"
#include "WorkflowOrientedApp/SModeWidget.h"

#define LOCTEXT_NAMESPACE "DungeonArchitectThemeEditor"

class UEdGraphNode_DungeonThemeMesh;
class UEdGraphNode_DungeonThemeActorTemplate;
DEFINE_LOG_CATEGORY_STATIC(DungeonEditor, Log, All);

const FName DungeonEditorAppName = FName(TEXT("DungeonEditorApp"));
TSharedPtr<FDungeonEditorThumbnailPool> FDungeonEditorThumbnailPool::Instance;


//////////////////////////////////////////////////////////////////////////
///
const FName FDungeonArchitectThemeEditor::AppModeID_GraphEditor(TEXT("GraphEditor"));
const FName FDungeonArchitectThemeEditor::AppModeID_MarkerGenerator(TEXT("MarkerGenerator"));

void FDungeonArchitectThemeEditor::InitDungeonEditor(const EToolkitMode::Type Mode,
                                                     const TSharedPtr<class IToolkitHost>& InitToolkitHost,
                                                     UDungeonThemeAsset* ThemeAsset) {
    // Initialize the asset editor and spawn nothing (dummy layout)
    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(ThemeAsset, this);
    AssetBeingEdited = ThemeAsset;

    if (ThemeAsset && !ThemeAsset->IsLatestVersion()) {
        TArray<FDungeonThemeGraphBuildError> CompileErrors;
        FDungeonArchitectThemeEditorUtils::CompileThemeGraph(ThemeAsset, CompileErrors);
        ThemeAsset->Modify();
    }

    InitPreviewProperties();
    FDungeonArchitectThemeEditorUtils::InitializeThemeAsset(AssetBeingEdited);

    BindCommands();
    ExtendMenu();
    ToolbarBuilder = MakeShareable(new FThemeEditorToolbar(SharedThis(this)));

    PreviewViewport = SNew(SThemePreviewViewport)
        .DungeonEditor(SharedThis(this))
        .ObjectToEdit(AssetBeingEdited);

    constexpr bool bCreateDefaultStandaloneMenu = true;
    constexpr bool bCreateDefaultToolbar = true;
    const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
    InitAssetEditor(Mode, InitToolkitHost, DungeonEditorAppName, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ThemeAsset);

    PreviewViewport->InitializeCamera();
    
    AppModes.Add(AppModeID_GraphEditor, MakeShareable(new FThemeGraphAppMode(SharedThis(this), PreviewViewport)));
    AppModes.Add(AppModeID_MarkerGenerator, MakeShareable(new FMarkerGeneratorAppMode(SharedThis(this), PreviewViewport)));

    TSubclassOf<UDungeonBuilder> ActiveDungeonBuilderClass = UDungeonBuilder::DefaultBuilderClass();
    if (AssetBeingEdited && AssetBeingEdited->PreviewViewportProperties) {
        ActiveDungeonBuilderClass = AssetBeingEdited->PreviewViewportProperties->BuilderClass;
    }

    for (const auto& Entry : AppModes) {
        TSharedPtr<FThemeEditorAppModeBase> AppMode = Entry.Value;
        AppMode->Init(ActiveDungeonBuilderClass);
        AddApplicationMode(Entry.Key, AppMode.ToSharedRef());        
    }

    SetCurrentMode(AppModeID_GraphEditor);
}

bool FDungeonArchitectThemeEditor::CanAccessModeThemeGraph() const {
    return true;
}

bool FDungeonArchitectThemeEditor::CanAccessModeMarkerGenerator() const {
    return true;
}

FName FDungeonArchitectThemeEditor::GetToolkitFName() const {
    return FName("DungeonEditor");
}

FText FDungeonArchitectThemeEditor::GetBaseToolkitName() const {
    return LOCTEXT("DungeonEditorAppLabel", "Dungeon Editor");
}

FText FDungeonArchitectThemeEditor::GetToolkitName() const {
    if (!AssetBeingEdited) {
        return FWorkflowCentricApplication::GetToolkitName();
    }
    
    return FText::FromString(AssetBeingEdited->GetName());
}

FString FDungeonArchitectThemeEditor::GetWorldCentricTabPrefix() const {
    return TEXT("DungeonEditor");
}

FString FDungeonArchitectThemeEditor::GetDocumentationLink() const {
    return TEXT("Dungeon/DungeonEditor");
}

FLinearColor FDungeonArchitectThemeEditor::GetWorldCentricTabColorScale() const {
    return FLinearColor::White;
}

bool FDungeonArchitectThemeEditor::IsTickableInEditor() const {
    return true;
}

void FDungeonArchitectThemeEditor::Tick(float DeltaTime) {
    const TSharedPtr<FThemeEditorAppModeBase> CurrentMode = StaticCastSharedPtr<FThemeEditorAppModeBase>(GetCurrentModePtr());
    if (CurrentMode.IsValid()) {
        CurrentMode->Tick(DeltaTime);
    }
}

bool FDungeonArchitectThemeEditor::IsTickable() const {
    return true;
}

TStatId FDungeonArchitectThemeEditor::GetStatId() const {
    return TStatId();
}

void FDungeonArchitectThemeEditor::BindCommands() const {
    const FDungeonArchitectCommands& Commands = FDungeonArchitectCommands::Get();

    ToolkitCommands->MapAction(
        Commands.OpenDocumentationWebsite,
        FExecuteAction::CreateStatic(&FDungeonEditorUtils::HandleOpenURL, FDungeonEditorConstants::URL_DOCS));

    ToolkitCommands->MapAction(
            Commands.OpenSupportForums,
            FExecuteAction::CreateStatic(&FDungeonEditorUtils::HandleOpenURL, FDungeonEditorConstants::URL_FORUMS));
    
    ToolkitCommands->MapAction(
        Commands.OpenSupportDiscord,
        FExecuteAction::CreateStatic(&FDungeonEditorUtils::HandleOpenURL, FDungeonEditorConstants::URL_DISCORD));
}

void FDungeonArchitectThemeEditor::NotifyBuilderClassChanged(TSubclassOf<UDungeonBuilder> InBuilderClass) {
    for (const auto& Entry : AppModes) {
        Entry.Value->SetBuilderClass(InBuilderClass);        
    }
}

void FDungeonArchitectThemeEditor::SaveAsset_Execute() {
    UE_LOG(DungeonEditor, Log, TEXT("Saving dungeon theme %s"), *GetEditingObjects()[0]->GetName());
    TArray<FDungeonThemeGraphBuildError> CompileErrors;
    FDungeonArchitectThemeEditorUtils::CompileThemeGraph(AssetBeingEdited, CompileErrors);

    FViewport* PreviewViewportPtr = nullptr;
    if (PreviewViewport.IsValid() && PreviewViewport->GetViewportClient().IsValid()) {
        PreviewViewportPtr = PreviewViewport->GetViewportClient()->Viewport;

        // Save the camera location of the preview viewport
        if (IsValid(AssetBeingEdited) && IsValid(AssetBeingEdited->PreviewViewportProperties)) {
            FViewportCameraTransform ViewTransform = PreviewViewport->GetViewportClient()->GetViewTransform();
            
            FTransform LastCamTransform = FTransform(
                PreviewViewport->GetViewportClient()->GetViewRotation(),
                PreviewViewport->GetViewportClient()->GetViewLocation());

            AssetBeingEdited->PreviewViewportProperties->bViewTransformValid = true;
            AssetBeingEdited->PreviewViewportProperties->ViewTransform.Location = PreviewViewport->GetViewportClient()->GetViewLocation();
            AssetBeingEdited->PreviewViewportProperties->ViewTransform.Rotation = PreviewViewport->GetViewportClient()->GetViewRotation();
            AssetBeingEdited->PreviewViewportProperties->ViewTransform.LookAt = PreviewViewport->GetViewportClient()->GetLookAtLocation();
            AssetBeingEdited->PreviewViewportProperties->Modify();
        }
    }

    FDungeonArchitectThemeEditorUtils::SaveThemeAsset(AssetBeingEdited, PreviewViewportPtr);
    
    if (PreviewViewport.IsValid()) {
        PreviewViewport->RebuildDungeon({});
    }
}

FDungeonArchitectThemeEditor::~FDungeonArchitectThemeEditor() {
    
}

void FDungeonArchitectThemeEditor::ExtendMenu() {

}

void FDungeonArchitectThemeEditor::InitPreviewProperties() {
    if (AssetBeingEdited && AssetBeingEdited->PreviewViewportProperties) {
        if (!AssetBeingEdited->PreviewViewportProperties->DungeonConfig) {
            // Recreate the dungeon config
            //RecreatePreviewDungeonBuilder();
        }
    }
}


FText FDungeonArchitectThemeEditor::GetLocalizedMode(FName InMode) {
    static TMap<FName, FText> LocModes;

    if (LocModes.Num() == 0) {
        LocModes.Add(AppModeID_GraphEditor, LOCTEXT("ThemeGraphModeLabel", "Theme Graph"));
        LocModes.Add(AppModeID_MarkerGenerator, LOCTEXT("MarkerGeneratorModeLabel", "Pattern Matcher"));
    }

    check(InMode != NAME_None);
    const FText* OutDesc = LocModes.Find(InMode);
    check(OutDesc);
    return *OutDesc;
}

void FDungeonArchitectThemeEditor::RegisterToolbarTab(const TSharedRef<FTabManager>& InTabManager) {
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

//////////////////////////////////////// FThemeEditorToolbar ////////////////////////////////////////  
void FThemeEditorToolbar::AddModesToolbar(TSharedPtr<FExtender> Extender) const {
    const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
    if (ThemeEditor.IsValid()) {
        Extender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        ThemeEditor->GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateSP(this, &FThemeEditorToolbar::FillModesToolbar));
    }
}

void FThemeEditorToolbar::AddThemeGraphToolbar(TSharedPtr<FExtender> Extender) {
    const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
    if (ThemeEditor.IsValid()) {
        Extender->AddToolBarExtension("Asset",
            EExtensionHook::After,
            ThemeEditor->GetToolkitCommands(),
            FToolBarExtensionDelegate::CreateSP(this, &FThemeEditorToolbar::FillThemeGraphToolbar));
    }
}

void FThemeEditorToolbar::AddMarkerGeneratorToolbar(TSharedPtr<FExtender> Extender) {
    const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
    if (ThemeEditor.IsValid()) {
        Extender->AddToolBarExtension("Asset",
            EExtensionHook::After,
            ThemeEditor->GetToolkitCommands(),
            FToolBarExtensionDelegate::CreateSP(this, &FThemeEditorToolbar::FillMarkerGeneratorToolbar));
    }
}


void FThemeEditorToolbar::FillModesToolbar(FToolBarBuilder& ToolbarBuilder) const {
    ToolbarBuilder.BeginSection("Support");
    {
        ToolbarBuilder.AddToolBarButton(FDungeonArchitectCommands::Get().OpenDocumentationWebsite);
        ToolbarBuilder.AddToolBarButton(FDungeonArchitectCommands::Get().OpenSupportForums);
        ToolbarBuilder.AddToolBarButton(FDungeonArchitectCommands::Get().OpenSupportDiscord);
    }
    ToolbarBuilder.EndSection();
    
    const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
    if (!ThemeEditor.IsValid()) return;

    const TAttribute<FName> GetActiveMode(ThemeEditor.ToSharedRef(), &FDungeonArchitectThemeEditor::GetCurrentMode);
    const FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(ThemeEditor.ToSharedRef(), &FDungeonArchitectThemeEditor::SetCurrentMode);
    
    // Left side padding
    ThemeEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

    // Theme Graph Mode
    ThemeEditor->AddToolbarWidget(
        SNew(SModeWidget, FDungeonArchitectThemeEditor::GetLocalizedMode(FDungeonArchitectThemeEditor::AppModeID_GraphEditor),
                FDungeonArchitectThemeEditor::AppModeID_GraphEditor)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(ThemeEditor.Get(), &FDungeonArchitectThemeEditor::CanAccessModeThemeGraph)
		.ToolTipText(LOCTEXT("ModeButtonTooltip_ThemeGraph", "Switch to Theme Graph Mode"))
		.IconImage(FDungeonArchitectStyle::Get().GetBrush("DA.SnapEd.SwitchToDesignMode"))
    );

    ThemeEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

    // Pattern Matcher Mode
    ThemeEditor->AddToolbarWidget(
        SNew(SModeWidget, FDungeonArchitectThemeEditor::GetLocalizedMode(FDungeonArchitectThemeEditor::AppModeID_MarkerGenerator),
                FDungeonArchitectThemeEditor::AppModeID_MarkerGenerator)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(ThemeEditor.Get(), &FDungeonArchitectThemeEditor::CanAccessModeMarkerGenerator)
		.ToolTipText(LOCTEXT("ModeButtonTooltip_MarkerGenerator", "Switch to Marker generation mode"))
		.IconImage(FDungeonArchitectStyle::Get().GetBrush("DA.SnapEd.SwitchToVisualizeMode"))
    );

    // Right side padding
    ThemeEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
}

void FThemeEditorToolbar::FillThemeGraphToolbar(FToolBarBuilder& ToolbarBuilder) {
    const FThemeGraphAppModeCommands& Commands = FThemeGraphAppModeCommands::Get();
    ToolbarBuilder.AddToolBarButton(Commands.Build);
}

void FThemeEditorToolbar::FillMarkerGeneratorToolbar(FToolBarBuilder& ToolbarBuilder) {
    const FMarkerGeneratorAppModeCommands& Commands = FMarkerGeneratorAppModeCommands::Get();
    ToolbarBuilder.AddToolBarButton(Commands.Build);
}


#undef LOCTEXT_NAMESPACE

