//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/ThemeGraphAppMode.h"

#include "Core/DungeonBuilder.h"
#include "Core/DungeonConfig.h"
#include "Core/Editors/ThemeEditor/AppModes/Common/ThemeEditorAppTabFactoryMacros.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMeshList.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphEditorImpl.h"
#include "Core/Editors/ThemeEditor/Widgets/SGraphPalette_PropActions.h"
#include "Core/Editors/ThemeEditor/Widgets/SMarkerListView.h"
#include "Core/Editors/ThemeEditor/Widgets/SThemePreviewViewport.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "AdvancedPreviewScene.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "EdGraph/EdGraph.h"
#include "GraphEditAction.h"
#include "GraphEditor.h"
#include "IContentBrowserSingleton.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "SAdvancedPreviewDetailsTab.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FThemeGraphAppMode"
DEFINE_LOG_CATEGORY_STATIC(ThemeEditorGraphMode, Log, All);

namespace ThemeGraphAppModeTabs {
	static const FName TabID_GraphEditor(TEXT("GraphEditor"));
	static const FName TabID_Preview(TEXT("Preview"));
	static const FName TabID_Details(TEXT("Details"));
	static const FName TabID_ContentBrowser(TEXT("ContentBrowser"));
	static const FName TabID_Markers(TEXT("Markers"));
	static const FName TabID_Actions(TEXT("Actions"));
	static const FName TabID_PreviewSettings(TEXT("PreviewSettings"));

	DEFINE_THEME_EDITOR_TAB_FACTORY(GraphEditor,
				LOCTEXT("Caption_GraphEditor", "Theme Graph"),
				"LevelEditor.Tabs.Viewports",
				LOCTEXT("TooltipMenu_GraphEditor", "Design your theme by placing assets in this graph editor"),
				LOCTEXT("TooltipTab_GraphEditor", "Design your theme by placing assets in this graph editor"))
	
	DEFINE_THEME_EDITOR_TAB_FACTORY(Details,
				LOCTEXT("Caption_Details", "Details"),
				"LevelEditor.Tabs.Details",
				LOCTEXT("TooltipMenu_Details", "Details panel for modifying the node properties"),
				LOCTEXT("TooltipTab_Details", "Details panel for modifying the node properties"))

	DEFINE_THEME_EDITOR_TAB_FACTORY(Markers,
				LOCTEXT("Caption_Markers", "Markers"),
				"Kismet.Tabs.Palette",
				LOCTEXT("TooltipMenu_Markers", "Displays a list of markers in the theme graph."),
				LOCTEXT("TooltipTab_Markers", "Displays a list of markers in the theme graph. Double click to jump to a node"))

	DEFINE_THEME_EDITOR_TAB_FACTORY(Actions,
				LOCTEXT("Caption_Actions", "Actions"),
				"LevelEditor.Tabs.Layers",
				LOCTEXT("TooltipMenu_Markers", "Displays a list of actions to perform on the theme graph"),
				LOCTEXT("TooltipTab_Markers", "Displays a list of actions to perform on the theme graph."))

	
	DEFINE_THEME_EDITOR_TAB_FACTORY(PreviewSettings,
				LOCTEXT("Caption_PreviewSettings", "Preview Settings"),
				"LevelEditor.Tabs.Properties",
				LOCTEXT("TooltipMenu_PreviewSettings", "Displays a list of actions to perform on the theme graph"),
				LOCTEXT("TooltipTab_PreviewSettings", "Displays a list of actions to perform on the theme graph."))

	DEFINE_THEME_EDITOR_TAB_FACTORY_CUSTOM_SPAWNER(Preview,
				LOCTEXT("Caption_PreviewViewport", "Preview"),
				"LevelEditor.Tabs.Viewports",
				LOCTEXT("TooltipMenu_Preview", "Preview the generated dungeon in a 3D viewport"),
				LOCTEXT("TooltipTab_Preivew", "Preview the generated dungeon in a 3D viewport"))

