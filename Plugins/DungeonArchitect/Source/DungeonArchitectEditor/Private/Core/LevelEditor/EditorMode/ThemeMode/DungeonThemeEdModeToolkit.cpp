//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeToolkit.h"

#include "Core/Dungeon.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphEditorImpl.h"
#include "Core/Editors/ThemeEditor/Widgets/SMarkerListView.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeCommands.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeToolbar.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeVisualNodeTool.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "GraphEditor.h"
#include "InteractiveToolManager.h"
#include "LevelEditorViewport.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SPrimaryButton.h"
#include "Toolkits/AssetEditorModeUILayer.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "Tools/UEdMode.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "DungeonThemeEdModeToolkit"

namespace DA::Private {
	
}

FDungeonThemeEdModeToolkit::FDungeonThemeEdModeToolkit() {
	bUsesToolkitBuilder = true;
}

void FDungeonThemeEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) {
	SAssignNew(ThemeGraphEditorHost, SBox);

	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	ActiveToolName = FText::GetEmpty();
	ActiveToolMessage = FText::GetEmpty();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	//DetailsViewArgs.NotifyHook = this;	// needed?
	PropertyEditor = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	MarkerListView = SNew(SMarkerListView)
		.OnMarkerDoubleClicked(this, &FDungeonThemeEdModeToolkit::OnMarkerListDoubleClicked);
	
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolNotificationMessage.AddSP(this, &FDungeonThemeEdModeToolkit::PostNotification);
	GetToolkitHost()->OnActiveViewportChanged().AddSP(this, &FDungeonThemeEdModeToolkit::OnActiveViewportChanged);
}

void FDungeonThemeEdModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const {
	PaletteNames.Add(NAME_Default);
}


void FDungeonThemeEdModeToolkit::InvokeUI() {
	//FModeToolkit::InvokeUI();
	
	if (TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin()) {
		TSharedPtr<FTabManager> TabManagerPtr = ModeUILayerPtr->GetTabManager();
		if (!TabManagerPtr) {
			return;
		}
		ThemeGraphTab = TabManagerPtr->TryInvokeTab(UAssetEditorUISubsystem::BottomLeftTabID);
	}
}


void FDungeonThemeEdModeToolkit::RequestModeUITabs() {
	FModeToolkit::RequestModeUITabs();
	
	if (TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin())
	{
		TSharedPtr<FWorkspaceItem> MenuModeCategoryPtr = ModeUILayerPtr->GetModeMenuCategory();

		if(!MenuModeCategoryPtr)
		{ 
			return;
		}
		ThemeGraphTabInfo.OnSpawnTab = FOnSpawnTab::CreateSP(SharedThis(this), &FDungeonThemeEdModeToolkit::CreateThemeGraphTab);
		ThemeGraphTabInfo.TabLabel = LOCTEXT("DungeonGraph", "Dungeon Graph");
		ThemeGraphTabInfo.TabTooltip = LOCTEXT("ModesToolboxTabTooltipText", "Open the Dungeon Graph tab, to design your dungeon layout");
		ThemeGraphTabInfo.TabIcon = GetEditorModeIcon();
		ModeUILayerPtr->SetModePanelInfo(UAssetEditorUISubsystem::BottomLeftTabID, ThemeGraphTabInfo);
	}
}

void FDungeonThemeEdModeToolkit::ShowObjectDetails(const TArray<UObject*>& InObjects, bool bForceRefresh) const {
	if (PropertyEditor.IsValid()) {
		PropertyEditor->SetObjects(InObjects, bForceRefresh);
	}
}

void FDungeonThemeEdModeToolkit::OnThemeSelectionChanged(ADungeon* InDungeon, UDungeonThemeAsset* InThemeAsset) const {
	FDungeonArchitectThemeEditorUtils::InitializeThemeAsset(InThemeAsset);
	
	if (InThemeAsset && InThemeAsset->UpdateGraph) {
		if (UDungeonThemeEdMode* ThemeEdMode = Cast<UDungeonThemeEdMode>(OwningEditorMode)) {
			ThemeEdMode->SetThemeGraphState(InDungeon, InThemeAsset);
			
			TSharedPtr<SWidget> ThemeGraphPanelContent = SNullWidget::NullWidget;
			TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditorImpl = ThemeEdMode->GetThemeGraphEditor();
			if (ThemeGraphEditorImpl.IsValid() && ThemeGraphEditorImpl->GetGraphEditorHost().IsValid()) {
				ThemeGraphPanelContent = ThemeGraphEditorImpl->GetGraphEditorHost();
			}
			
			ThemeGraphEditorHost->SetContent(ThemeGraphPanelContent.ToSharedRef());
		}
	}
	
	RefreshMarkerListView();
}

void FDungeonThemeEdModeToolkit::OnDungeonBuildClicked() const {
	if (UDungeonThemeEdMode* ThemeEdMode = Cast<UDungeonThemeEdMode>(OwningEditorMode.Get())) {
		FDungeonBuildSettings BuildSettings{};
		ThemeEdMode->BuildActiveDungeon(BuildSettings);
	}
}

void FDungeonThemeEdModeToolkit::OnPerformLayoutClicked() const {
	if (UDungeonThemeEdMode* ThemeEdMode = Cast<UDungeonThemeEdMode>(OwningEditorMode.Get())) {
		ThemeEdMode->PerformGraphLayout();
	}
}

