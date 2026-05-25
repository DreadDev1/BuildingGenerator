//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/CanvasEditor/DungeonCanvasMaterialThemeEditor.h"

#include "Builders/CellFlow/CellFlowConfig.h"
#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/GridFlow/GridFlowConfig.h"
#include "Builders/SnapGridFlow/SnapGridFlowDungeon.h"
#include "Builders/SnapMap/SnapMapDungeonConfig.h"
#include "Core/Dungeon.h"
#include "Core/DungeonConfig.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorCommands.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorSettings.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorToolbar.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorUtilities.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorViewport.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialThemeEditorProperties.h"
#include "Frameworks/Canvas/UI/Viewport/SDungeonCanvasViewport.h"

#include "BlueprintCompilationManager.h"
#include "Framework/Commands/GenericCommands.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialFunctionMaterialLayerBlend.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/MessageDialog.h"
#include "Slate/SceneViewport.h"
#include "UObject/SoftObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"

DEFINE_LOG_CATEGORY_STATIC(LogCanvasEditor, Log, All);

#define LOCTEXT_NAMESPACE "DungeonCanvasEditor"

struct FDungeonCanvasMaterialThemeEditorTabs {
	// Tab identifiers
	static const FName MaterialLayersID;
	static const FName PreviewID;
	static const FName DetailsID;
};
const FName FDungeonCanvasMaterialThemeEditorTabs::MaterialLayersID(TEXT("MaterialLayers"));
const FName FDungeonCanvasMaterialThemeEditorTabs::PreviewID(TEXT("Preview3D"));
const FName FDungeonCanvasMaterialThemeEditorTabs::DetailsID(TEXT("Details"));

namespace DA::DungeonCanvas::Private
{
	const FName DungeonCanvasEditorAppName(TEXT("DungeonCanvasEditorApp"));

}


FDungeonCanvasMaterialThemeEditor::FDungeonCanvasMaterialThemeEditor()
	: Sampling()
{
}

void FDungeonCanvasMaterialThemeEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDungeonCanvasMaterialTheme* InThemeAsset) {
	AssetBeingEdited = InThemeAsset;
	FBlueprintCompilationManager::FlushCompilationQueue(nullptr);

	TSharedPtr<FDungeonCanvasMaterialThemeEditor> ThisPtr(SharedThis(this));
	
	const UDungeonCanvasEditorSettings& Settings = *GetDefault<UDungeonCanvasEditorSettings>();
	Sampling = Settings.Sampling;

	FDungeonCanvasDrawSettings DrawSettings;
	DrawSettings.BackgroundColor = FLinearColor::Transparent;
	DrawSettings.TextureBorderEnabled = true;
	DrawSettings.bDrawTransparentBackgroundCheckerboardTexture = true;
	DrawSettings.bFogOfWarEnabled = false;
	
	CanvasViewport = SNew(SDungeonCanvasEditorViewport, SharedThis(this))
		.DrawSettings(DrawSettings);
	CanvasViewport->SetDungeonCanvasTheme(AssetBeingEdited);
	
	BindCommands();

	// Create the details panel widget
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.NotifyHook = this;
		
		DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		DetailsPanel->SetObject(AssetBeingEdited);
	}

	// Create the preview scene to host the dungeon canvas actor
	{
		FPreviewScene::ConstructionValues CVS;
		CVS.bCreatePhysicsScene = false;
		CVS.bDefaultLighting = false;
		PreviewScene = MakeUnique<FPreviewScene>(CVS);
	}
	
	const FSoftObjectPath MaterialLayerPresetsPath(TEXT("/DungeonArchitect/Core/Editors/CanvasEditor/Data/DungeonCanvasEditorMaterialLayerPresets.DungeonCanvasEditorMaterialLayerPresets"));
	EditorDefaultSettings = Cast<UDungeonCanvasEditorDefaults>(MaterialLayerPresetsPath.TryLoad());

	MaterialLayerList = SNew(SEditableListView<TObjectPtr<UDungeonCanvasMaterialLayer>>)
		.GetListSource(this, &FDungeonCanvasMaterialThemeEditor::GetMaterialLayerList)
		.OnSelectionChanged(this, &FDungeonCanvasMaterialThemeEditor::OnMaterialLayerSelectionChanged)
		.OnDeleteItem(this, &FDungeonCanvasMaterialThemeEditor::OnMaterialLayerDelete)
		.OnGetAddButtonMenuContent(this, &FDungeonCanvasMaterialThemeEditor::GetAddMaterialLayerMenu)
		.OnReorderItem(this, &FDungeonCanvasMaterialThemeEditor::OnMaterialLayerReordered)
		.GetItemText(this, &FDungeonCanvasMaterialThemeEditor::GetMaterialLayerListRowText)
		.CreateItemWidget(this, &FDungeonCanvasMaterialThemeEditor::CreateMaterialLayerListItemWidget)
		.IconBrush(FDungeonArchitectStyle::Get().GetBrush("DA.SnapEd.GraphIcon"))
		.AllowDropOnGraph(false)
	;

    // Default layout
    const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout(
            "Standalone_DungeonCanvasMaterialThemeEditor_Layout_v0.0.3")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Horizontal)
                ->SetSizeCoefficient(1.0f)
                ->Split
                (

                FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.3f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(FDungeonCanvasMaterialThemeEditorTabs::MaterialLayersID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(FDungeonCanvasMaterialThemeEditorTabs::DetailsID, ETabState::OpenedTab)
					)
                )

                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.7f)
                    ->SetHideTabWell(true)
                    ->AddTab(FDungeonCanvasMaterialThemeEditorTabs::PreviewID, ETabState::OpenedTab)
                )
            )
        );
	
	// Initialize the asset editor and spawn tabs
	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, DA::DungeonCanvas::Private::DungeonCanvasEditorAppName,
			StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, AssetBeingEdited);

	DungeonCanvasToolbar = MakeShareable(new FDungeonCanvasBlueprintEditorToolbar());
	
	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
	CreateDefaultCommands();

	if (AssetBeingEdited) {
		if (AssetBeingEdited->PreviewDungeonProperties) {
			AssetBeingEdited->PreviewDungeonProperties->PropertyChangeListener = SharedThis(this);
		}

		AssetBeingEdited->MaterialLayers.RemoveAll([](const UDungeonCanvasMaterialLayer* Item) {
			return Item == nullptr;
		});
	}

	UpdateInstance();
	MaterialLayerList->SetItemSelection(nullptr, ESelectInfo::Direct);

	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
	Compile();
}