	TSharedRef<SDockTab> FThemeEditorTabFactory_Preview::SpawnTab(const FWorkflowTabSpawnInfo& Info) const {
		TSharedRef<SDockTab> DockTab = FWorkflowTabFactory::SpawnTab(Info);
		const TSharedPtr<SThemePreviewViewport> PreviewViewport = StaticCastSharedPtr<SThemePreviewViewport>(WidgetPtr.Pin());
		if (PreviewViewport.IsValid()) {
			PreviewViewport->SetParentTab(FDungeonArchitectThemeEditor::AppModeID_GraphEditor, DockTab);
		}
		return DockTab;;
	}
	
	DEFINE_THEME_EDITOR_TAB_FACTORY_CUSTOM_SPAWNER(ContentBrowser,
				LOCTEXT("Caption_ContentBrowser", "Content Browser"),
				"LevelEditor.Tabs.ContentBrowser",
				LOCTEXT("TooltipMenu_ContentBrowser", "Content browser to help drag-drop assets into the theme graph"),
				LOCTEXT("TooltipTab_ContentBrowser", "Content browser to help drag-drop assets into the theme graph"))

	TSharedRef<SDockTab> FThemeEditorTabFactory_ContentBrowser::SpawnTab(const FWorkflowTabSpawnInfo& Info) const {
		TSharedRef<SDockTab> SpawnedTab = FWorkflowTabFactory::SpawnTab(Info);
		IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();

		const FName ContentBrowserID = TEXT("DA_ThemeEditor_ContentBrowser");
		FContentBrowserConfig ContentBrowserConfig;
		{
			/*
			ContentBrowserConfig.ThumbnailLabel =  EThumbnailLabel::ClassName ;
			ContentBrowserConfig.ThumbnailScale = 0.1f;
			ContentBrowserConfig.InitialAssetViewType = EAssetViewType::Column;
			ContentBrowserConfig.bShowBottomToolbar = true;
			ContentBrowserConfig.bCanShowClasses = true;
			ContentBrowserConfig.bUseSourcesView = true;
			ContentBrowserConfig.bExpandSourcesView = true;
			ContentBrowserConfig.bUsePathPicker = true;
			ContentBrowserConfig.bCanShowFilters = true;
			ContentBrowserConfig.bCanShowAssetSearch = true;
			ContentBrowserConfig.bCanShowFolders = true;
			ContentBrowserConfig.bCanShowRealTimeThumbnails = true;
			ContentBrowserConfig.bCanShowLockButton = true;
			*/
			ContentBrowserConfig.bCanShowDevelopersFolder = true;
			ContentBrowserConfig.bCanSetAsPrimaryBrowser = false;
		}
		
		const TSharedRef<SWidget> ContentBrowser = ContentBrowserSingleton.CreateContentBrowser(ContentBrowserID, SpawnedTab, &ContentBrowserConfig);
		SpawnedTab->SetContent(ContentBrowser);
		
		return SpawnedTab;;
	}
	
}

namespace {
	template <typename T>
	void GetGraphNodes(UEdGraph* Graph, TArray<T*>& OutResult) {
		OutResult.Reset();
		for (UEdGraphNode* Node : Graph->Nodes) {
			if (Node && Node->IsA<T>()) {
				OutResult.Add(Cast<T>(Node));
			}
		}
	}