TSharedRef<SDockTab> FDungeonThemeEdModeToolkit::CreateThemeGraphTab(const FSpawnTabArgs& Args) {
	TSharedPtr<SWidget> BodyContent = SNew(SSplitter)
		.Orientation(Orient_Vertical)
		+SSplitter::Slot()
		.Value(0.65)
		[
			ThemeGraphEditorHost.ToSharedRef()
		]
		+SSplitter::Slot()
		.Value(0.35)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+SSplitter::Slot()
			.Value(0.5)
			[
				PropertyEditor.ToSharedRef()
			]
			+SSplitter::Slot()
			.Value(0.5)
			[
				MarkerListView.ToSharedRef()
			]
		];
	
	TSharedPtr<SWidget> TabContent = SNew(SBox)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SDungeonThemeEdModeToolbar)
				.OnThemeSelected(this, &FDungeonThemeEdModeToolkit::OnThemeSelectionChanged)
				.OnDungeonBuildClicked(this, &FDungeonThemeEdModeToolkit::OnDungeonBuildClicked)
				.OnPerformLayoutClicked(this, &FDungeonThemeEdModeToolkit::OnPerformLayoutClicked)
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0)
			.VAlign(VAlign_Fill)
			[
				BodyContent.ToSharedRef()
			]
		];
	
	TSharedPtr<SDockTab> CreatedTab = SNew(SDockTab)
		[
			TabContent.ToSharedRef()
		];

	ThemeGraphTab = CreatedTab;
	return CreatedTab.ToSharedRef();
}

void FDungeonThemeEdModeToolkit::OnMarkerListDoubleClicked(const TSharedPtr<FMarkerListEntry>& Entry) const {
	if (Entry.IsValid()) {
		if (UEdGraphNode* Node = Entry->MarkerNode.Get()) {
			if (UDungeonThemeEdMode* ThemeEdMode = Cast<UDungeonThemeEdMode>(OwningEditorMode)) {
				TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditorImpl = ThemeEdMode->GetThemeGraphEditor();
				if (ThemeGraphEditorImpl.IsValid()) {
					TSharedPtr<SGraphEditor> GraphEditor = ThemeGraphEditorImpl->GetGraphEditor();
					if (GraphEditor.IsValid()) {
						GraphEditor->JumpToNode(Node);
					}
				}
			}
		}
	}
}

namespace DAThemeEdMode{
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

void FDungeonThemeEdModeToolkit::RefreshMarkerListView() const {
	if (UDungeonThemeEdMode* ThemeEdMode = Cast<UDungeonThemeEdMode>(OwningEditorMode)) {
		if (MarkerListView.IsValid()) {
			TArray<TSharedPtr<FMarkerListEntry>> MarkerEntries;
			DAThemeEdMode::GetMarkerNodes(ThemeEdMode->GetThemeAsset(), MarkerEntries);
			MarkerListView->Refresh(MarkerEntries);
		}
	}
}

FName FDungeonThemeEdModeToolkit::GetToolkitFName() const {
	return FName("DungeonThemeEdMode");
}

FText FDungeonThemeEdModeToolkit::GetBaseToolkitName() const {
	return NSLOCTEXT("DungeonThemeEdModeToolkit", "DisplayName", "Theme Editor");
}

void FDungeonThemeEdModeToolkit::PostNotification(const FText& Message) {
	ClearNotification();

	ActiveToolMessage = Message;

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		ActiveToolMessageHandle = GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PushStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessage);
	}
}

void FDungeonThemeEdModeToolkit::ClearNotification() {
	ActiveToolMessage = FText::GetEmpty();

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PopStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessageHandle);
	}
	ActiveToolMessageHandle.Reset();
}

void FDungeonThemeEdModeToolkit::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool) {
	FModeToolkit::OnToolStarted(Manager, Tool);
	if (UDungeonThemeEdModeToolBase* ThemeTool = Cast<UDungeonThemeEdModeToolBase>(Tool)) {
		bInActiveTool = true;
		ActiveToolName = Tool->GetToolInfo().ToolDisplayName;
	
		// try to update icon
		FString ActiveToolIdentifier = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveToolName(EToolSide::Left);
		ActiveToolIdentifier.InsertAt(0, ".");
		FName ActiveToolIconName = ISlateStyle::Join(FDungeonThemeEdModeCommands::Get().GetContextName(), TCHAR_TO_ANSI(*ActiveToolIdentifier));
		ActiveToolIcon = FDungeonArchitectStyle::Get().GetOptionalBrush(ActiveToolIconName);
	
		ThemeTool->AddOverlayWidgets(SharedThis(this));

		// Invalidate all the level viewports so that e.g. hitproxy buffers are cleared
		// (fixes the editor gizmo still being clickable despite not being visible)
		if (GIsEditor) {
			for (FLevelEditorViewportClient* Viewport : GEditor->GetLevelViewportClients()) {
				Viewport->Invalidate();
			}
		}
	}
}

void FDungeonThemeEdModeToolkit::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) {
	ActiveToolName = FText::GetEmpty();
	ActiveToolMessage = FText::GetEmpty();
	
	if (IsHosted()) {
		if (UDungeonThemeEdModeToolBase* ThemeTool = Cast<UDungeonThemeEdModeToolBase>(Tool)) {
			ThemeTool->RemoveOverlayWidgets(SharedThis(this));
		}
	}
	
	FModeToolkit::OnToolEnded(Manager, Tool);
}

void FDungeonThemeEdModeToolkit::OnActiveViewportChanged(TSharedPtr<IAssetViewport> OldViewport, TSharedPtr<IAssetViewport> NewViewport) const {
	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
	if (UDungeonThemeEdModeToolBase* ThemeTool = Cast<UDungeonThemeEdModeToolBase>(CurTool)) {
		if (OldViewport) {
			ThemeTool->RemoveOverlayWidgets(SharedThis(this), NewViewport);
		}
			
		ThemeTool->AddOverlayWidgets(SharedThis(this), NewViewport);
	}
}

#undef LOCTEXT_NAMESPACE

