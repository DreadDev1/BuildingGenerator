//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"

#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Dungeon.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphEditorImpl.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Utils/DungeonThemeGraphLayout.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeCommands.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeToolkit.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBuilders.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeMarkerNodeTool.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeSelectTool.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeVisualNodeTool.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "GraphEditor.h"
#include "InteractiveToolManager.h"
#include "LevelEditorViewport.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "UObject/ObjectSaveContext.h"

#define LOCTEXT_NAMESPACE "DungeonThemeEdMode"

const FEditorModeID UDungeonThemeEdMode::EM_DungeonTheme = TEXT("EM_DungeonThemeEditorMode");

namespace DA::ThemeEdMode {
	template<typename TTool>
	bool ActivateTool(const UEdMode* InEdMode) {
		if (UInteractiveToolManager* ToolManager = InEdMode ? InEdMode->GetToolManager() : nullptr) {
			ToolManager->SelectActiveToolType(EToolSide::Left, TTool::ToolIdentifier);
			return ToolManager->ActivateTool(EToolSide::Left);
		}
		return false;
	}
}


////////////// UDungeonThemeEdMode //////////////
UDungeonThemeEdMode::UDungeonThemeEdMode() {
	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UDungeonThemeEdMode::EM_DungeonTheme,
		LOCTEXT("ModeName", "Dungeon Theme"),
		FSlateIcon(),
		true);
}

UDungeonThemeEdMode::~UDungeonThemeEdMode() {
}

void UDungeonThemeEdMode::ModeTick(float DeltaTime) {
	Super::ModeTick(DeltaTime);
	
	DeferredExecutor.Tick();
	
	UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool();
	if (!ActiveTool) {
		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
		ActivateSelectTool();
	}
	
	if (ThemeGraphEditorImpl.IsValid()) {
		ThemeGraphEditorImpl->Tick(DeltaTime);
	}
}

bool UDungeonThemeEdMode::IsCompatibleWith(FEditorModeID OtherModeID) const {
	return true;
}

void UDungeonThemeEdMode::SetThemeGraphState(ADungeon* InDungeon, UDungeonThemeAsset* InThemeAsset) {
	if (InDungeon == ActiveDungeon && InThemeAsset == ActiveThemeAsset) {
		// Same as existing state. do nothing
		return;
	}

	if (ActiveDungeon.IsValid()) {
		ActiveDungeon->OnDungeonBuildComplete.RemoveAll(this);
		ActiveDungeon->OnDungeonPropertyChanged.RemoveAll(this);
	}
	
	if (!InDungeon || !InThemeAsset) {
		// Invalid state
		ActiveDungeon = nullptr;
		ActiveThemeAsset = nullptr;
		ThemeGraphEditorImpl = nullptr;
		return;
	}

	ActiveDungeon = InDungeon;
	ActiveThemeAsset = InThemeAsset;

	// Recreate the markers in the ActiveThemeAssets if they do not match the builder class in the ActiveDungeon
	if (InDungeon && InThemeAsset && InThemeAsset->PreviewViewportProperties) {
		if (InDungeon->BuilderClass && InThemeAsset->PreviewViewportProperties->BuilderClass != InDungeon->BuilderClass) {
			InThemeAsset->PreviewViewportProperties->BuilderClass = InDungeon->BuilderClass;
			if (UEdGraph_DungeonTheme* DungeonGraph = Cast<UEdGraph_DungeonTheme>(ActiveThemeAsset->UpdateGraph)) {
				DungeonGraph->RecreateDefaultMarkerNodes(ActiveDungeon->BuilderClass);
			}
		}
	}

	if (ActiveDungeon.IsValid()) {
		ActiveDungeon->OnDungeonBuildComplete.AddDynamic(this, &UDungeonThemeEdMode::OnDungeonBuildComplete);
		ActiveDungeon->OnDungeonPropertyChanged.AddDynamic(this, &UDungeonThemeEdMode::OnDungeonPropertyChanged);
	}

	FThemeGraphEditorImplSettings Settings;
	Settings.ThemeGraph = InThemeAsset->UpdateGraph;
	Settings.PropertyEditor = ToolkitImpl->GetPropertyEditor();
	ThemeGraphEditorImpl = MakeShared<FThemeGraphEditorImpl>();
	ThemeGraphEditorImpl->Init(Settings);

	ThemeGraphEditorImpl->GetOnBuildDungeon().BindUObject(this, &UDungeonThemeEdMode::BuildActiveDungeon);
	ThemeGraphEditorImpl->GetOnGraphChanged().BindUObject(this, &UDungeonThemeEdMode::NotifyGraphChange);
	ThemeGraphEditorImpl->GetOnSelectedNodesChanged().BindUObject(this, &UDungeonThemeEdMode::SelectedNodesChanged);
	ThemeGraphEditorImpl->GetOnNodeDoubleClicked().BindUObject(this, &UDungeonThemeEdMode::NodeDoubleClicked);
	ThemeGraphEditorImpl->GetOnNodePropertyChanged().BindUObject(this, &UDungeonThemeEdMode::NodePropertyChanged);
}