	void GetMarkerNodes(UDungeonThemeAsset* ThemeAsset, TArray<TSharedPtr<FMarkerListEntry>>& OutResult) {
		OutResult.Reset();
		if (ThemeAsset && ThemeAsset->UpdateGraph) {
			TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
			GetGraphNodes<UEdGraphNode_DungeonThemeMarker>(ThemeAsset->UpdateGraph, MarkerNodes);
			for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerNodes) {
				TSharedPtr<FMarkerListEntry> Item = MakeShareable(new FMarkerListEntry);
				Item->MarkerName = MarkerNode->MarkerName;
				Item->MarkerNode = MarkerNode;
				OutResult.Add(Item);
			}
		}
	}
	
	template <typename T>
	bool ContainsNodesOfType(TSet<const UEdGraphNode*> Nodes) {
		for (const UEdGraphNode* Node : Nodes) {
			if (Cast<const T>(Node)) {
				return true;
			}
		}
		return false;
	}

}

/////////////////////// FThemeGraphAppMode ///////////////////////

FThemeGraphAppMode::FThemeGraphAppMode(TSharedPtr<FDungeonArchitectThemeEditor> InThemeEditor, TSharedPtr<SThemePreviewViewport> InPreviewViewport)
		: FThemeEditorAppModeBase(InThemeEditor, FDungeonArchitectThemeEditor::AppModeID_GraphEditor)
		, PreviewViewportPtr(InPreviewViewport)
{
}