void FDungeonCanvasMaterialThemeEditor::CreateDefaultCommands() {
	ToolkitCommands->MapAction(FGenericCommands::Get().Undo, FExecuteAction::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::UndoAction));
	ToolkitCommands->MapAction(FGenericCommands::Get().Redo, FExecuteAction::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::RedoAction));
}


void FDungeonCanvasMaterialThemeEditor::Compile() {
	FDungeonCanvasEditorUtilities::CompileDungeonCanvasMaterialTemplate(AssetBeingEdited);
	FlushRenderingCommands();

	if (AssetBeingEdited && AssetBeingEdited->CompiledThemeMaterial) {
		UMaterialEditingLibrary::UpdateMaterialInstance(AssetBeingEdited->CompiledThemeMaterial);
		FlushRenderingCommands();
	}
	
	// Restore the cached layer values on to the compiled material constant
	FDungeonCanvasEditorUtilities::RestoreMaterialLayerParameters(AssetBeingEdited);
	
	if (CanvasInstance.IsValid()) {
		CanvasInstance->Initialize(PreviewDungeon.Get());
	}
}

void FDungeonCanvasMaterialThemeEditor::SaveAsset_Execute() {
	UpdateAssetThumbnail();
	
	Super::SaveAsset_Execute();
}


FName FDungeonCanvasMaterialThemeEditor::GetToolkitFName() const {
	return FName("DungeonCanvasMaterialThemeEditor");
}

FText FDungeonCanvasMaterialThemeEditor::GetBaseToolkitName() const {
	return LOCTEXT("AppLabel", "Dungeon Canvas Material Theme Editor");
}

FString FDungeonCanvasMaterialThemeEditor::GetDocumentationLink() const {
	return FString();
}

FText FDungeonCanvasMaterialThemeEditor::GetToolkitToolTipText() const {
	static const FText Tooltip = LOCTEXT("DungeonCanvasMaterialEditorToolTip", "Dungeon Canvas Material Editor Tool");
	return Tooltip;
}

FLinearColor FDungeonCanvasMaterialThemeEditor::GetWorldCentricTabColorScale() const {
	return FLinearColor(0.5f, 0.25f, 0.35f, 0.5f);
}

FString FDungeonCanvasMaterialThemeEditor::GetWorldCentricTabPrefix() const {
	return LOCTEXT("WorldCentricTabPrefix", "Dungeon Canvas ").ToString();
}

void FDungeonCanvasMaterialThemeEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) {
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
        LOCTEXT("WorkspaceMenu_DungeonCanvasThemeEditor", "Dungeon Canvas Material Theme Editor"));
    auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(FDungeonCanvasMaterialThemeEditorTabs::MaterialLayersID,
                FOnSpawnTab::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::SpawnTab_MaterialLayers))
                .SetDisplayName(LOCTEXT("MaterialLayersTab", "Material Layers"))
                .SetGroup(WorkspaceMenuCategoryRef)
                .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

    InTabManager->RegisterTabSpawner(FDungeonCanvasMaterialThemeEditorTabs::PreviewID,
                FOnSpawnTab::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::SpawnTab_Preview3D))
                .SetDisplayName(LOCTEXT("PreviewTab", "Preview"))
                .SetGroup(WorkspaceMenuCategoryRef)
                .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
    
    InTabManager->RegisterTabSpawner(FDungeonCanvasMaterialThemeEditorTabs::DetailsID,
                FOnSpawnTab::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::SpawnTab_Details))
                .SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
                .SetGroup(WorkspaceMenuCategoryRef)
                .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

}

void FDungeonCanvasMaterialThemeEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) {
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FDungeonCanvasMaterialThemeEditorTabs::PreviewID);
	InTabManager->UnregisterTabSpawner(FDungeonCanvasMaterialThemeEditorTabs::DetailsID);
	InTabManager->UnregisterTabSpawner(FDungeonCanvasMaterialThemeEditorTabs::MaterialLayersID);
}

void FDungeonCanvasMaterialThemeEditor::Tick(float DeltaTime) {
	// Disable the graph (tick, init etc) when it's simulating. This happens when PIE is enabled
	if (!CanvasInstance.IsValid()) {
		UpdateInstance();
	}
	
	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread) {
		if (PreviewScene.IsValid()) {
			PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaTime);
		}
	}

	if (bRecompileNextFrame) {
		bRecompileNextFrame = false;
		Compile();
	}
}

TStatId FDungeonCanvasMaterialThemeEditor::GetStatId() const {
	RETURN_QUICK_DECLARE_CYCLE_STAT(FDungeonCanvasEditor, STATGROUP_Tickables);
}

void FDungeonCanvasMaterialThemeEditor::RecreateDungeonBuilder(UDungeonEditorViewportProperties* InViewportProperties) {
	if (!InViewportProperties) {
		return;
	}
	if (!PreviewDungeon->GetBuilder() || PreviewDungeon->GetBuilder()->GetClass() != InViewportProperties->BuilderClass) {
		PreviewDungeon->SetBuilderClass(InViewportProperties->BuilderClass);
	}
	
	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
	check(PreviewDungeon->GetBuilder() && PreviewDungeon->GetConfig());
	if (!InViewportProperties->DungeonConfig || InViewportProperties->DungeonConfig->GetClass() != PreviewDungeon->GetConfig()->GetClass()) {
		InViewportProperties->DungeonConfig = NewObject<UDungeonConfig>(InViewportProperties, PreviewDungeon->GetConfig()->GetClass());
		InViewportProperties->DungeonConfig->ConfigPropertyChanged.Unbind();
		InViewportProperties->DungeonConfig->ConfigPropertyChanged.BindUObject(InViewportProperties, &UDungeonEditorViewportProperties::PostEditChangeConfigProperty);
	}

	// Set the default dungeon config settings, so we can build it directly without manual intervention
	if (EditorDefaultSettings.IsValid()) {
		if (UGridFlowConfig* GridFlowConfig = Cast<UGridFlowConfig>(InViewportProperties->DungeonConfig)) {
			GridFlowConfig->GridFlow = EditorDefaultSettings->BuilderDefaultAssets.GridFlowGraph;
		}
		else if (UCellFlowConfig* CellFlowConfig = Cast<UCellFlowConfig>(InViewportProperties->DungeonConfig)) {
			CellFlowConfig->CellFlow = EditorDefaultSettings->BuilderDefaultAssets.CellFlowGraph;
		}
		else if (USnapGridFlowConfig* SGFConfig = Cast<USnapGridFlowConfig>(InViewportProperties->DungeonConfig)) {
			SGFConfig->FlowGraph = EditorDefaultSettings->BuilderDefaultAssets.SgfGraph;
			SGFConfig->ModuleDatabase = EditorDefaultSettings->BuilderDefaultAssets.SgfModDB;
		}
		else if (USnapMapDungeonConfig* SnapMapConfig = Cast<USnapMapDungeonConfig>(InViewportProperties->DungeonConfig)) {
			SnapMapConfig->DungeonFlowGraph = EditorDefaultSettings->BuilderDefaultAssets.SnapMapGraph;
			SnapMapConfig->ModuleDatabase = EditorDefaultSettings->BuilderDefaultAssets.SnapMapModDB;
		}
	}
	bRecompileNextFrame = true;
}

void FDungeonCanvasMaterialThemeEditor::ShowDetailsForSingleObject(UObject* Object, bool bForceRefresh) const {
	if (DetailsPanel) {
		DetailsPanel->SetObject(Object, bForceRefresh);
	}
}

ESimpleElementBlendMode FDungeonCanvasMaterialThemeEditor::GetColourChannelBlendMode() const {
	// Add the red, green, blue, alpha and desaturation flags to the enum to identify the chosen filters
	uint32 Result = (uint32)SE_BLEND_RGBA_MASK_START;
	Result += 15;
	
	return (ESimpleElementBlendMode)Result;
}

EDungeonCanvasEditorSampling FDungeonCanvasMaterialThemeEditor::GetSampling() const {
	return Sampling;
}

