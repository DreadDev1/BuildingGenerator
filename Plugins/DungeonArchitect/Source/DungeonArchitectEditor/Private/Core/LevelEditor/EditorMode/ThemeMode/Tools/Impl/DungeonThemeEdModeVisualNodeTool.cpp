//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeVisualNodeTool.h"

#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/Common/Slate/SDADraggableToolOverlayWidget.h"
#include "Core/Common/Slate/SDAToolShutdownOverlay.h"
#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Common/Utils/DungeonEditorViewportCapture.h"
#include "Core/Common/Utils/DungeonScenePostProcess.h"
#include "Core/Dungeon.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphEditorImpl.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphLayoutManager.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolkit.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeUtils.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeSelectTool.h"
#include "Core/Markers/DungeonMarkerVisualization.h"
#include "Core/Settings/DungeonArchitectProjectUserSettings.h"
#include "Core/Utils/DungeonUtils.h"

#include "ActorFactories/ActorFactory.h"
#include "Engine/TextureRenderTarget2D.h"
#include "InteractiveToolManager.h"
#include "LevelEditorViewport.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "DungeonThemeEdModeVisualNodeTool"

class UDungeonArchitectProjectUserSettings;
const FString UDungeonThemeEdModeVisualNodeTool::ToolIdentifier = "DungeonThemeEdModeToolVisualNode";


///////////////////////////////// ADungeonThemeEdModeGizmoProxy /////////////////////////////////
void UDungeonThemeEdModeVisualNodeTool::Setup() {
	Super::Setup();
	
	SetToolDisplayName(LOCTEXT("ToolDisplayName", "Theme Node Editor"));
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	UEdGraphNode_DungeonThemeActorBase* ActiveThemeNode = ActorSelection.GetSelectedActorThemeNode();
	if (!ActiveThemeNode) {
		return;
	}

	SetActorBeingEdited(nullptr);
	TArray<FName> NodeTags;
	FDungeonArchitectThemeEditorUtils::GetNodeTags(ActiveThemeNode, NodeTags);
	for (const FName& NodeTag : NodeTags) {
		if (AActor* PreferredActorToEdit = ActorSelection.GetSelectedActor()) {
			// Make sure this actor belongs to the node that's being edited
			if (PreferredActorToEdit->Tags.Contains(NodeTag)) {
				SetActorBeingEdited(PreferredActorToEdit);
			}
		}
	} 
	
	if (!ActorBeingEdited.IsValid()) {
		if (GCurrentLevelEditingViewportClient) {
			// Find the nearest mesh spawned by this node
			FName NodeID = FDungeonUtils::GetNodeID(ActiveThemeNode);
			SetActorBeingEdited(FDungeonLevelEditorUtils::FindNearestNodeMesh(GCurrentLevelEditingViewportClient, NodeID));
		}
	}

	if (!ActorBeingEdited.IsValid()) {
		// Cannot find an actor to edit. End the tool
		if (EdMode.IsValid()) {
			EdMode->RequestActivateSelectTool();
		}
		return;
	}
	
	// Zoom in on the node
	FDungeonLevelEditorUtils::FocusViewportOnActor(GCurrentLevelEditingViewportClient, ActorBeingEdited.Get());
		
	// Select the actor
	if (GEditor) {
		GEditor->OnActorsMoved().AddUObject(this, &UDungeonThemeEdModeVisualNodeTool::HandleActorsMoved);
		GEditor->SelectNone(true, true, false);
		GEditor->SelectActor(ActorBeingEdited.Get(), true, true);
	}

	// Show the node properties
	EdMode->ShowObjectDetails({ ActiveThemeNode });
	
	VisualHighlightSaturation = GetMutableDefault<UDungeonArchitectProjectUserSettings>()->GetThemeEdModeActorHighlightSaturation();

	SetHighlightSaturation(VisualHighlightSaturation);

	if (ViewportCapture) {
		ViewportCapture->Deinitialize();
		ViewportCapture = nullptr;
	}
	
	ViewportCapture = NewObject<UDungeonEditorViewportCapture>();
	ViewportCapture->Initialize(TargetWorld.Get());
	
	// Disable the theme graph editor as we don't want the state to change when we're modifying the transform
	if (EdMode.IsValid()) {
		TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditor = EdMode->GetThemeGraphEditor();
		if (ThemeGraphEditor.IsValid()) {
			ThemeGraphEditor->SetIsEditable(false);
		}

		if (ADungeon* ActiveDungeon = EdMode->GetActiveDungeon()) {
			if (UDungeonBuilder* Builder = ActiveDungeon->GetBuilder()) {
				Builder->CreateMarkerVisualizationRules(MarkerVisualizationRules);
			}
		}
	}

	RefreshSelectedActor();
}