void FThemeGraphAppMode::Init(TSubclassOf<UDungeonBuilder> InBuilderClass) {
	const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
	if (!ThemeEditor.IsValid()) return;
	
	UDungeonThemeAsset* AssetBeingEdited = ThemeEditor->GetAssetBeingEdited();
	
	BindCommands(ThemeEditor->GetToolkitCommands());
	
	PropertyEditor = CreatePropertyEditorWidget();
    ActionPalette = SNew(SGraphPalette_PropActions, AssetBeingEdited->UpdateGraph);
	MarkerListView = SNew(SMarkerListView)
		.OnMarkerDoubleClicked(this, &FThemeGraphAppMode::OnMarkerListDoubleClicked);
	
	RefreshMarkerListView();
	const TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
	if (PreviewViewport.IsValid()) {
		TSharedPtr<FAdvancedPreviewScene> PreviewScene;
		if (PreviewViewport.IsValid()) {
			PreviewScene = PreviewViewport->GetAdvancedPreview();
		}

		if (PreviewScene.IsValid()) {
			const TSharedPtr<SAdvancedPreviewDetailsTab> SettingsWidget = SNew(SAdvancedPreviewDetailsTab, PreviewScene.ToSharedRef());
			SettingsWidget->Refresh();
			PreviewSettingsWidget = SettingsWidget;
		}
		else {
			PreviewSettingsWidget = SNullWidget::NullWidget;
		}
	}

	FThemeGraphEditorImplSettings Settings;
	Settings.ThemeGraph = AssetBeingEdited->UpdateGraph;
	Settings.PropertyEditor = PropertyEditor;
	
	EditorImpl = MakeShared<FThemeGraphEditorImpl>();
	EditorImpl->Init(Settings);
	
	EditorImpl->GetOnBuildDungeon().BindSP(this, &FThemeGraphAppMode::HandleBuildDungeon);
	EditorImpl->GetOnGraphChanged().BindSP(this, &FThemeGraphAppMode::NotifyGraphChange);
	EditorImpl->GetOnSelectedNodesChanged().BindSP(this, &FThemeGraphAppMode::SelectedNodesChanged);
	EditorImpl->GetOnNodeDoubleClicked().BindSP(this, &FThemeGraphAppMode::NodeDoubleClicked);
	EditorImpl->GetOnNodePropertyChanged().BindSP(this, &FThemeGraphAppMode::NodePropertyChanged);
	EditorImpl->SetEditorDefaultPropertiesGetter([this]() {
		TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
		return PreviewViewport.IsValid() ? PreviewViewport->GetDungeonActor() : nullptr;
	});
	
	// Show the dungeon properties
	if (PropertyEditor.IsValid()) {
		ShowObjectDetails(nullptr);
	}

	TSharedPtr<SWidget> GraphEditorHost = EditorImpl->GetGraphEditorHost();
	if (!GraphEditorHost.IsValid()) {
		GraphEditorHost = SNullWidget::NullWidget;
	}
	
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_GraphEditor(ThemeEditor, GraphEditorHost)));
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_Preview(ThemeEditor, PreviewViewport)));
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_Details(ThemeEditor, PropertyEditor)));
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_ContentBrowser(ThemeEditor)));
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_Markers(ThemeEditor, MarkerListView)));
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_Actions(ThemeEditor, ActionPalette)));
	TabFactories.RegisterFactory(MakeShareable(new ThemeGraphAppModeTabs::FThemeEditorTabFactory_PreviewSettings(ThemeEditor, PreviewSettingsWidget)));

	TabLayout = FTabManager::NewLayout(
            "ThemeGraphAppMode_Layout_v0.0.2")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            ->Split
            (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Horizontal)
                ->SetSizeCoefficient(0.63f)
                ->Split
                (

                    FTabManager::NewSplitter()
                    ->SetOrientation(Orient_Vertical)
                    ->SetSizeCoefficient(0.66f)
                    ->Split // Graph Widget
                    (
                        FTabManager::NewStack()
                        ->SetSizeCoefficient(0.66f)
                        ->SetHideTabWell(true)
                        ->AddTab(ThemeGraphAppModeTabs::TabID_GraphEditor, ETabState::OpenedTab)
                    )
                    ->Split // ContentBrowser
                    (
                        FTabManager::NewStack()
                        ->SetSizeCoefficient(0.33f)
                        ->AddTab(ThemeGraphAppModeTabs::TabID_ContentBrowser, ETabState::OpenedTab)
                    )
                )
                ->Split
                (
                    FTabManager::NewSplitter()
                    ->SetOrientation(Orient_Vertical)
                    ->SetSizeCoefficient(0.37f)
                    ->Split
                    (
                        //FTabManager::NewStack()
                        FTabManager::NewSplitter()
                        ->SetOrientation(Orient_Horizontal)
                        ->SetSizeCoefficient(0.33f)
                        ->Split
                        (
                            FTabManager::NewStack()
                            ->SetSizeCoefficient(0.6f)
                            ->AddTab(ThemeGraphAppModeTabs::TabID_Details, ETabState::OpenedTab)
                        )
                        ->Split
                        (
                            FTabManager::NewStack()
                            ->SetSizeCoefficient(0.6f)
                            ->AddTab(ThemeGraphAppModeTabs::TabID_PreviewSettings, ETabState::OpenedTab)
                            ->AddTab(ThemeGraphAppModeTabs::TabID_Actions, ETabState::OpenedTab)
                            ->AddTab(ThemeGraphAppModeTabs::TabID_Markers, ETabState::OpenedTab)
                            ->SetForegroundTab(ThemeGraphAppModeTabs::TabID_Markers)
                        )
                    )
                    ->Split
                    (
                        FTabManager::NewStack()
                        ->SetSizeCoefficient(0.667f)
                        ->AddTab(ThemeGraphAppModeTabs::TabID_Preview, ETabState::OpenedTab)
                    )
                )
            )
        );

	
	ThemeEditor->GetToolbarBuilder()->AddThemeGraphToolbar(ToolbarExtender);
	ThemeEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
}

void FThemeGraphAppMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) {
	const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();

	ThemeEditor->RegisterToolbarTab(InTabManager.ToSharedRef());
	ThemeEditor->PushTabFactories(TabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FThemeGraphAppMode::Tick(float DeltaTime) {
	if (EditorImpl.IsValid()) {
		EditorImpl->Tick(DeltaTime);
	}
}

void FThemeGraphAppMode::ShowObjectDetails(UObject* ObjectProperties, bool bForceRefresh) const {
	if (EditorImpl.IsValid()) {
		EditorImpl->ShowObjectDetails(ObjectProperties, bForceRefresh);
	}
}

void FThemeGraphAppMode::SetBuilderClass(TSubclassOf<class UDungeonBuilder> InBuilderClass) {
	if (EditorImpl.IsValid()) {
		EditorImpl->SetBuilderClass(InBuilderClass);
	}
}

TSharedPtr<SGraphEditor> FThemeGraphAppMode::GetGraphEditor() const {
	return EditorImpl.IsValid() ? EditorImpl->GetGraphEditor() : nullptr;
}

UDungeonThemeAsset* FThemeGraphAppMode::GetThemeAsset() const {
	const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
	return ThemeEditor.IsValid() ? ThemeEditor->GetAssetBeingEdited() : nullptr;
}

void FThemeGraphAppMode::RefreshMarkerListView() const {
    if (MarkerListView.IsValid()) {
        TArray<TSharedPtr<FMarkerListEntry>> MarkerEntries;
        GetMarkerNodes(GetThemeAsset(), MarkerEntries);
        MarkerListView->Refresh(MarkerEntries);
    }
}


void FThemeGraphAppMode::OnMarkerListDoubleClicked(const TSharedPtr<FMarkerListEntry>& Entry) const {
    if (Entry.IsValid()) {
	    if (UEdGraphNode* Node = Entry->MarkerNode.Get()) {
	    	if (EditorImpl.IsValid()) {
	    		TSharedPtr<SGraphEditor> GraphEditor = EditorImpl->GetGraphEditor();
	    		if (GraphEditor.IsValid()) {
	    			GraphEditor->JumpToNode(Node);
	    		}
	    	}
        }
    }
}

void FThemeGraphAppMode::NotifyGraphChange(const FEdGraphEditAction& Action) {
	if (ContainsNodesOfType<UEdGraphNode_DungeonThemeMarker>(Action.Nodes)) {
		RefreshMarkerListView();
	}
}

void FThemeGraphAppMode::NodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent) {
	// The nodes that were modified and requires a clean rebuild by the scene provider
	TSet<FName> NodeObjectsToRebuild;

	// Flag the node id to generate it cleanly in the scene provider
	for (const UEdGraphNode* Node : Action.Nodes) {
		const UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<const UEdGraphNode_DungeonThemeActorBase>(Node);
		if (ActorNode) {
			FName NodeId(*ActorNode->NodeGuid.ToString());
			NodeObjectsToRebuild.Add(NodeId);
		}
	}

	const TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
	if (PreviewViewport.IsValid()) {
		PreviewViewport->SetNodesToRebuild(NodeObjectsToRebuild);
	}

	if (ContainsNodesOfType<UEdGraphNode_DungeonThemeMarker>(Action.Nodes)) {
		RefreshMarkerListView();
	}
}

void FThemeGraphAppMode::SelectedNodesChanged(const TSet<UObject*>& SelectedNodes) const {
	UEdGraphNode_DungeonThemeMarker* MarkerNode = nullptr;
	if (SelectedNodes.Num() == 1) {
		MarkerNode = Cast<UEdGraphNode_DungeonThemeMarker>(SelectedNodes.Array()[0]);
	}
    
	const TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
	if (PreviewViewport.IsValid()) {
		if (MarkerNode) {
			PreviewViewport->VisualizeMarkers({ *MarkerNode->MarkerName });
		}
		else {
			PreviewViewport->ClearMarkerVisualizations();
		}
	}
}

void FThemeGraphAppMode::NodeDoubleClicked(class UEdGraphNode* InNode) const {
	// Focus of the nearest spawned actor from this theme node
	if (const UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(InNode)) {
		const TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
		if (PreviewViewport.IsValid()) {
			const FName NodeId = FName(*ActorNode->NodeGuid.ToString());
			PreviewViewport->ZoomOnNearestNodeMesh(NodeId);
		}
	}
}

void FThemeGraphAppMode::HandleBuildFullDungeon() const {
	HandleBuildDungeon({});
}

void FThemeGraphAppMode::HandleBuildDungeon(const FDungeonBuildSettings& InBuildSettings) const {
	const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = ThemeEditorPtr.Pin();
	if (ThemeEditor.IsValid()) {
		TArray<FDungeonThemeGraphBuildError> CompileErrors;
		FDungeonArchitectThemeEditorUtils::CompileThemeGraph(ThemeEditor->GetAssetBeingEdited(), CompileErrors);
	}
	
	const TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
	if (PreviewViewport.IsValid()) {;
		PreviewViewport->RebuildDungeon(InBuildSettings);
	}
	
	if (ActionPalette.IsValid()) {
		ActionPalette->Refresh();
	}
}

void FThemeGraphAppMode::RecreateDefaultMarkerNodes(const TSubclassOf<UDungeonBuilder>& InBuilderClass) const {
	if (InBuilderClass) {
		UDungeonThemeAsset* ThemeAsset = GetThemeAsset();
		if (ThemeAsset && ThemeAsset->UpdateGraph) {
			if (UEdGraph_DungeonTheme* DungeonGraph = Cast<UEdGraph_DungeonTheme>(ThemeAsset->UpdateGraph)) {
				DungeonGraph->RecreateDefaultMarkerNodes(InBuilderClass);
			}
		}
	}
}

void FThemeGraphAppMode::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) {
	FThemeEditorAppModeBase::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);

	HandlePropertyChanged(PropertyChangedEvent);
	
}