bool UDungeonThemeEdMode::Select(AActor* InActor, bool bInSelected) {
	if (!bInSelected) {
		return false;
	}
	
	FDungeonThemeEdModeActorSelection OldActorSelection = ActorSelection;

	if (bInSelected) {
		ActorSelection = {};
		if (InActor && ActiveDungeon.IsValid() && ActiveThemeAsset.IsValid()) {
			if (FDungeonUtils::ActorBelongsToDungeon(InActor, ActiveDungeon.Get())) {
				UEdGraphNode_DungeonThemeActorBase* VisualNode = FDungeonArchitectThemeEditorUtils::FindThemeNodeFromSpawnedActor(InActor, ActiveThemeAsset->UpdateGraph);
				RequestFocusOnThemeNode(VisualNode);
				ActorSelection.Initialize(InActor, VisualNode, ActiveDungeon.Get());
				ShowObjectDetails({ ActorSelection.GetSelectedActorThemeNode() }, false);
			}
		}
	}
	
	if (OldActorSelection.GetSelectedActor() != ActorSelection.GetSelectedActor()) {
		if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
			ActiveTool->OnSelectedActorChanged();
		}	
	}

	return false;
}

bool UDungeonThemeEdMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const {
	if (InActor == ActiveDungeon.Get()) {
		return true;
	}
	
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		return ActiveTool->IsActorSelectionAllowed(InActor, bInSelection);
	}
	return true;
}

void UDungeonThemeEdMode::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) {
	Super::OnToolEnded(Manager, Tool);
	
}

void UDungeonThemeEdMode::Initialize() {
	Super::Initialize();

}

void UDungeonThemeEdMode::Enter() {
	Super::Enter();
	
	FEditorDelegates::PreSaveWorldWithContext.AddUObject(this, &UDungeonThemeEdMode::OnPreSaveWorld);

	const FDungeonThemeEdModeCommands& DungeonThemeEdModeCommands = FDungeonThemeEdModeCommands::Get();
	UDungeonThemeEdModeSelectToolBuilder* SelectToolBuilder = NewObject<UDungeonThemeEdModeSelectToolBuilder>(this);
	SelectToolBuilder->SetEdMode(this);
	RegisterTool(DungeonThemeEdModeCommands.SelectTool, UDungeonThemeEdModeSelectTool::ToolIdentifier, SelectToolBuilder);

	UDungeonThemeEdModeVisualNodeToolBuilder* VisualNodeToolBuilder = NewObject<UDungeonThemeEdModeVisualNodeToolBuilder>(this);
	VisualNodeToolBuilder->SetEdMode(this);
	RegisterTool(DungeonThemeEdModeCommands.VisualNodeTool, UDungeonThemeEdModeVisualNodeTool::ToolIdentifier, VisualNodeToolBuilder);

	MarkerNodeToolBuilder = NewObject<UDungeonThemeEdModeMarkerNodeToolBuilder>(this);
	MarkerNodeToolBuilder->SetEdMode(this);
	RegisterTool(DungeonThemeEdModeCommands.VisualNodeTool, UDungeonThemeEdModeMarkerNodeTool::ToolIdentifier, MarkerNodeToolBuilder);
	
	FEditorDelegates::OnNewActorsDropped.AddUObject(this, &UDungeonThemeEdMode::OnNewActorsDropped);
}

void UDungeonThemeEdMode::Exit() {
	FEditorDelegates::OnNewActorsDropped.RemoveAll(this);
	MarkerNodeToolBuilder = nullptr;

	TArray<FDungeonThemeGraphBuildError> CompileErrors;
	CompileThemeGraph(CompileErrors, true);
	
	if (ActiveDungeon.IsValid()) {
		ActiveDungeon->OnDungeonBuildComplete.RemoveAll(this);
	}

	FEditorDelegates::PreSaveWorldWithContext.RemoveAll(this);
	
	Super::Exit();
}