void UDungeonThemeEdModeVisualNodeTool::Shutdown(EToolShutdownType ShutdownType) {
	Super::Shutdown(ShutdownType);

	if (EdMode.IsValid()) {
		TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditor = EdMode->GetThemeGraphEditor();
		if (ThemeGraphEditor.IsValid()) {
			ThemeGraphEditor->SetIsEditable(true);
		}
	}

	if (GEditor) {
		GEditor->SelectNone(true, true, false);
	}

	if (ViewportCapture) {
		ViewportCapture->Deinitialize();
		ViewportCapture = nullptr;
	}
	
	if (GEditor) {
		GEditor->OnActorsMoved().RemoveAll(this);
	}
}

void UDungeonThemeEdModeVisualNodeTool::HandleActorsMoved(TArray<AActor*>& InActors) const {
	if (ActorBeingEdited.IsValid() && InActors.Contains(ActorBeingEdited.Get())) {
		UpdateThemeNodeTransform();
	}
}

void UDungeonThemeEdModeVisualNodeTool::UpdateThemeNodeTransform() const {
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	UEdGraphNode_DungeonThemeActorBase* ActiveThemeNode = ActorSelection.GetSelectedActorThemeNode();

	FTransform ParentTransform = FTransform::Identity;
	if (ActiveThemeNode) {
		UEdGraphNode_DungeonThemeBase* ParentNode = FDungeonArchitectThemeEditorUtils::GetParentNode(ActiveThemeNode);
		while (ParentNode) {
			if (UEdGraphNode_DungeonThemeActorBase* ParentActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(ParentNode)) {
				ParentTransform = ParentActorNode->Offset * ParentTransform;
			}
			ParentNode = FDungeonArchitectThemeEditorUtils::GetParentNode(ParentNode);
		}
	}

	auto GetSpawnedActorInfo = [this, ActiveThemeNode](FDungeonMarkerInstance& MarkerInfo) -> bool {
		if (ActorBeingEdited.IsValid() && ActiveThemeNode) {
			if (UDungeonSpawnDataComponent* SpawnData = ActorBeingEdited->GetComponentByClass<UDungeonSpawnDataComponent>()) {
				int32 MarkerIndex = SpawnData->MarkerIndex;
				if (ADungeon* ActiveDungeon = EdMode->GetActiveDungeon()) {
					if (UDungeonModel* DungeonModel = ActiveDungeon->GetModel()) {
						if (DungeonModel->WorldMarkers.IsValidIndex(MarkerIndex)) {
							MarkerInfo = DungeonModel->WorldMarkers[MarkerIndex];
							return true;
						}
					}
				}
			}
		}
		return false;
	};
	
	FDungeonMarkerInstance MarkerInfo{};
	if (GetSpawnedActorInfo(MarkerInfo)) {
		FTransform GizmoTransform = ActorBeingEdited->GetActorTransform();
		FTransform NodeLocalTransform = GizmoTransform.GetRelativeTransform(MarkerInfo.Transform);
		NodeLocalTransform = NodeLocalTransform.GetRelativeTransform(ParentTransform);
		ActiveThemeNode->Offset = NodeLocalTransform;
		if (EdMode.IsValid()) {
			TArray<FDungeonThemeGraphBuildError> CompileErrors;
			EdMode->CompileThemeGraph(CompileErrors);

			// Apply the theme engine on the existing marker setup
			FDungeonBuildSettings BuildSettings;
			BuildSettings.StartPhase = EDungeonBuildGraphPhase::SpawnItems;
			EdMode->BuildActiveDungeon(BuildSettings);
		}
	}
}