void FThemeGraphAppMode::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged) {
	FThemeEditorAppModeBase::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
	
	HandlePropertyChanged(PropertyChangedEvent);
}

void FThemeGraphAppMode::HandlePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent) const {
	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyChangedEvent.GetNumObjectsBeingEdited() > 0) {
		if (const UObject* ObjectBeingEdited = PropertyChangedEvent.GetObjectBeingEdited(0)) {
			if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, BuilderClass)) {
				if (const ADungeon* ObjectDungeon = Cast<ADungeon>(ObjectBeingEdited)) {
					if (ObjectDungeon->BuilderClass) {
						RecreateDefaultMarkerNodes(ObjectDungeon->BuilderClass);
					}
				}
			}

			if (ObjectBeingEdited->IsA<UDungeonConfig>() || ObjectBeingEdited->IsA<ADungeon>()) {
				TSharedPtr<SThemePreviewViewport> PreviewViewport = PreviewViewportPtr.Pin();
				if (PreviewViewport.IsValid()) {
					if (AThemeEditorDungeonActor* DungeonActor = PreviewViewport->GetDungeonActor()) {
						DungeonActor->HandlePropertyChanged(PropertyName);
					}
				}
				FDungeonBuildSettings BuildSettings{};
				PreviewViewport->RebuildDungeon(BuildSettings);
			}
		}
	}
}

void FThemeGraphAppMode::BindCommands(TSharedRef<FUICommandList> ToolkitCommands) {
	const FThemeGraphAppModeCommands& Commands = FThemeGraphAppModeCommands::Get();

	ToolkitCommands->MapAction(
		Commands.Build,
		FExecuteAction::CreateSP(this, &FThemeGraphAppMode::HandleBuildFullDungeon));
}


/////////////////////////////// FThemeGraphAppModeCommands ///////////////////////////////
FThemeGraphAppModeCommands::FThemeGraphAppModeCommands()
	: TCommands<FThemeGraphAppModeCommands>(
		TEXT("DAThemeEd"),
		LOCTEXT("ContextDesc", "Theme Editor Graph"),
		NAME_None,
		FDungeonArchitectStyle::GetStyleSetName())
{
}

void FThemeGraphAppModeCommands::RegisterCommands() {
	UI_COMMAND(Build, "Build", "Build the pattern model", EUserInterfaceActionType::Button, FInputChord());
}


#undef LOCTEXT_NAMESPACE

