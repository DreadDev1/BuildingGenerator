//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeMarkerNodeTool.h"

#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/Common/Slate/SDADraggableToolOverlayWidget.h"
#include "Core/Common/Slate/SDAToolShutdownOverlay.h"
#include "Core/Common/Utils/DungeonScenePostProcess.h"
#include "Core/Dungeon.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphEditorImpl.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"
#include "Core/Settings/DungeonArchitectProjectUserSettings.h"

#include "IAssetViewport.h"
#include "LevelEditorViewport.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "DungeonThemeEdModeMarkerTool"

class UDungeonArchitectProjectUserSettings;
const FString UDungeonThemeEdModeMarkerNodeTool::ToolIdentifier = "DungeonThemeEdModeMarkerNodeTool";

void UDungeonThemeEdModeMarkerNodeTool::Setup() {
	Super::Setup();
	
	SetToolDisplayName(LOCTEXT("ToolDisplayName", "Marker Node Editor"));
	if (!MarkerNode.IsValid()) {
		return;
	}
	
	if (GEditor) {
		GEditor->SelectNone(true, true, false);
	}

	ZoomInOnNearestMarker();	
	
	if (EdMode.IsValid()) {
		// Disable the theme graph editor as we don't want the graph state to change in this tool
		TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditor = EdMode->GetThemeGraphEditor();
		if (ThemeGraphEditor.IsValid()) {
			ThemeGraphEditor->SetIsEditable(false);
		}

		// Zoom in on the marker node
		bNodeSelectionGuard = true;
		ThemeGraphEditor->SelectAndFocusOnNode(MarkerNode.Get());
		bNodeSelectionGuard = false;

		// Create visualization marker rules. They tell us how to draw the different shapes on the markers
		if (ADungeon* ActiveDungeon = EdMode->GetActiveDungeon()) {
			if (UDungeonBuilder* Builder = ActiveDungeon->GetBuilder()) {
				Builder->CreateMarkerVisualizationRules(MarkerVisualizationRules);
			}
		}
	}

}

void UDungeonThemeEdModeMarkerNodeTool::Shutdown(EToolShutdownType ShutdownType) {
	Super::Shutdown(ShutdownType);

	// Enable the graph editor
	if (EdMode.IsValid()) {
		TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditor = EdMode->GetThemeGraphEditor();
		if (ThemeGraphEditor.IsValid()) {
			ThemeGraphEditor->SetIsEditable(true);
		}
	}
}

bool UDungeonThemeEdModeMarkerNodeTool::IsActorSelectionAllowed(AActor* Actor, bool bInSelection) {
	if (!bInSelection) {
		// Allow deselection
		return true;
	}
	
	return false;
}

void UDungeonThemeEdModeMarkerNodeTool::Render(IToolsContextRenderAPI* RenderAPI) {
	Super::Render(RenderAPI);

	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	
	if (bMarkerInfoValid) {
		MarkerVisualizer.RenderMarker(RenderAPI->GetPrimitiveDrawInterface(), MarkerInfo.MarkerName,
				MarkerInfo.Transform, MarkerVisualizationRules);
	}
}

void UDungeonThemeEdModeMarkerNodeTool::AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport) {
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
				.Text(LOCTEXT("ToolTitle", "Marker Tool"))
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
						.Value_UObject(this, &UDungeonThemeEdModeMarkerNodeTool::GetHighlightSaturation)
						.OnValueChanged_UObject(this, &UDungeonThemeEdModeMarkerNodeTool::SetHighlightSaturation)
						.OnMouseCaptureEnd_UObject(this, &UDungeonThemeEdModeMarkerNodeTool::SaveHighlightSaturationUserSetting)
					]
				]
			]
		];
	InToolkit->GetToolkitHost()->AddViewportOverlayWidget(ToolOverlayWidget.ToSharedRef(), InViewport);
}