void UDungeonThemeEdMode::CreateToolkit() {
	ToolkitImpl = MakeShareable(new FDungeonThemeEdModeToolkit);
	Toolkit = ToolkitImpl;
}

void UDungeonThemeEdMode::OnNewActorsDropped(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& DroppedActors) {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		ActiveTool->HandleNewActorsDropped(DroppedActors);
	}
}

bool UDungeonThemeEdMode::ProcessEditDelete() {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		if (ActiveTool->ProcessDeleteSelectedActor()) {
			return true;
		}
	}
	return Super::ProcessEditDelete();
}

bool UDungeonThemeEdMode::ProcessEditCut() {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		if (ActiveTool->ProcessDeleteSelectedActor()) {
			return true;
		}
	}
	return Super::ProcessEditCut();
}

bool UDungeonThemeEdMode::ProcessEditDuplicate() {
	return true;
}

void UDungeonThemeEdMode::ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, bool bOffsetLocations) {
	Super::ActorsDuplicatedNotify(PreDuplicateSelection, PostDuplicateSelection, bOffsetLocations);
	
}

ADungeon* UDungeonThemeEdMode::GetActiveDungeon() const {
	return ActiveDungeon.Get();
}

UDungeonThemeAsset* UDungeonThemeEdMode::GetThemeAsset() const {
	return ActiveThemeAsset.Get();
}

void UDungeonThemeEdMode::RequestFocusOnThemeNode(UEdGraphNode* InNode) const {
	if (ThemeGraphEditorImpl.IsValid()) {
		ThemeGraphEditorImpl->SelectAndFocusOnNode(InNode);
	}
}

void UDungeonThemeEdMode:: BuildActiveDungeon(const FDungeonBuildSettings& InBuildSettings) {
	// Override the build settings when building from the ed mode
	FDungeonBuildSettings BuildSettings = InBuildSettings;
	{
		// We want to override the instance setting to make it always non-instanced, so we can move the individual meshes around
		BuildSettings.bOverrideInstancedSetting = true;
		BuildSettings.bInstanced = false;

		if (!BuildSettings.bOverrideFrameBuildTimeMs) {
			// We want to build the dungeon asynchronously
			BuildSettings.bOverrideFrameBuildTimeMs = true;
			BuildSettings.FrameBuildTimeMs = 64.0f;
		}
	}

	if (ActiveDungeon.IsValid() && ActiveThemeAsset.IsValid()) {
		// Compile the theme graph asset
		TArray<FDungeonThemeGraphBuildError> CompileErrors;
		if (FDungeonArchitectThemeEditorUtils::CompileThemeGraph(ActiveThemeAsset.Get(), CompileErrors)) {
			// Build the active dungeon
			FDungeonEditorUtils::BuildDungeon(ActiveDungeon.Get(), BuildSettings, false);
		}
	}
}

bool UDungeonThemeEdMode::CompileThemeGraph(TArray<FDungeonThemeGraphBuildError>& OutCompileErrors, bool bAutoSaveAsset) const {
	if (ActiveThemeAsset.IsValid()) {
		// Compile the theme graph asset
		if (FDungeonArchitectThemeEditorUtils::CompileThemeGraph(ActiveThemeAsset.Get(), OutCompileErrors)) {
			// Save the theme asset
			if (bAutoSaveAsset) {
				FViewport* Viewport = GCurrentLevelEditingViewportClient
					? GCurrentLevelEditingViewportClient->Viewport
					: nullptr;
				FDungeonArchitectThemeEditorUtils::SaveThemeAsset(ActiveThemeAsset.Get(), Viewport);
			}
			return true;
		}
	}
	return false;
}

void UDungeonThemeEdMode::RequestDeleteThemeNode(UEdGraphNode_DungeonThemeActorBase* VisualNode, bool bRecompile) {
	if (!VisualNode) {
		return;
	}

	if (UEdGraph* EdGraph = VisualNode->GetGraph()) {
		EdGraph->RemoveNode(VisualNode);
	}

	if (bRecompile) {
		TArray<FDungeonThemeGraphBuildError> CompileErrors;
		CompileThemeGraph(CompileErrors);
	}
	
	PerformGraphLayoutNextFrame();
}