const TArray<TObjectPtr<UDungeonCanvasMaterialLayer>>* FDungeonCanvasMaterialThemeEditor::GetMaterialLayerList() const {
	return AssetBeingEdited ? &AssetBeingEdited->MaterialLayers : nullptr;
}

void FDungeonCanvasMaterialThemeEditor::OnMaterialLayerSelectionChanged(TObjectPtr<UDungeonCanvasMaterialLayer> Item, ESelectInfo::Type SelectInfo) {
	ActiveMaterialLayer = Item;

	if (SelectInfo != ESelectInfo::Direct) {
		ShowDetailsForSingleObject(Item);
	}
}

void FDungeonCanvasMaterialThemeEditor::AddNewMaterialLayer(const FText& InLayerName, const FText& InLayerDescription, const TSoftObjectPtr<UMaterialFunctionMaterialLayer>& InMaterialLayer, const TSoftObjectPtr<UMaterialFunctionMaterialLayerBlend>& InMaterialBlend) {
	static const FText DefaultLayerName = LOCTEXT("DefaultLayerName", "Effect Layer");

	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
	
	if (AssetBeingEdited) {
		UDungeonCanvasMaterialLayer* NewLayer = NewObject<UDungeonCanvasMaterialLayer>(AssetBeingEdited);
		NewLayer->LayerName = InLayerName;
		NewLayer->LayerDescription = InLayerDescription;
		NewLayer->MaterialLayer = InMaterialLayer;
		if (InMaterialBlend.IsValid()) {
			NewLayer->MaterialBlend = InMaterialBlend;
		}
		else if (EditorDefaultSettings.IsValid()) {
			NewLayer->MaterialBlend = EditorDefaultSettings->DefaultLayerBlend;
		}
		
		AssetBeingEdited->MaterialLayers.Insert(NewLayer, 0);
		AssetBeingEdited->Modify();
		
		MaterialLayerList->RefreshListView();
		MaterialLayerList->SetItemSelection(NewLayer, ESelectInfo::Direct);
		Compile();
		
		ShowDetailsForSingleObject(NewLayer, true);
	}
}

void FDungeonCanvasMaterialThemeEditor::OnMaterialLayerDelete(TObjectPtr<UDungeonCanvasMaterialLayer> Item) {
	if (!Item) return;
	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
	
	static const FText Title = LOCTEXT("DADeleteCanvasMatLayerTitle", "Delete Layer?");
	const EAppReturnType::Type ReturnValue = FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::No,
															LOCTEXT("DADeleteCanvasMatLayer","Are you sure you want to delete the layer?"),
															Title);

	if (ReturnValue == EAppReturnType::Yes) {
		// Delete the layer
		if (AssetBeingEdited) {
			AssetBeingEdited->MaterialLayers.Remove(Item);
			AssetBeingEdited->Modify();

			ActiveMaterialLayer = nullptr;
			bRecompileNextFrame = true;
		}
	}
}

void FDungeonCanvasMaterialThemeEditor::OnMaterialLayerReordered(TObjectPtr<UDungeonCanvasMaterialLayer> Source, TObjectPtr<UDungeonCanvasMaterialLayer> Dest) {
	if (AssetBeingEdited) {
		FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
		int32 SourceIndex = INDEX_NONE;
		if (AssetBeingEdited->MaterialLayers.Find(Source, SourceIndex)) {
			AssetBeingEdited->MaterialLayers.Remove(Source);
			
			int32 DestIndex = INDEX_NONE;
			if (!AssetBeingEdited->MaterialLayers.Find(Dest, DestIndex)) {
				DestIndex = 0;
			}
			AssetBeingEdited->MaterialLayers.Insert(Source, DestIndex);
			bRecompileNextFrame = true;
		}
	}
}

FText FDungeonCanvasMaterialThemeEditor::GetMaterialLayerListRowText(TObjectPtr<UDungeonCanvasMaterialLayer> Item) const {
	static const FText UnknownText = LOCTEXT("MissingLayerName", "Unknown");
	return Item ? Item->LayerName : UnknownText;
}

FText FDungeonCanvasMaterialThemeEditor::GetLayerListRowDescriptionText(TObjectPtr<UDungeonCanvasMaterialLayer> Item) const {
	static const FText UnknownText = LOCTEXT("MissingLayerDescName", "Unknown");
	return Item ? Item->LayerDescription : UnknownText;
}