void UDungeonThemeEdModeMarkerNodeTool::RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport) {
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

void UDungeonThemeEdModeMarkerNodeTool::OnSelectedNodesChanged(const TSet<UObject*>& InSelectedNodes) {
	Super::OnSelectedNodesChanged(InSelectedNodes);
	if (bNodeSelectionGuard) {
		return;
	}

	TArray<UObject*> SelectedNodes = InSelectedNodes.Array();
	UEdGraphNode_DungeonThemeMarker* NewSelectedMarkerNode = SelectedNodes.Num() > 0 ? Cast<UEdGraphNode_DungeonThemeMarker>(SelectedNodes[0]) : nullptr;
	if (NewSelectedMarkerNode && NewSelectedMarkerNode == MarkerNode) {
		// No change in the marker node
		return;
	}
	
	if (NewSelectedMarkerNode && NewSelectedMarkerNode != MarkerNode) {
		SetMarkerNode(NewSelectedMarkerNode);
		ZoomInOnNearestMarker();
	}
	else {
		// Switch back to the select tool
		if (EdMode.IsValid()) {
			EdMode->RequestActivateSelectTool();
		}
	}
}

UMaterialInterface* UDungeonThemeEdModeMarkerNodeTool::GetPostProcessMaterialTemplate() const {
	return LoadObject<UMaterialInterface>(nullptr,
		TEXT("/DungeonArchitect/Core/Editors/ThemeEditor/EdMode/Tools/MarkerTool/M_DA_ThemeEdMode_PostProcess_ToolMarkerEdit_Inst"));
}

void UDungeonThemeEdModeMarkerNodeTool::SetMarkerNode(UEdGraphNode_DungeonThemeMarker* InMarkerNode) {
	MarkerNode = InMarkerNode;
}

float UDungeonThemeEdModeMarkerNodeTool::GetHighlightSaturation() const {
	return VisualHighlightSaturation;
}

void UDungeonThemeEdModeMarkerNodeTool::SetHighlightSaturation(float InValue) {
	VisualHighlightSaturation = InValue;
	if (PostProcessor) {
		if (UMaterialInstanceDynamic* MID = PostProcessor->GetPostProcessMaterial()) {
			MID->SetScalarParameterValue("Saturation", VisualHighlightSaturation);
		}
	}
}

void UDungeonThemeEdModeMarkerNodeTool::OnAssetDragDrop(UObject* InAssetObject) {
	Super::OnAssetDragDrop(InAssetObject);

	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	UEdGraphNode_DungeonThemeActorBase* ActiveThemeNode = ActorSelection.GetSelectedActorThemeNode();
	if (InAssetObject && EdMode.IsValid() && MarkerNode.IsValid()) {
		TSharedPtr<SGraphEditor> GraphEditor = EdMode->GetThemeGraphEditor()->GetGraphEditor();
		if (UEdGraphNode_DungeonThemeActorBase* NewVisualNode = FDungeonArchitectThemeEditorUtils::CreateVisualNodeUnderNode(InAssetObject, MarkerNode.Get(), 0)) {
			NewVisualNode->ConsumeOnAttach = false;
			
			// Recompile the graph
			{
				TArray<FDungeonThemeGraphBuildError> CompileErrors;
				if (!EdMode->CompileThemeGraph(CompileErrors)) {
					// Failed to compile theme graph
					UE_LOG(LogThemeEdMode, Error, TEXT("Failed to compile ThemeGraph"));
				}
			}

			EdMode->PerformGraphLayoutNextFrame();
				
			// Rebuild the dungeon synchronously, this makes sure our new actors are spawned after build executes
			FDungeonBuildSettings BuildSettings;
			BuildSettings.StartPhase = EDungeonBuildGraphPhase::SpawnItems;		// Don't re-build the layout. just spawn in the items from the updated theme graph
			BuildSettings.bOverrideFrameBuildTimeMs = true;
			BuildSettings.FrameBuildTimeMs = 0;		// Synchronous build
			EdMode->BuildActiveDungeon(BuildSettings);

			AActor* NewSpawnedActor{};

			// Search for the mesh spawned in this marker that has the visual node id
			if (ActorSelection.IsCachedWorldMarkerValid()) {
				NewSpawnedActor = FDungeonArchitectThemeEditorUtils::FindActorSpawnedByMarker(ActorSelection.GetCachedWorldMarker().Id, EdMode->GetActiveDungeon(), NewVisualNode);
			}

			if (NewSpawnedActor && GEditor) {
				GEditor->SelectNone(true, true, false);
				GEditor->SelectActor(NewSpawnedActor, true, true);
				EdMode->RequestActivateVisualNodeEdTool(NewVisualNode);
			}
		}
	}
}
 
void UDungeonThemeEdModeMarkerNodeTool::SaveHighlightSaturationUserSetting() const {
	GetMutableDefault<UDungeonArchitectProjectUserSettings>()->SetThemeEdModeActorHighlightSaturation(VisualHighlightSaturation);
}

void UDungeonThemeEdModeMarkerNodeTool::ZoomInOnNearestMarker() {
	float BestDistance = MAX_flt;
	const FVector ViewLocation = GCurrentLevelEditingViewportClient
		? GCurrentLevelEditingViewportClient->GetViewLocation()
		: FVector::Zero();

	bMarkerInfoValid = false;
	MarkerInfo = {};
	
	if (EdMode.IsValid() && MarkerNode.IsValid()) {
		FString MarkerName = MarkerNode->MarkerName;
		if (ADungeon* ActiveDungeon = EdMode->GetActiveDungeon()) {
			if (UDungeonModel* DungeonModel = ActiveDungeon->GetModel()) {
				for (const FDungeonMarkerInstance& Marker : DungeonModel->WorldMarkers) { 
					if (Marker.MarkerName == MarkerName) {
						float DistanceSq = (Marker.Transform.GetLocation() - ViewLocation).SizeSquared();
						if (!bMarkerInfoValid || DistanceSq < BestDistance) {
							MarkerInfo = Marker;
							bMarkerInfoValid = true;
							BestDistance = DistanceSq;
						}
					}
				} 
			}
		}
	}

	if (bMarkerInfoValid && GCurrentLevelEditingViewportClient) {
		float Height = 400;
		float Radius = 400;
		FVector Center = MarkerInfo.Transform.GetLocation() + FVector(0, 0, Height * 0.5f);
		FVector Extent = FVector(Radius, Radius, Height * 0.5f);
		FBox Bounds(Center - Extent, Center + Extent);
		GCurrentLevelEditingViewportClient->FocusViewportOnBox(Bounds);
	}

	// Update the actor selection with the cached marker info
	FDungeonThemeEdModeActorSelection& Selection = EdMode->GetMutableActorSelection();
	Selection.Initialize(MarkerNode.Get(), MarkerInfo);
}

#undef LOCTEXT_NAMESPACE

