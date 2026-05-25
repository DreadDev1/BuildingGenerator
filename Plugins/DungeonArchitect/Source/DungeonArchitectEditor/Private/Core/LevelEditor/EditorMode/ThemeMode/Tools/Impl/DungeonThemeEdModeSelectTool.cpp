//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/Impl/DungeonThemeEdModeSelectTool.h"

#include "Core/Common/Slate/SDADraggableToolOverlayWidget.h"
#include "Core/Common/Utils/AssetThumbnailCache.h"
#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"

#include "EditorViewportclient.h"
#include "Materials/MaterialInterface.h"
#include "SPrimaryButton.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Text/SRichTextBlock.h"

const FString UDungeonThemeEdModeSelectTool::ToolIdentifier = "DungeonThemeEdModeSelectTool";

#define LOCTEXT_NAMESPACE "DungeonThemeEdModeSelectTool"

///////////////////////////// UDungeonThemeEdModeSelectTool /////////////////////////////
void UDungeonThemeEdModeSelectTool::AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport) {
	Super::AddOverlayWidgets(InToolkit, InViewport);
	
	ToolOverlayWidget = SNew(SDADraggableToolOverlayWidget)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ToolTitle", "Select Tool"))
				.TextStyle(FDungeonArchitectStyle::Get(), "ThemeEdMode.Tool.Title")
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([this]() {
					const FDungeonThemeEdModeActorSelection Selection = GetActorSelection();
					if (!Selection.GetSelectedActorThemeNode()) {
						return EVisibility::Collapsed;
					}
					return EVisibility::Visible;
				})
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(ThumbnailWidgetHost, SBox)
						.WidthOverride(64)
						.HeightOverride(64)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4)
					.AutoWidth()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SRichTextBlock)
							.DecoratorStyleSet(&FDungeonArchitectStyle::Get())
							.TextStyle(FDungeonArchitectStyle::Get(), "DA.RichText.Normal")
							.Text_UObject(this, &UDungeonThemeEdModeSelectTool::GetOverlayPrimaryMessage)
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SRichTextBlock)
							.DecoratorStyleSet(&FDungeonArchitectStyle::Get())
							.TextStyle(FDungeonArchitectStyle::Get(), "DA.RichText.Normal")
							.Text_UObject(this, &UDungeonThemeEdModeSelectTool::GetOverlaySecondaryMessage)
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SPrimaryButton)
							.Text(LOCTEXT("EditOverlayButton", "Edit Transform"))
							.ToolTipText(LOCTEXT("EditOverlayButtonTooltip", "Edit the transform of the"))
							.OnClicked_Lambda([this]() {
								if (EdMode.IsValid()) {
									const FDungeonThemeEdModeActorSelection Selection = EdMode->GetActorSelection();
									EdMode->RequestActivateVisualNodeEdTool(Selection.GetSelectedActorThemeNode());
								}
								return FReply::Handled();
							})
						]
					]
				]
			]
		];
	
	InToolkit->GetToolkitHost()->AddViewportOverlayWidget(ToolOverlayWidget.ToSharedRef(), InViewport);
}

void UDungeonThemeEdModeSelectTool::RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport) {
	Super::RemoveOverlayWidgets(InToolkit, InViewport);
	
	if (ToolOverlayWidget.IsValid()) {
		InToolkit->GetToolkitHost()->RemoveViewportOverlayWidget(ToolOverlayWidget.ToSharedRef(), InViewport);
		ToolOverlayWidget = nullptr;
	}
}

UMaterialInterface* UDungeonThemeEdModeSelectTool::GetPostProcessMaterialTemplate() const {
	return LoadObject<UMaterialInterface>(nullptr,
		TEXT("/DungeonArchitect/Core/Editors/ThemeEditor/EdMode/Tools/SelectTool/M_DA_ThemeEdMode_PostProcess_ToolSelect_Inst"));
}

FText UDungeonThemeEdModeSelectTool::GetOverlayPrimaryMessage() const {
	TArray<UObject*> ThumbnailObjects;
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	if (ActorSelection.GetSelectedActorThemeNode()) {
		ThumbnailObjects = ActorSelection.GetSelectedActorThemeNode()->GetThumbnailAssetObjects().FilterByPredicate([](UObject* Object) {
			return IsValid(Object);
		});
	}
	
	const FString AssetName = ThumbnailObjects.Num() > 0
 		? "<DA.RichText.Strong>" + ThumbnailObjects[0]->GetName() + "</>"
		: "None";
	return FText::Format(LOCTEXT("OverlayPrimaryText", "Asset: {0}"), FText::FromString(AssetName));
}