EVisibility FDungeonCanvasMaterialThemeEditor::IsLayerListRowDescriptionVisible(TObjectPtr<UDungeonCanvasMaterialLayer> Item) const {
	return Item && !Item->LayerDescription.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedPtr<SWidget> FDungeonCanvasMaterialThemeEditor::CreateMaterialLayerListItemWidget(TObjectPtr<UDungeonCanvasMaterialLayer> InMaterialLayer) {
	return
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &FDungeonCanvasMaterialThemeEditor::GetMaterialLayerListRowText, InMaterialLayer)
				.ColorAndOpacity_Lambda([InMaterialLayer]() {
					return InMaterialLayer && InMaterialLayer->bEnabled ? FLinearColor::White : FLinearColor::Gray;
				})
				.Font(FDungeonArchitectStyle::Get().GetFontStyle("DungeonArchitect.ListView.LargeFont"))
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &FDungeonCanvasMaterialThemeEditor::GetLayerListRowDescriptionText, InMaterialLayer)
				.Visibility(this, &FDungeonCanvasMaterialThemeEditor::IsLayerListRowDescriptionVisible, InMaterialLayer)
				.ColorAndOpacity(FLinearColor::Gray)
				.Font(FDungeonArchitectStyle::Get().GetFontStyle("DungeonArchitect.ListView.Font"))
			]
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([InMaterialLayer]() {
				return InMaterialLayer && InMaterialLayer->bEnabled
					? ECheckBoxState::Checked
					: ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, InMaterialLayer](ECheckBoxState NewCheckboxState) {
				if (InMaterialLayer) {
					FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
					InMaterialLayer->bEnabled = (NewCheckboxState == ECheckBoxState::Checked);
					bRecompileNextFrame = true;
				}
			})
			.ToolTipText(LOCTEXT("LayerVisiblityTooltip", "Enable / Disable the material layer"))
			.CheckedImage(FAppStyle::Get().GetBrush("Icons.Visible"))
			.CheckedHoveredImage(FAppStyle::Get().GetBrush("Icons.Visible"))
			.CheckedPressedImage(FAppStyle::Get().GetBrush("Icons.Visible"))
			.UncheckedImage(FAppStyle::Get().GetBrush("Icons.Hidden"))
			.UncheckedHoveredImage(FAppStyle::Get().GetBrush("Icons.Hidden"))
			.UncheckedPressedImage(FAppStyle::Get().GetBrush("Icons.Hidden"))
		];
	
}

void FDungeonCanvasMaterialThemeEditor::UpdateAssetThumbnail() const {
	TSharedPtr<FViewport> PreviewViewport;
	if (CanvasViewport.IsValid()) {
		PreviewViewport = CanvasViewport->GetViewport();
	}

	if (PreviewViewport.IsValid()) {
		const FAssetData AssetData(AssetBeingEdited);
		TArray<FAssetData> ThemeAssetList;
		ThemeAssetList.Add(AssetData);

		FDungeonEditorUtils::CaptureThumbnailFromViewport(PreviewViewport.Get(), ThemeAssetList, [](const FColor& InColor) {
			FLinearColor LinearColor = InColor.ReinterpretAsLinear();
			return LinearColor.ToFColor(true);
		});
	}
}

TSharedRef<SDockTab> FDungeonCanvasMaterialThemeEditor::SpawnTab_MaterialLayers(const FSpawnTabArgs& SpawnTabArgs) {
	return SNew(SDockTab)
		.Label(LOCTEXT("MaterialLayerTab_Title", "Material Layers"))
		[
			MaterialLayerList.ToSharedRef()
		];
}

TSharedRef<SDockTab> FDungeonCanvasMaterialThemeEditor::SpawnTab_Preview3D(const FSpawnTabArgs& SpawnTabArgs) {
	return SNew(SDockTab)
		.Label(LOCTEXT("PreviewViewportTab_Title", "Preview Viewport"))
		[
			CanvasViewport.ToSharedRef()
		];
}

TSharedRef<SDockTab> FDungeonCanvasMaterialThemeEditor::SpawnTab_Details(const FSpawnTabArgs& SpawnTabArgs) {
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		[
			DetailsPanel.ToSharedRef()
		];
}

TSharedRef<SWidget> FDungeonCanvasMaterialThemeEditor::GetAddMaterialLayerMenu() {
	FMenuBuilder MenuBuilder(true, nullptr);
	if (EditorDefaultSettings.IsValid()) {
		MenuBuilder.BeginSection("MaterialLayersPresets", LOCTEXT("MaterialLayerPresetSection", "Material Layer Presets"));
		for (const FDungeonCanvasEditorMaterialLayerPreset& Preset : EditorDefaultSettings->MaterialLayerPresets) {
			MenuBuilder.AddMenuEntry(
				Preset.Title,
				Preset.Tooltip,
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, Preset]() {
					AddNewMaterialLayer(Preset.Title, Preset.Tooltip, Preset.MaterialLayer, Preset.MaterialBlend);
				})));
		}
		MenuBuilder.EndSection();
	}

	MenuBuilder.BeginSection("MaterialLayerCustom", LOCTEXT("MaterialLayerCustomSection", "Advanced"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("LayerNameBackground", "Custom Material Layer"),
			LOCTEXT("LayerNameBackgroundTooltip", "Add your own material layer and blend functions"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				AddNewMaterialLayer(LOCTEXT("CustomLayerTitle", "Custom Layer"), LOCTEXT("CustomLayerTitle", "Custom Layer for user defined material functions"), nullptr, nullptr);
			})));
	}
	MenuBuilder.EndSection();


	return MenuBuilder.MakeWidget();
}