float UDungeonThemeEdModeVisualNodeTool::GetHighlightSaturation() const {
	return VisualHighlightSaturation;
}

void UDungeonThemeEdModeVisualNodeTool::SetHighlightSaturation(float InValue) {
	VisualHighlightSaturation = InValue;
	if (PostProcessor) {
		if (UMaterialInstanceDynamic* MID = PostProcessor->GetPostProcessMaterial()) {
			MID->SetScalarParameterValue("Saturation", VisualHighlightSaturation);
		}
	}
}

void UDungeonThemeEdModeVisualNodeTool::SaveHighlightSaturationUserSetting() const {
	GetMutableDefault<UDungeonArchitectProjectUserSettings>()->SetThemeEdModeActorHighlightSaturation(VisualHighlightSaturation);
}

void UDungeonThemeEdModeVisualNodeTool::CaptureOffscreenHighlightActors(const UWorld* InTargetWorld, const FMinimalViewInfo& ViewInfo, const FIntPoint& ViewSize) const {
	if (!InTargetWorld) {
		return;
	}

	TArray<AActor*> HighlightedActors = { ActorBeingEdited.Get() };
	
	if (ViewportCapture) {
		if (ViewSize.X > 0 && ViewSize.Y > 0) {
			ViewportCapture->SetRenderTargetSize(ViewSize.X, ViewSize.Y);
		}

		// Assign the actors to capture in the render target 
		{
			TArray<AActor*> ActorsToCapture;
			for (TWeakObjectPtr<AActor> ActorPtr : HighlightedActors) {
				if (ActorPtr.IsValid()) {
					ActorsToCapture.Add(ActorPtr.Get());
				}
			} 
			ViewportCapture->SetAlphaMaskedActors(ActorsToCapture);
		}
		
		ViewportCapture->CaptureAlphaMask(ViewInfo);

		const FName TextureParameterName = TEXT("InputTexture");
		if (PostProcessor) {
			if (UMaterialInstanceDynamic* PostProcessMID = PostProcessor->GetPostProcessMaterial()) {
				PostProcessMID->SetTextureParameterValue(TextureParameterName, ViewportCapture->GetOrCreateAlphaMaskRenderTarget());
			}
		}
	}
}

void UDungeonThemeEdModeVisualNodeTool::SetActorBeingEdited(AActor* InActor) {
	if (ActorBeingEdited == InActor) {
		// No change
		return;
	}
	
	ActorBeingEdited = InActor;
}


UMaterialInterface* UDungeonThemeEdModeVisualNodeTool::GetPostProcessMaterialTemplate() const {
	return LoadObject<UMaterialInterface>(nullptr,
		TEXT("/DungeonArchitect/Core/Editors/ThemeEditor/EdMode/Tools/NodeEditTool/M_DA_ThemeEdMode_PostProcess_ToolNodeEdit_Inst"));
}

void UDungeonThemeEdModeVisualNodeTool::Render(IToolsContextRenderAPI* RenderAPI) {
	Super::Render(RenderAPI);

	FMinimalViewInfo ViewInfo{};
	FIntPoint ViewSize{};
	FDungeonThemeEdModeUtils::GatherViewportInfo(RenderAPI->GetSceneView(), RenderAPI->GetCameraState(), ViewInfo, ViewSize);
	CaptureOffscreenHighlightActors(TargetWorld.Get(), ViewInfo, ViewSize);

	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	
	if (ActorSelection.IsCachedWorldMarkerValid()) {
		MarkerVisualizer.RenderMarker(RenderAPI->GetPrimitiveDrawInterface(), ActorSelection.GetCachedWorldMarker().MarkerName,
				ActorSelection.GetCachedWorldMarker().Transform, MarkerVisualizationRules);
	}
}