FText UDungeonThemeEdModeSelectTool::GetOverlaySecondaryMessage() const {
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	UEdGraphNode_DungeonThemeMarker* MarkerNode = ActorSelection.GetParentMarkerNode();
	FString MarkerName = MarkerNode
		? "<DA.RichText.Strong>" + MarkerNode->MarkerName + "</>"
		: "None";
	return FText::Format(LOCTEXT("OverlaySecondaryText", "Marker: {0}"), FText::FromString(MarkerName));
}

void UDungeonThemeEdModeSelectTool::Setup() {
	Super::Setup();

	static const FIntPoint ThumbnailSize = FIntPoint(64, 64);
	ThumbnailCacheManager = MakeShareable(new FDAAssetThumbnailCacheManager(ThumbnailSize));

	if (GEditor) {
		const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(ActorSelection.GetSelectedActor(), true, true);
	}
}

void UDungeonThemeEdModeSelectTool::Shutdown(EToolShutdownType ShutdownType) {
	Super::Shutdown(ShutdownType);

	ThumbnailCacheManager = nullptr;

	if (EdMode.IsValid()) {
		TArray<FDungeonThemeGraphBuildError> CompileErrors;
		EdMode->CompileThemeGraph(CompileErrors);
	}
}

bool UDungeonThemeEdModeSelectTool::IsActorSelectionAllowed(AActor* InActor, bool bInSelection) {
	return Super::IsActorSelectionAllowed(InActor, bInSelection);
}

void UDungeonThemeEdModeSelectTool::OnSelectedActorChanged() {
	Super::OnSelectedActorChanged();
	
	UpdateNodeThumbnailWidget();
}

void UDungeonThemeEdModeSelectTool::OnSelectedNodesChanged(const TSet<UObject*>& InSelectedNodes) {
	Super::OnSelectedNodesChanged(InSelectedNodes);

	UEdGraphNode_DungeonThemeActorBase* NewSelectedNode = InSelectedNodes.Num() == 1
			? Cast<UEdGraphNode_DungeonThemeActorBase>(InSelectedNodes.Array()[0])
			: nullptr;

	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	if (NewSelectedNode != ActorSelection.GetSelectedActorThemeNode()) {
		if (EdMode.IsValid()) {
			FDungeonThemeEdModeActorSelection& Selection = EdMode->GetMutableActorSelection();
			Selection.Initialize(nullptr, NewSelectedNode, EdMode->GetActiveDungeon());
		}
		if (GEditor) {
			GEditor->SelectNone(true, true, false);
		}
	}
	
	UpdateNodeThumbnailWidget();

	EdMode->ShowObjectDetails(InSelectedNodes.Array(), false);
}

void UDungeonThemeEdModeSelectTool::UpdateNodeThumbnailWidget() {
	const FDungeonThemeEdModeActorSelection ActorSelection = GetActorSelection();
	// Update the overlay widget
	if (ThumbnailWidgetHost.IsValid()) {
		TSharedPtr<SWidget> NodePreviewWidget;
		if (ActorSelection.GetSelectedActorThemeNode()) {
			NodePreviewWidget = FDungeonArchitectThemeEditorUtils::CreateNodeThumbWidget(ActorSelection.GetSelectedActorThemeNode(), ThumbnailCacheManager);
		}
		
		ThumbnailWidgetHost->SetContent(NodePreviewWidget.IsValid()
			? NodePreviewWidget.ToSharedRef()
			: SNullWidget::NullWidget);
	}
}

void UDungeonThemeEdModeSelectTool::OnActorDoubleClicked(AActor* InActor) {
	Super::OnActorDoubleClicked(InActor);
	
	if (EdMode.IsValid()) {
		EdMode->RequestActivateVisualNodeEdTool(nullptr);
	}
}

void UDungeonThemeEdModeSelectTool::OnThemeNodeDoubleClicked(UEdGraphNode_DungeonThemeBase* InThemeNode) {
	Super::OnThemeNodeDoubleClicked(InThemeNode);
	
	if (EdMode.IsValid()) {
		if (UEdGraphNode_DungeonThemeActorBase* VisualNode = Cast<UEdGraphNode_DungeonThemeActorBase>(InThemeNode)) {
			EdMode->RequestActivateVisualNodeEdTool(VisualNode);
		}
		else if (UEdGraphNode_DungeonThemeMarker* MarkerNode = Cast<UEdGraphNode_DungeonThemeMarker>(InThemeNode)) {
			EdMode->RequestActivateMarkerNodeEdTool(MarkerNode);
		}
	}
}


#undef LOCTEXT_NAMESPACE