void FDungeonCanvasMaterialThemeEditor::RandomizePreviewDungeon() {
	UE_LOG(LogCanvasEditor, Log, TEXT("Randomize dungeon requested"));
	// Execute the blueprint event
	if (IsValid(CanvasInstance.Get()) && PreviewDungeon.IsValid()) {
		if (AssetBeingEdited && AssetBeingEdited->PreviewDungeonProperties && AssetBeingEdited->PreviewDungeonProperties->DungeonConfig) {
			AssetBeingEdited->PreviewDungeonProperties->DungeonConfig->Seed = FMath::Rand();
		}
		BuildPreviewDungeon();
		CanvasInstance->Initialize(PreviewDungeon.Get());
	}
	
}

void FDungeonCanvasMaterialThemeEditor::AddReferencedObjects(FReferenceCollector& Collector) {
	Collector.AddReferencedObject(AssetBeingEdited);
	CanvasViewport->AddReferencedObjects(Collector);
}

FString FDungeonCanvasMaterialThemeEditor::GetReferencerName() const {
	static const FString NameString = TEXT("DungeonCanvasMaterialThemeEditor");
	return NameString;
}

void FDungeonCanvasMaterialThemeEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) {
	const FName PropertyName = PropertyThatChanged ? PropertyThatChanged->GetFName() : NAME_None;
	
	const FString PropertyPath = PropertyThatChanged ? PropertyThatChanged->GetPathName() : "";
	UE_LOG(LogCanvasEditor, Log, TEXT("PostChange Path: %s"), *PropertyPath);
	
	if (const UObject* ObjectBeingEdited = PropertyChangedEvent.GetObjectBeingEdited(0)) {
		if (AssetBeingEdited) {
			if (AssetBeingEdited->PreviewDungeonProperties == ObjectBeingEdited) {
				if (PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonEditorViewportProperties, BuilderClass)) {
					UpdateInstance();
					ShowPreviewDungeonSettings();
				}
				if (PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonCanvasMaterialThemeEditorProperties, LayoutDrawMarginPercent)) {
					if (CanvasInstance.IsValid()) {
						CanvasInstance->LayoutDrawMarginPercent = AssetBeingEdited->PreviewDungeonProperties->LayoutDrawMarginPercent;
						CanvasInstance->Initialize(PreviewDungeon.Get());
					}
				}
			}
		}
	}
}

void FDungeonCanvasMaterialThemeEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged) {
	UE_LOG(LogCanvasEditor, Log, TEXT("Chain prop changed"));

	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);

	const UObject* EditedObject = {};
	if (PropertyChangedEvent.GetNumObjectsBeingEdited() > 0) {
		EditedObject = PropertyChangedEvent.GetObjectBeingEdited(0);	
	}

	auto Node = PropertyThatChanged->GetHead();
	while(Node)
	{
		if (const FProperty* Property = Node->GetValue())
		{
			const FName PropertyName = Property->GetFName();
			if (PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonCanvasMaterialLayer, MaterialLayer)
				|| PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonCanvasMaterialLayer, MaterialBlend))
			{
				UE_LOG(LogCanvasEditor, Log, TEXT("PostChainChange Path: %s"), *Property->GetPathName());
				bRecompileNextFrame = true;
				break;
			}

			if (EditedObject && EditedObject->IsA<UDungeonConfig>()) {
				UpdateInstance();
				bRecompileNextFrame = true;
				break;
			}
			
			if (PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonCanvasMaterialThemeEditorProperties, LayoutDrawMarginPercent)) {
				if (CanvasInstance.IsValid()) {
					CanvasInstance->LayoutDrawMarginPercent = AssetBeingEdited->PreviewDungeonProperties->LayoutDrawMarginPercent;
					CanvasInstance->Initialize(PreviewDungeon.Get());
				}
			}
		}
		
		Node = Node->GetNextNode();
	}
}

void FDungeonCanvasMaterialThemeEditor::OnPropertyChanged(FString PropertyName, UDungeonEditorViewportProperties* Properties) {
	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonEditorViewportProperties, BuilderClass).ToString()) {
		RecreateDungeonBuilder(Properties);
		check(Properties->DungeonConfig);
		ShowDetailsForSingleObject(Properties, true);

		if (DungeonCanvasToolbar.IsValid()) {
			DungeonCanvasToolbar->OnBuilderChanged();
		}
		UpdateInstance();
		bRecompileNextFrame = true;
		Properties->Modify();
	}
}