void UDungeonThemeEdModeVisualNodeTool::AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport) {
	Super::AddOverlayWidgets(InToolkit, InViewport);
	
	ToolShutdownViewportOverlayWidget = SNew(SDAToolShutdownOverlay, InToolkit->GetScriptableEditorMode())
		.DisplayText(GetToolInfo().ToolDisplayName);
	InToolkit->GetToolkitHost()->AddViewportOverlayWidget(ToolShutdownViewportOverlayWidget.ToSharedRef(), InViewport);
	
	ToolOverlayWidget = SNew(SDADraggableToolOverlayWidget)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ToolTitle", "Edit Tool"))
				.TextStyle(FDungeonArchitectStyle::Get(), "ThemeEdMode.Tool.Title")
			]
			+SVerticalBox::Slot()
			.Padding(6)
			.AutoHeight()
			[
				SNew(SGridPanel)
				.FillColumn(1, 1)
				.FillRow(0,1.0f)
				+SGridPanel::Slot(0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LabelHighlightStrength", "Highlight"))
				]
				+SGridPanel::Slot(1, 0)
				[
					SNew(SBox)
					.WidthOverride(100)
					[
						SNew(SSlider)
						.MinValue(0)
						.MaxValue(1)
						.Value_UObject(this, &UDungeonThemeEdModeVisualNodeTool::GetHighlightSaturation)
						.OnValueChanged_UObject(this, &UDungeonThemeEdModeVisualNodeTool::SetHighlightSaturation)
						.OnMouseCaptureEnd_UObject(this, &UDungeonThemeEdModeVisualNodeTool::SaveHighlightSaturationUserSetting)
					]
				]
			]
		];
	InToolkit->GetToolkitHost()->AddViewportOverlayWidget(ToolOverlayWidget.ToSharedRef(), InViewport);
}

void UDungeonThemeEdModeVisualNodeTool::RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport) {
	Super::RemoveOverlayWidgets(InToolkit, InViewport);

	if (ToolShutdownViewportOverlayWidget.IsValid()) {
		InToolkit->GetToolkitHost()->RemoveViewportOverlayWidget(ToolShutdownViewportOverlayWidget.ToSharedRef(), InViewport);
		ToolShutdownViewportOverlayWidget = nullptr;
	}
	
	if (ToolOverlayWidget.IsValid()) {
		InToolkit->GetToolkitHost()->RemoveViewportOverlayWidget(ToolOverlayWidget.ToSharedRef(), InViewport);
		ToolOverlayWidget = nullptr;
	}
}

void UDungeonThemeEdModeVisualNodeTool::OnSelectedActorChanged() {
	Super::OnSelectedActorChanged();

	// TODO: Remove me and refactor
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	SetActorBeingEdited(ActorSelection.GetSelectedActor());
}

void UDungeonThemeEdModeVisualNodeTool::OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes) {
	Super::OnSelectedNodesChanged(SelectedNodes);

	//if (SelectedNodes.Contains(GetActorSelection().GetSelectedActorThemeNode())) {
	//	EdMode->RequestActivateSelectTool();
	//}
}