void UDungeonThemeEdMode::PerformGraphLayout() const {
	TSharedPtr<SGraphEditor> GraphEditor = GetThemeGraphEditor()->GetGraphEditor();
	TObjectPtr<UEdGraph> ThemeGraph = ActiveThemeAsset.IsValid() ? ActiveThemeAsset->UpdateGraph : nullptr;
	if (!GraphEditor.IsValid() || !ThemeGraph) {
		return;
	}
	
	FDungeonThemeGraphLayout::FSettings LayoutSettings;
	FDungeonThemeGraphLayout::PerformLayout(ThemeGraph, GraphEditor, LayoutSettings);

	// Jump to the selected node
	FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(Selection); NodeIt; ++NodeIt) {
		if (UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt)) {
			GraphEditor->JumpToNode(SelectedNode);
			break;
		}
	}
}

void UDungeonThemeEdMode::PerformGraphLayoutNextFrame() {
	TWeakObjectPtr<UDungeonThemeEdMode> WeakThis = this;
	DeferredExecutor.EnqueueExecutionForNextTick([WeakThis]() {
		if (WeakThis.IsValid()) {
			WeakThis->PerformGraphLayout();
		}
	});
}

void UDungeonThemeEdMode::OnPreSaveWorld(UWorld* World, FObjectPreSaveContext ObjectPreSaveContext) const {
	TArray<FDungeonThemeGraphBuildError> CompileErrors;
	CompileThemeGraph(CompileErrors, true);
}

void UDungeonThemeEdMode::NotifyGraphChange(const FEdGraphEditAction& Action) {
	if (ToolkitImpl.IsValid()) {
		ToolkitImpl->RefreshMarkerListView();
	}

	if (ActiveDungeon.IsValid()) {
		ActiveDungeon->Modify();
	}
}

void UDungeonThemeEdMode::NodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent) {
	if (ActiveThemeAsset.IsValid()) {
		ActiveThemeAsset->Modify();
	}

	if (ActiveDungeon.IsValid()) {
		ActiveDungeon->Modify();
	}
}


void UDungeonThemeEdMode::SelectedNodesChanged(const TSet<UObject*>& SelectedNodes) {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		ActiveTool->OnSelectedNodesChanged(SelectedNodes);
	}
}

void UDungeonThemeEdMode::NodeDoubleClicked(UEdGraphNode* InNode) const {
	if (UEdGraphNode_DungeonThemeBase* ThemeNode = Cast<UEdGraphNode_DungeonThemeBase>(InNode)) {
		if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
			ActiveTool->OnThemeNodeDoubleClicked(ThemeNode);
		}
	}
}

void UDungeonThemeEdMode::ExecuteNextFrame(const TFunction<void(UDungeonThemeEdMode*)>& InCallback) {
	TWeakObjectPtr<UDungeonThemeEdMode> WeakThis = this;
	AsyncTask(ENamedThreads::GameThread, [WeakThis, InCallback]() {
		if (WeakThis.IsValid()) {
			InCallback(WeakThis.Get());
		}
	});
}

UDungeonThemeEdModeToolBase* UDungeonThemeEdMode::GetActiveTool() const {
	if (UInteractiveToolManager* ToolManager = GetToolManager()) {
		return Cast<UDungeonThemeEdModeToolBase>(ToolManager->GetActiveTool(EToolSide::Left));
	}
	return nullptr;
}

void UDungeonThemeEdMode::OnDungeonBuildComplete(ADungeon* Dungeon, bool bSuccess) {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		ActiveTool->OnDungeonBuildComplete();
	}
}

void UDungeonThemeEdMode::OnDungeonPropertyChanged(ADungeon* InDungeon, const FName& InPropertyName) {
	// Recreate the default marker list when the dungeon builder class changes
	if (InPropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, BuilderClass)) {
		if (ActiveDungeon == InDungeon) {
			if (ActiveDungeon->BuilderClass) {
				if (ActiveThemeAsset.IsValid() && ActiveThemeAsset->UpdateGraph) {
					if (UEdGraph_DungeonTheme* DungeonGraph = Cast<UEdGraph_DungeonTheme>(ActiveThemeAsset->UpdateGraph)) {
						DungeonGraph->RecreateDefaultMarkerNodes(ActiveDungeon->BuilderClass);
					}
				}
			}
		}
	}
}

void UDungeonThemeEdMode::ActivateSelectTool() const {
	DA::ThemeEdMode::ActivateTool<UDungeonThemeEdModeSelectTool>(this);
}

void UDungeonThemeEdMode::ActivateMarkerTool(UEdGraphNode_DungeonThemeMarker* InMarkerNode) const {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		if (ActiveTool->IsA<UDungeonThemeEdModeMarkerNodeTool>()) {
			// Already in this tool, and we don't want to change actors or nodes while in this mode
			return;
		}
	}

	if (InMarkerNode) {
		MarkerNodeToolBuilder->SetMarkerNode(InMarkerNode);
		DA::ThemeEdMode::ActivateTool<UDungeonThemeEdModeMarkerNodeTool>(this);
	}
}