void FDungeonCanvasMaterialThemeEditor::BindCommands() {
	// Add Commands if needed.  Example below
	const FDungeonCanvasEditorCommands& Commands = FDungeonCanvasEditorCommands::Get();

	ToolkitCommands->MapAction(
		Commands.Compile,
		FExecuteAction::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::CompileTheme_Clicked),
		FCanExecuteAction(),
		FIsActionChecked());

	ToolkitCommands->MapAction(
		Commands.RandomizeDungeon,
		FExecuteAction::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::RandomizePreviewDungeon),
		FCanExecuteAction(),
		FIsActionChecked());
	
	ToolkitCommands->MapAction(
		Commands.EditCanvasSettings,
		FExecuteAction::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::EditThemeSettings_Clicked),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::IsDetailsPanelEditingCanvasThemeSettings));
	
	ToolkitCommands->MapAction(
		Commands.EditDungeonSettings,
		FExecuteAction::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::EditPreviewDungeonSettings_Clicked),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::IsDetailsPanelEditingDungeonPreviewSettings));
}

void FDungeonCanvasMaterialThemeEditor::UndoAction() {
	GEditor->UndoTransaction();
}

void FDungeonCanvasMaterialThemeEditor::RedoAction() {
	GEditor->RedoTransaction();
}

void FDungeonCanvasMaterialThemeEditor::EditPreviewDungeonSettings_Clicked() {
	ShowPreviewDungeonSettings();
}

void FDungeonCanvasMaterialThemeEditor::EditThemeSettings_Clicked() const {
	ShowDetailsForSingleObject(AssetBeingEdited);
}

void FDungeonCanvasMaterialThemeEditor::CompileTheme_Clicked() {
	FDungeonCanvasEditorUtilities::SaveMaterialLayerParameters(AssetBeingEdited);
	Compile();
}

void FDungeonCanvasMaterialThemeEditor::ShowPreviewDungeonSettings() {
	if (AssetBeingEdited) {
		ShowDetailsForSingleObject(AssetBeingEdited->PreviewDungeonProperties);
	}
}

bool FDungeonCanvasMaterialThemeEditor::IsDetailsPanelEditingDungeonPreviewSettings() const {
	if (AssetBeingEdited && AssetBeingEdited->PreviewDungeonProperties) {
		return DetailsPanel->GetSelectedObjects().Contains(AssetBeingEdited->PreviewDungeonProperties);
	}
	return false;
}

bool FDungeonCanvasMaterialThemeEditor::IsDetailsPanelEditingCanvasThemeSettings() const {
	if (AssetBeingEdited) {
		return DetailsPanel->GetSelectedObjects().Contains(AssetBeingEdited);
	}
	return false;
}

void FDungeonCanvasMaterialThemeEditor::ExtendMenu() {
	if (MenuExtender.IsValid())
	{
		RemoveMenuExtender(MenuExtender);
		MenuExtender.Reset();
	}

	MenuExtender = MakeShareable(new FExtender);

	AddMenuExtender(MenuExtender);
}

void FDungeonCanvasMaterialThemeEditor::ExtendToolbar() {
	// If the ToolbarExtender is valid, remove it before rebuilding it
	if (ToolbarExtender.IsValid())
	{
		RemoveToolbarExtender(ToolbarExtender);
		ToolbarExtender.Reset();
	}

	ToolbarExtender = MakeShareable(new FExtender);

	AddToolbarExtender(ToolbarExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::FillToolbar)
	);
}

void FDungeonCanvasMaterialThemeEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder) {
	ToolbarBuilder.BeginSection("Build");
	{
		ToolbarBuilder.AddToolBarButton(FDungeonCanvasEditorCommands::Get().Compile);
		ToolbarBuilder.AddToolBarButton(FDungeonCanvasEditorCommands::Get().RandomizeDungeon);
		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &FDungeonCanvasMaterialThemeEditor::GeneratePreviewBuilderClassDropDown),
			LOCTEXT("RetargetMode_Label", "Select Builder"),
			LOCTEXT("RetargetMode_ToolTip", "Choose the dungeon builder for previewing."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Recompile"));
	}
	ToolbarBuilder.EndSection();
	ToolbarBuilder.BeginSection("Settings");
	{
		ToolbarBuilder.AddToolBarButton(FDungeonCanvasEditorCommands::Get().EditCanvasSettings);
		ToolbarBuilder.AddToolBarButton(FDungeonCanvasEditorCommands::Get().EditDungeonSettings);
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FDungeonCanvasMaterialThemeEditor::GeneratePreviewBuilderClassDropDown() {
	TWeakPtr<FDungeonCanvasMaterialThemeEditor> EditorPtr = SharedThis(this);
	auto FnGetDefaultPreviewBuilder = [EditorPtr]() -> TSubclassOf<UDungeonBuilder> {
		if (EditorPtr.IsValid()) {
			TSharedPtr<FDungeonCanvasMaterialThemeEditor> Editor = EditorPtr.Pin();
			if (Editor.IsValid() && Editor->AssetBeingEdited && Editor->AssetBeingEdited->PreviewDungeonProperties) {
				if (TSubclassOf<UDungeonBuilder> BuilderClass = Editor->AssetBeingEdited->PreviewDungeonProperties->BuilderClass) {
					return BuilderClass;
				}
			}
		}

		// fallback to the grid builder
		return UGridDungeonBuilder::StaticClass();
	};

	auto FnSetBuilder = [EditorPtr](TSubclassOf<UDungeonBuilder> InBuilder) {
		TSharedPtr<FDungeonCanvasMaterialThemeEditor> Editor = EditorPtr.Pin();
		if (Editor.IsValid() && Editor->AssetBeingEdited && Editor->AssetBeingEdited->PreviewDungeonProperties) {
			Editor->AssetBeingEdited->PreviewDungeonProperties->BuilderClass = InBuilder;
			
			FName PropertyName = GET_MEMBER_NAME_CHECKED(UDungeonEditorViewportProperties, BuilderClass);
			Editor->OnPropertyChanged(PropertyName.ToString(), Editor->AssetBeingEdited->PreviewDungeonProperties);
		}
	};
		
	TSharedPtr<SDungeonBuilderClassCombo> BuilderClassCombo = SNew(SDungeonBuilderClassCombo)
		.SelectedBuilderItem(FnGetDefaultPreviewBuilder())
		.OnSetDungeonBuilder_Lambda(FnSetBuilder);

	return SNew(SBorder)
		.Padding(FMargin(1))  // Thin padding for the black border
		.BorderBackgroundColor(FLinearColor::Black)  // Set the border color to black
		.BorderImage(new FSlateColorBrush(FLinearColor::Black))  // Use a solid black brush for the border
		[
			SNew(SBorder)
			.Padding(FMargin(10))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))  // Inner border for contrast
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0, 0, 0, 5))  // Add some bottom padding to the label
				[
					SNew(STextBlock)
					.Text(FText::FromString("Select the preview builder"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				]
	            
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuilderClassCombo.ToSharedRef()
				]
			]
		];
}