void UDungeonThemeEdModeVisualNodeTool::OnAssetDragDrop(UObject* InAssetObject) {
	Super::OnAssetDragDrop(InAssetObject);
	
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	UEdGraphNode_DungeonThemeActorBase* ActiveThemeNode = ActorSelection.GetSelectedActorThemeNode();
	if (InAssetObject && EdMode.IsValid() && ActiveThemeNode) {
		if (UEdGraphNode_DungeonThemeBase* ParentNode = FDungeonArchitectThemeEditorUtils::GetParentNode(ActiveThemeNode)) {
			TSharedPtr<SGraphEditor> GraphEditor = EdMode->GetThemeGraphEditor()->GetGraphEditor();
			if (UEdGraphNode_DungeonThemeActorBase* NewVisualNode = FDungeonArchitectThemeEditorUtils::CreateVisualNodeUnderNode(InAssetObject, ParentNode, 0)) {
				NewVisualNode->ConsumeOnAttach = false;
				
				// Recompile the graph
				{
					TArray<FDungeonThemeGraphBuildError> CompileErrors;
					if (!EdMode->CompileThemeGraph(CompileErrors)) {
						// Failed to compile theme graph
						UE_LOG(LogThemeEdMode, Error, TEXT("Failed to compile ThemeGraph"));
					}
				}

				// Try to adjust the layout of the marker tree on the next frame, after the new node has been built in the graph editor
				TArray<UEdGraphNode_DungeonThemeMarker*> ParentMarkers = FDungeonArchitectThemeEditorUtils::FindParentMarkers(ActiveThemeNode);
				if (ParentMarkers.Num() > 0) {
					EdMode->PerformGraphLayoutNextFrame();
				}
				
				// Rebuild the dungeon synchronously, this makes sure our new actors are spawned after build executes
				FDungeonBuildSettings BuildSettings;
				
				// Don't re-build the layout. just spawn in the items from the updated theme graph
				BuildSettings.StartPhase = EDungeonBuildGraphPhase::SpawnItems;		

				// Synchronous build
				BuildSettings.bOverrideFrameBuildTimeMs = true;
				BuildSettings.FrameBuildTimeMs = 0;
				
				EdMode->BuildActiveDungeon(BuildSettings);

				AActor* NewSpawnedActor{};

				// Search for the mesh spawned in this marker that has the visual node id
				if (ActorSelection.IsCachedWorldMarkerValid()) {
					NewSpawnedActor = FDungeonArchitectThemeEditorUtils::FindActorSpawnedByMarker(ActorSelection.GetCachedWorldMarker().Id, EdMode->GetActiveDungeon(), NewVisualNode);
				}

				if (NewSpawnedActor) {
					SetActorBeingEdited(NewSpawnedActor);
				}
			}
		}
		
		GEditor->SelectNone(true, true, false);
		GEditor->SelectActor(ActorBeingEdited.Get(), true, true);
	}
}

bool UDungeonThemeEdModeVisualNodeTool::ProcessDeleteSelectedActor() {
	bool bProcessed = Super::ProcessDeleteSelectedActor();

	if (bProcessed && EdMode.IsValid()) {
		EdMode->RequestActivateSelectTool();
	}
	
	return bProcessed;
}

void UDungeonThemeEdModeVisualNodeTool::RefreshSelectedActor() {
	// Search for the mesh spawned in this marker that has the visual node id
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	if (ActorSelection.IsCachedWorldMarkerValid()) {
		if (AActor* MarkerSpawnedActor = FDungeonArchitectThemeEditorUtils::FindActorSpawnedByMarker(ActorSelection.GetCachedWorldMarker().Id, EdMode->GetActiveDungeon(), ActorSelection.GetSelectedActorThemeNode())) {
			SetActorBeingEdited(MarkerSpawnedActor);
			
			GEditor->SelectNone(true, true, false);
			GEditor->SelectActor(ActorBeingEdited.Get(), true, true);
		}
	}
}

void UDungeonThemeEdModeVisualNodeTool::OnDungeonBuildComplete() {
	Super::OnDungeonBuildComplete();

	RefreshSelectedActor();
}

void UDungeonThemeEdModeVisualNodeTool::OnNodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent) {
	Super::OnNodePropertyChanged(Action, InChangeEvent);
	FName PropertyName = InChangeEvent.Property ? InChangeEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UEdGraphNode_DungeonThemeActorBase, Probability)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UEdGraphNode_DungeonThemeActorBase, ConsumeOnAttach)) {
		// The probability of the node changed, exit this mode and switch back to the select tool
		if (EdMode.IsValid()) {
			EdMode->RequestActivateSelectTool();
		}
	}
}

#undef LOCTEXT_NAMESPACE