void UDungeonThemeEdMode::ActivateVisualNodeEdTool(UEdGraphNode_DungeonThemeActorBase* InThemeNode) {
	if (UDungeonThemeEdModeToolBase* ActiveTool = GetActiveTool()) {
		if (ActiveTool->IsA<UDungeonThemeEdModeVisualNodeTool>()) {
			// Already in this tool, and we don't want to change actors or nodes while in this mode
			return;
		}
	}
	
	AActor* PreferredActor = ActorSelection.GetSelectedActor();
	if (PreferredActor && ActiveDungeon.IsValid()) {
		if (!FDungeonUtils::ActorBelongsToDungeon(PreferredActor, ActiveDungeon.Get())) {
			PreferredActor = nullptr;
		}
	}

	if (!InThemeNode && !PreferredActor) {
		return;
	}

	if (!InThemeNode) {
		// Try to grab it from the actor
		if (ActiveThemeAsset.IsValid()) {
			InThemeNode = FDungeonArchitectThemeEditorUtils::FindThemeNodeFromSpawnedActor(PreferredActor, ActiveThemeAsset->UpdateGraph);
		}
	}
	
	if (InThemeNode) {
		ActorSelection.Initialize(PreferredActor, InThemeNode, ActiveDungeon.Get());
		DA::ThemeEdMode::ActivateTool<UDungeonThemeEdModeVisualNodeTool>(this);
	}
}


void UDungeonThemeEdMode::RequestActivateSelectTool() {
	DeferredExecutor.EnqueueExecutionForNextTick([this]() {
		ActivateSelectTool();
	});
}

void UDungeonThemeEdMode::RequestActivateVisualNodeEdTool(UEdGraphNode_DungeonThemeActorBase* InThemeNode) {
	TWeakObjectPtr<UEdGraphNode_DungeonThemeActorBase> WeakThemeNode = InThemeNode;
	DeferredExecutor.EnqueueExecutionForNextTick([this, WeakThemeNode]() {
		ActivateVisualNodeEdTool(WeakThemeNode.Get());
	});
}

void UDungeonThemeEdMode::RequestActivateMarkerNodeEdTool(UEdGraphNode_DungeonThemeMarker* InMarkerNode) {
	TWeakObjectPtr<UEdGraphNode_DungeonThemeMarker> WeakMarkerNode = InMarkerNode;
	DeferredExecutor.EnqueueExecutionForNextTick([this, WeakMarkerNode]() {
		ActivateMarkerTool(WeakMarkerNode.Get());
	});
}

void UDungeonThemeEdMode::ShowObjectDetails(const TArray<UObject*>& InObjects, bool bForceRefresh) const {
	//FDungeonLevelEditorUtils::SetDetailsViewObjects(InObjects, bForceRefresh);
	if (ToolkitImpl.IsValid()) {
		ToolkitImpl->ShowObjectDetails(InObjects, bForceRefresh);
	}
}

TSharedRef<FLegacyEdModeWidgetHelper> UDungeonThemeEdMode::CreateWidgetHelper() {
	return MakeShared<FDungeonThemeEdModeWidgetHelper>();
}

////////////////////////////// FDungeonThemeEdModeWidgetHelper //////////////////////////////
bool FDungeonThemeEdModeWidgetHelper::UsesTransformWidget() const {
	if (UDungeonThemeEdModeToolBase* Tool = GetActiveTool()) {
		return Tool->UsesTransformWidget();
	}
	return FLegacyEdModeWidgetHelper::UsesTransformWidget();
}

UDungeonThemeEdModeToolBase* FDungeonThemeEdModeWidgetHelper::GetActiveTool() const {
	if (Owner) {
		if (UModeManagerInteractiveToolsContext* ToolsContext = Owner->GetInteractiveToolsContext()) {
			if (UEdMode* EdMode = Owner->GetActiveScriptableMode(UDungeonThemeEdMode::EM_DungeonTheme)) {
				if (UInteractiveToolManager* ToolManager = EdMode->GetToolManager()) {
					return Cast<UDungeonThemeEdModeToolBase>(ToolManager->GetActiveTool(EToolSide::Left));
				}
			}
		}
	}
	return nullptr;
}


#undef LOCTEXT_NAMESPACE