void FDungeonCanvasMaterialThemeEditor::BuildPreviewDungeon() {
	if (!PreviewDungeon.IsValid()) {
		return;
	}

	if (AssetBeingEdited) {
		if (UDungeonCanvasMaterialThemeEditorProperties* PreviewViewportProperties = AssetBeingEdited->PreviewDungeonProperties) {
			// Set the builder instance and initialize it
			PreviewDungeon->SetBuilderClass(PreviewViewportProperties->BuilderClass);

			if (!PreviewViewportProperties->DungeonConfig) {
				RecreateDungeonBuilder(PreviewViewportProperties);
			}
			
			// copy over the config from the preview config over to the dungeon actor's config
			UDungeonConfig* DungeonConfig = PreviewDungeon->GetConfig();
			UEngine::CopyPropertiesForUnrelatedObjects(PreviewViewportProperties->DungeonConfig, DungeonConfig);

			if (UDungeonCanvasComponent* DungeonCanvasComponent = PreviewDungeon->GetComponentByClass<UDungeonCanvasComponent>()) {
				DungeonCanvasComponent->LayoutDrawMarginPercent = PreviewViewportProperties->LayoutDrawMarginPercent;
			}
		}
	}

	UDungeonBuilder* DungeonBuilder = PreviewDungeon->GetBuilder();
	
	// Flag the builder as a preview layout only builder. This will not spawn any items on the scene
	FDungeonBuilderParameters BuilderParameters = {};
	BuilderParameters.bLayoutPreviewBuildOnly = true;
	DungeonBuilder->BuildDungeon(PreviewDungeon.Get(), PreviewDungeon->GetWorld(), BuilderParameters);
}

void FDungeonCanvasMaterialThemeEditor::UpdateInstance() {
	if (!PreviewScene) {
		return;
	}
	
	if (UWorld* World = PreviewScene->GetWorld()) {
		// Recreate the preview dungeon actor
		if (PreviewDungeon.IsValid()) {
			PreviewDungeon->DestroyDungeon();
			PreviewDungeon->Destroy();
		}
		PreviewDungeon = World->SpawnActor<ADungeon>();
		CanvasInstance = Cast<UDungeonCanvasComponent>(PreviewDungeon->AddComponentByClass(UDungeonCanvasComponent::StaticClass(), false, FTransform::Identity, false));
		CanvasInstance->bGenerateFloorTexturesOnDemand = false;		// Build everything at once since we're previewing it
		if (AssetBeingEdited && AssetBeingEdited->PreviewDungeonProperties) {
			CanvasInstance->LayoutDrawMarginPercent = AssetBeingEdited->PreviewDungeonProperties->LayoutDrawMarginPercent;
		}
	}

	if (IsValid(CanvasInstance.Get()) && PreviewDungeon.IsValid()) {
		BuildPreviewDungeon();
		CanvasInstance->Initialize(PreviewDungeon.Get());
	}

	MaterialLayerList->RefreshListView();
}


UDungeonCanvasComponent* FDungeonCanvasMaterialThemeEditor::GetInstance() const {
	return CanvasInstance.Get();
}

#undef LOCTEXT_NAMESPACE

