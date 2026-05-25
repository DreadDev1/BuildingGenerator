//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolkit.h"

#include "Builders/Forge/DungeonForgeBuilder.h"
#include "Builders/Forge/DungeonForgeConfig.h"
#include "Core/Dungeon.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommands.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommon.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorMode.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeCommands.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolbar.h"
#include "Core/Utils/Debug/DungeonDebugVisualizer.h"
#include "Frameworks/Forge/Common/Utils/DungeonForgeDebugUtils.h"
#include "Frameworks/Forge/DungeonForgeAsset.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNode.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphSchema.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "EdGraphUtilities.h"
#include "EditorModeManager.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "InteractiveToolManager.h"
#include "LevelEditorViewport.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "SPrimaryButton.h"
#include "STransformGizmoNumericalUIOverlay.h"
#include "ScopedTransaction.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Toolkits/AssetEditorModeUILayer.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "DungeonForgeModeToolkit"


const FName FDungeonForgeTabsID::DetailsView(TEXT("DetailsViewTab"));

FDungeonForgeEditorModeToolkit::FDungeonForgeEditorModeToolkit()
{
	bUsesToolkitBuilder = true;
}

FDungeonForgeEditorModeToolkit::~FDungeonForgeEditorModeToolkit() {
	if (FBaseToolkit::IsHosted())
	{
		if (GizmoNumericalUIOverlayWidget.IsValid())
		{
			FBaseToolkit::GetToolkitHost()->RemoveViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef());
			GizmoNumericalUIOverlayWidget.Reset();
		}
	}
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolNotificationMessage.RemoveAll(this);
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolWarningMessage.RemoveAll(this);
}

void FDungeonForgeEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	GetToolkitHost()->OnActiveViewportChanged().AddSP(this, &FDungeonForgeEditorModeToolkit::OnActiveViewportChanged);

	ModeWarningArea = SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f)));
	ModeWarningArea->SetText(FText::GetEmpty());
	ModeWarningArea->SetVisibility(EVisibility::Collapsed);

	
	ToolWarningArea = SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f)));
	ToolWarningArea->SetText(FText::GetEmpty());
	
	RegisterPalettes();

	SAssignNew(ForgeGraphHost, SBorder);
	UpdateGraphWidgetHost();
	
	ToolkitBuilder->SetCategoryButtonLabelVisibility(EVisibility::Collapsed);
	
	SAssignNew(ToolkitWidget, SBorder)
		.HAlign(HAlign_Fill)
		.Padding(0)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			ToolkitBuilder->GenerateWidget()->AsShared()
		];

	ActiveToolName = FText::GetEmpty();
	ActiveToolMessage = FText::GetEmpty();

	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolNotificationMessage.AddSP(this, &FDungeonForgeEditorModeToolkit::PostNotification);
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolWarningMessage.AddSP(this, &FDungeonForgeEditorModeToolkit::PostWarning);

	MakeToolShutdownOverlayWidget();

	// Note that the numerical UI widget should be created before making the selection palette so that
	// it can be bound to the buttons there.
	GizmoNumericalUIOverlayWidget = SNew(STransformGizmoNumericalUIOverlay)
		.DefaultLeftPadding(15)
		// Position above the little axis visualization
		.DefaultVerticalPadding(75)
		.bPositionRelativeToBottom(true);
	
	GetToolkitHost()->AddViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef());
}

void FDungeonForgeEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	FDungeonForgeEditorModeCommands::GetCategories().GenerateKeyArray(PaletteNames);
}


void FDungeonForgeEditorModeToolkit::CustomizeModeDetailsViewArgs(FDetailsViewArgs& ArgsInOut) {
	FModeToolkit::CustomizeModeDetailsViewArgs(ArgsInOut);
	ArgsInOut.NotifyHook = this;
}

FName FDungeonForgeEditorModeToolkit::GetToolkitFName() const
{
	return FName("DungeonForgeEditorMode");
}

FText FDungeonForgeEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "Dungeon Forge Toolkit");
}

TSharedPtr<SWidget> FDungeonForgeEditorModeToolkit::GetInlineContent() const {
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.FillHeight(1.0f)
	.VAlign(VAlign_Fill)
	[
		ToolkitWidget.ToSharedRef()
	];
}

void FDungeonForgeEditorModeToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) {
	if (!PropertyThatChanged) {
		return;
	}

	// const FName PropertyName = PropertyThatChanged->GetFName();
	
}

void FDungeonForgeEditorModeToolkit::PostNotification(const FText& Message) {
	ClearNotification();

	ActiveToolMessage = Message;

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		ActiveToolMessageHandle = GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PushStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessage);
	}
}

void FDungeonForgeEditorModeToolkit::ClearNotification() {
	ActiveToolMessage = FText::GetEmpty();

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PopStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessageHandle);
	}
	ActiveToolMessageHandle.Reset();
}

void FDungeonForgeEditorModeToolkit::PostWarning(const FText& Message) {
	ToolWarningArea->SetText(Message);
	ToolWarningArea->SetVisibility(EVisibility::Visible);
}

void FDungeonForgeEditorModeToolkit::ClearWarning() {
	ToolWarningArea->SetText(FText());
	ToolWarningArea->SetVisibility(EVisibility::Collapsed);
}

void FDungeonForgeEditorModeToolkit::SetSelectedObject(UObject* InObject) const {
	if (ModeDetailsView.IsValid()) {
		ModeDetailsView->SetObject(InObject);
	}
}

void FDungeonForgeEditorModeToolkit::SetSelectedObjects(TArray<UObject*> InObjects) const {
	if (ModeDetailsView.IsValid()) {
		ModeDetailsView->SetObjects(InObjects);
	}
}

void FDungeonForgeEditorModeToolkit::RequestModeUITabs() {
	FModeToolkit::RequestModeUITabs();

	if (TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin())
	{
		TSharedPtr<FWorkspaceItem> MenuModeCategoryPtr = ModeUILayerPtr->GetModeMenuCategory();

		if(!MenuModeCategoryPtr)
		{ 
			return;
		}

		/*
		// Graph Editor Tab
		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(FDungeonForgeTabsID::GraphEditor);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(FDungeonForgeTabsID::GraphEditor, FOnSpawnTab::CreateSP(SharedThis(this), &FDungeonForgeEditorModeToolkit::CreateGraphTab))
			.SetDisplayName(LOCTEXT("DungeonGraphTabLabel", "Dungeon Graph"))
			.SetTooltipText(LOCTEXT("DungeonGraphTabTooltipText", "Open the Dungeon Graph tab, to design your dungeon layout"))
			.SetGroup(MenuModeCategoryPtr.ToSharedRef())
			.SetIcon(GetEditorModeIcon());
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(FDungeonForgeTabsID::GraphEditor, FVector2D(300, 325));
		*/

		/*
		// Actions Palette Tab
		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(FDungeonForgeTabsID::ActionsPalette);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(FDungeonForgeTabsID::ActionsPalette, FOnSpawnTab::CreateSP(SharedThis(this), &FDungeonForgeEditorModeToolkit::CreateActionsPaletteTab))
			.SetDisplayName(LOCTEXT("ActionsPaletteTabLabel", "Actions Palette"))
			.SetTooltipText(LOCTEXT("ActionsPaletteTabTooltipText", "List of nodes you can drag drop on to the graph editor"))
			.SetGroup(MenuModeCategoryPtr.ToSharedRef())
			.SetIcon(GetEditorModeIcon());
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(FDungeonForgeTabsID::ActionsPalette, FVector2D(300, 325));
		*/
		
		// Details View Tab
		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(FDungeonForgeTabsID::DetailsView);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(FDungeonForgeTabsID::DetailsView, FOnSpawnTab::CreateSP(SharedThis(this), &FDungeonForgeEditorModeToolkit::CreateDetailsViewTab))
			.SetDisplayName(LOCTEXT("DetailsViewTabLabel", "Forge Properties"))
			.SetTooltipText(LOCTEXT("DetailsViewTabTooltipText", "The properties of the dungeon forge objects"))
			.SetGroup(MenuModeCategoryPtr.ToSharedRef())
			.SetIcon(GetEditorModeIcon());
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(FDungeonForgeTabsID::DetailsView, FVector2D(300, 325));

		ModeUILayerPtr->ToolkitHostShutdownUI().BindSP(this, &FDungeonForgeEditorModeToolkit::UnregisterAndRemoveFloatingTabs);
	}
}

TSharedRef<SDockTab> FDungeonForgeEditorModeToolkit::CreateDetailsViewTab(const FSpawnTabArgs& Args) {
	TSharedPtr<SDockTab> CreatedTab = SNew(SDockTab)
	[
		ModeDetailsView.ToSharedRef()
	];

	DetailsViewTab = CreatedTab;
	return CreatedTab.ToSharedRef();
}

UDungeonForgeEditorMode* FDungeonForgeEditorModeToolkit::GetForgeEditorMode() const {
	return Cast<UDungeonForgeEditorMode>(GetScriptableEditorMode().Get());
}

class FDungeonForgeToolkitBuilder : public FToolkitBuilder {
public:
	explicit FDungeonForgeToolkitBuilder(
		FName ToolbarCustomizationName,
		TSharedPtr<FUICommandList> InToolkitCommandList,
		TSharedPtr<FToolkitSections> InToolkitSections)
			: FToolkitBuilder(ToolbarCustomizationName, InToolkitCommandList, InToolkitSections) {}
	
	virtual void UpdateWidget() override {
		FToolkitBuilder::UpdateWidget();
		CategoryToolbarVisibility = EVisibility::Collapsed;
	}
};

void FDungeonForgeEditorModeToolkit::RegisterPalettes() {
	UDungeonForgeEditorMode* DungeonForgeToolsMode = GetForgeEditorMode();
	
	ModeDetailsView->SetIsPropertyVisibleDelegate(
		FIsPropertyVisible::CreateLambda(
			[](const FPropertyAndParent& InPropertyAndParent) -> bool
			{
				static const FString Material = FString(TEXT("Material"));
				static const FName Category = FName(TEXT("Category"));
				const FString CategoryMeta = InPropertyAndParent.Property.GetMetaData(Category);
				return CategoryMeta != Material;
			}
	));

	ToolkitSections = MakeShared<FToolkitSections>();

	ToolkitSections->ModeWarningArea = ModeWarningArea;
	ToolkitSections->ToolWarningArea = ToolWarningArea;
	ToolkitSections->DetailsView = ModeDetailsView;
		
	ToolkitBuilder = MakeShared<FDungeonForgeToolkitBuilder>(
		DungeonForgeToolsMode->GetModeInfo().ToolbarCustomizationName,
		GetToolkitCommands(),
		ToolkitSections);
	
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands = FDungeonForgeEditorModeCommands::GetCommands();
	TMap<FName, TSharedPtr<FUICommandInfo>> Categories = FDungeonForgeEditorModeCommands::GetCategories();
	
	TSharedPtr<FUICommandInfo> FirstCategoryCommand = nullptr;

	for (const TPair<FName, TSharedPtr<FUICommandInfo>>& Category : Categories)
	{
		if (Category.Value.IsValid())
		{
			TArray<TSharedPtr<FUICommandInfo>> CategoryCommands;

			if (const TArray<TSharedPtr<FUICommandInfo>>* ToolList = Commands.Find(Category.Key)) {
				for (const TSharedPtr<FUICommandInfo>& Tool : *ToolList) {
					if (Tool.IsValid())
					{
						CategoryCommands.Add(Tool);
					}
				}
			}

			if (CategoryCommands.IsEmpty() == false)
			{
				ToolkitBuilder->AddPalette(MakeShared<FToolPalette>(Category.Value.ToSharedRef(), CategoryCommands));

				if (FirstCategoryCommand.IsValid() == false)
				{
					FirstCategoryCommand = Category.Value;
				}
			}
		}
	}

	if (FirstCategoryCommand.IsValid())
	{
		ToolkitBuilder->SetActivePaletteOnLoad(FirstCategoryCommand.Get());
	}

	ToolkitBuilder->UpdateWidget();
}

void FDungeonForgeEditorModeToolkit::UpdateActiveToolProperties() {
	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
	if (CurTool == nullptr)
	{
		return;
	}

	// Before actually changing the detail panel, we need to see where the current keyboard focus is, because
	// if it's inside the detail panel, we'll need to reset it to the detail panel as a whole, else we might
	// lose it entirely when that detail panel element gets destroyed (which would make us unable to receive any
	// hotkey presses until the user clicks somewhere).
	TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (FocusedWidget != ModeDetailsView) 
	{
		// Search upward from the currently focused widget
		TSharedPtr<SWidget> CurrentWidget = FocusedWidget;
		while (CurrentWidget.IsValid())
		{
			if (CurrentWidget == ModeDetailsView)
			{
				// Reset focus to the detail panel as a whole to avoid losing it when the inner elements change.
				FSlateApplication::Get().SetKeyboardFocus(ModeDetailsView);
				break;
			}

			CurrentWidget = CurrentWidget->GetParentWidget();
		}
	}
		
	ModeDetailsView->SetObjects(CurTool->GetToolProperties(true));
}

void FDungeonForgeEditorModeToolkit::InvalidateCachedDetailPanelState(UObject* ChangedObject) {
	ModeDetailsView->InvalidateCachedState();	
}

TObjectPtr<UDungeonForgeEditorGraph> FDungeonForgeEditorModeToolkit::GetForgeEditorGraph() {
	return ForgeEditorGraph.Get();
}

TObjectPtr<UDungeonForgeEditorGraph> FDungeonForgeEditorModeToolkit::GetForgeEditorGraph(UDungeonForgeGraph* InForgeGraph) {
	if (!InForgeGraph) {
		return nullptr;
	}

	if (!IsValid(InForgeGraph->ForgeEditorGraph)) {
		InForgeGraph->ForgeEditorGraph = NewObject<UDungeonForgeEditorGraph>(InForgeGraph, UDungeonForgeEditorGraph::StaticClass(), NAME_None, RF_Transactional | RF_Transient);
		InForgeGraph->ForgeEditorGraph->Schema = UDungeonForgeEditorGraphSchema::StaticClass();
		InForgeGraph->ForgeEditorGraph->InitFromNodeGraph(InForgeGraph);
	}

	return InForgeGraph->ForgeEditorGraph;
}

FText FDungeonForgeEditorModeToolkit::GetToolWarningText() const {
	return LOCTEXT("InactiveToolWarning", "Select a tool to start drawing.\n\nShift click will show tool presets, if available.");
}

void FDungeonForgeEditorModeToolkit::OnActiveViewportChanged(TSharedPtr<IAssetViewport> OldViewport, TSharedPtr<IAssetViewport> NewViewport)
{
	// Only worry about handling this notification if Modeling has an active tool
	if (!ActiveToolName.IsEmpty())
	{
		// Check first to see if this changed because the old viewport was deleted and if not, remove our hud
		if (OldViewport)	
		{
			GetToolkitHost()->RemoveViewportOverlayWidget(ToolShutdownViewportOverlayWidget.ToSharedRef(), OldViewport);

			if (GizmoNumericalUIOverlayWidget.IsValid())
			{
				GetToolkitHost()->RemoveViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef(), OldViewport);
			}
		}

		// Add the hud to the new viewport
		GetToolkitHost()->AddViewportOverlayWidget(ToolShutdownViewportOverlayWidget.ToSharedRef(), NewViewport);

		if (GizmoNumericalUIOverlayWidget.IsValid())
		{
			GetToolkitHost()->AddViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef(), NewViewport);
		}
	}
}

void FDungeonForgeEditorModeToolkit::MakeToolShutdownOverlayWidget() {
	const FSlateBrush* OverlayBrush = FAppStyle::Get().GetBrush("EditorViewport.OverlayBrush");
	/*
	// If there is another mode, it might also have an overlay, and we would like ours to be opaque in that case
	// to draw on top cleanly (e.g., level instance editing mode has an overlay in the same place. Note that level
	// instance mode currently marks itself as not visible despite the overlay, so we shouldn't use IsOnlyVisibleActiveMode)
	if (!GetEditorModeManager().IsOnlyActiveMode(UModelingToolsEditorMode::EM_ModelingToolsEditorModeId))
	{
		OverlayBrush = FDungeonArchitectStyle::Get().GetBrush("ModelingMode.OpaqueOverlayBrush");
	}
	*/

	SAssignNew(ToolShutdownViewportOverlayWidget, SHorizontalBox)

	+SHorizontalBox::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0.0f, 0.0f, 0.f, 15.f))
	[
		SNew(SBorder)
		.BorderImage(OverlayBrush)
		.Padding(8.f)
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SImage)
				.Image_Lambda([this] () { return ActiveToolIcon; })
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock)
				.Text(this, &FDungeonForgeEditorModeToolkit::GetActiveToolDisplayName)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0.0, 0.f, 2.f, 0.f))
			[
				SNew(SPrimaryButton)
				.Text(LOCTEXT("OverlayAccept", "Accept"))
				.ToolTipText(LOCTEXT("OverlayAcceptTooltip", "Accept/Commit the results of the active Tool [Enter]"))
				.OnClicked_Lambda([this]() { GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->EndTool(EToolShutdownType::Accept); return FReply::Handled(); })
				.IsEnabled_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->CanAcceptActiveTool(); })
				.Visibility_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->ActiveToolHasAccept() ? EVisibility::Visible : EVisibility::Collapsed; })
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(2.0, 0.f, 0.f, 0.f))
			[
				SNew(SButton)
				.Text(LOCTEXT("OverlayCancel", "Cancel"))
				.ToolTipText(LOCTEXT("OverlayCancelTooltip", "Cancel the active Tool [Esc]"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]() { GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->EndTool(EToolShutdownType::Cancel); return FReply::Handled(); })
				.IsEnabled_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->CanCancelActiveTool(); })
				.Visibility_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->ActiveToolHasAccept() ? EVisibility::Visible : EVisibility::Collapsed; })
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(2.0, 0.f, 0.f, 0.f))
			[
				SNew(SPrimaryButton)
				.Text(LOCTEXT("OverlayComplete", "Complete"))
				.ToolTipText(LOCTEXT("OverlayCompleteTooltip", "Exit the active Tool [Enter]"))
				.OnClicked_Lambda([this]() { GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->EndTool(EToolShutdownType::Completed); return FReply::Handled(); })
				.IsEnabled_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->CanCompleteActiveTool(); })
				.Visibility_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->CanCompleteActiveTool() ? EVisibility::Visible : EVisibility::Collapsed; })
			]
		]	
	];
}

void FDungeonForgeEditorModeToolkit::SelectAllNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->SelectAllNodes();
	}
}

bool FDungeonForgeEditorModeToolkit::CanSelectAllNodes() const
{
	return GraphEditorWidget.IsValid();
}

void FDungeonForgeEditorModeToolkit::DeleteSelectedNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		UDungeonForgeGraph* ForgeGraph = ForgeEditorGraph->GetForgeGraph();
		check(ForgeEditorGraph.IsValid() && ForgeGraph);

		bool bChanged = false;

		{
			const FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("ForgeEditorDeleteTransactionMessage", "Forge Editor: Delete"), nullptr);
			ForgeEditorGraph->Modify();
		
			TArray<UDungeonForgeGraphNode*> NodesToRemove;

			for (UObject* Object : GraphEditorWidget->GetSelectedNodes())
			{
				if (UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object))
				{
					if (ForgeEditorGraphNode->CanUserDeleteNode())
					{
						UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode->GetForgeNode();
						check(ForgeNode);

						NodesToRemove.Add(ForgeNode);

						ForgeEditorGraphNode->DestroyNode();
						bChanged = true;
					}
				}
				else if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object))
				{
					if (GraphNode->CanUserDeleteNode())
					{
						GraphNode->DestroyNode();
						bChanged = true;
					}
				}
			}

			ForgeEditorGraph->Modify();

			if (bChanged)
			{
				ForgeGraph->RemoveNodes(NodesToRemove);
			}
		}

		if (bChanged)
		{
			GraphEditorWidget->ClearSelectionSet();
			GraphEditorWidget->NotifyGraphChanged();
		}
	}
}

bool FDungeonForgeEditorModeToolkit::CanDeleteSelectedNodes() const
{
	if (GraphEditorWidget.IsValid())
	{
		for (UObject* Object : GraphEditorWidget->GetSelectedNodes())
		{
			UEdGraphNode* GraphNode = CastChecked<UEdGraphNode>(Object);

			if (GraphNode->CanUserDeleteNode())
			{
				return true;
			}
		}
	}

	return false;
}

void FDungeonForgeEditorModeToolkit::CopySelectedNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();

		//TODO: evaluate creating a clipboard object instead of ownership hack
		for (UObject* SelectedNode : SelectedNodes)
		{
			UEdGraphNode* GraphNode = CastChecked<UEdGraphNode>(SelectedNode);
			GraphNode->PrepareForCopying();
		}

		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
		FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

		for (UObject* SelectedNode : SelectedNodes)
		{
			if (UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(SelectedNode))
			{
				ForgeEditorGraphNode->PostCopy();
			}
		}
	}
}

bool FDungeonForgeEditorModeToolkit::CanCopySelectedNodes() const
{
	if (GraphEditorWidget.IsValid())
	{
		for (UObject* Object : GraphEditorWidget->GetSelectedNodes())
		{
			if (UEdGraphNode* GraphNode = CastChecked<UEdGraphNode>(Object))
			{
				if (GraphNode->CanDuplicateNode())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void FDungeonForgeEditorModeToolkit::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedNodes();
}

bool FDungeonForgeEditorModeToolkit::CanCutSelectedNodes() const
{
	return CanCopySelectedNodes() && CanDeleteSelectedNodes();
}

void FDungeonForgeEditorModeToolkit::PasteNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		PasteNodesHere(GraphEditorWidget->GetPasteLocation2f());
	}
}

void FDungeonForgeEditorModeToolkit::PasteNodesHere(const FVector2D& Location)
{
	// TODO: Implement me
}

bool FDungeonForgeEditorModeToolkit::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(ForgeEditorGraph.Get(), ClipboardContent);
}

void FDungeonForgeEditorModeToolkit::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FDungeonForgeEditorModeToolkit::CanDuplicateNodes() const
{
	return CanCopySelectedNodes();
}

FString FDungeonForgeEditorModeToolkit::GetForgeAssetPath() const {
	return ForgeAssetSearchWidgetPath.GetSoftObjectPath().GetAssetPathString();
}

void FDungeonForgeEditorModeToolkit::OnForgeAssetChanged(const FAssetData& InAssetData) {
	ForgeAssetSearchWidgetPath = InAssetData;
}

void FDungeonForgeEditorModeToolkit::SpawnDungeonForgeActor() {
	if (UDungeonForgeEditorMode* ForgeEdMode = GetForgeEditorMode()) {
		ForgeEdMode->SpawnDungeonForgeActor(ForgeAssetSearchWidgetPath);
	}
}

void FDungeonForgeEditorModeToolkit::BindGizmoNumericalUI() const {
	if (ensure(GizmoNumericalUIOverlayWidget.IsValid())) {
		ensure(GizmoNumericalUIOverlayWidget->BindToGizmoContextObject(GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)));
	}
}

void FDungeonForgeEditorModeToolkit::OnAlignTop()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnAlignTop();
	}
}

void FDungeonForgeEditorModeToolkit::OnAlignMiddle()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnAlignMiddle();
	}
}

void FDungeonForgeEditorModeToolkit::OnAlignBottom()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnAlignBottom();
	}
}

void FDungeonForgeEditorModeToolkit::OnAlignLeft()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnAlignLeft();
	}
}

void FDungeonForgeEditorModeToolkit::OnAlignCenter()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnAlignCenter();
	}
}

void FDungeonForgeEditorModeToolkit::OnAlignRight()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnAlignRight();
	}
}

void FDungeonForgeEditorModeToolkit::OnStraightenConnections()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnStraightenConnections();
	}
}

void FDungeonForgeEditorModeToolkit::OnDistributeNodesH()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnDistributeNodesH();
	}
}

void FDungeonForgeEditorModeToolkit::OnDistributeNodesV()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->OnDistributeNodesV();
	}
}

void FDungeonForgeEditorModeToolkit::OnCreateComment()
{
	if (ForgeEditorGraph.IsValid())
	{
		/* TODO: Implememt me
		FForgeEditorGraphSchemaAction_NewComment CommentAction;

		TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(ForgeEditorGraph);
		FVector2D Location = FVector2D::ZeroVector;
		if (GraphEditorPtr)
		{
			Location = GraphEditorPtr->GetPasteLocation();
		}

		CommentAction.PerformAction(ForgeEditorGraph, nullptr, Location);
		*/
	}
}

void FDungeonForgeEditorModeToolkit::HandleBuildButtonClicked() const {
	if (UDungeonForgeEditorMode* DungeonForgeMode = Cast<UDungeonForgeEditorMode>(GetScriptableEditorMode().Get())) {
		DungeonForgeMode->BuildDungeon();
	}
}

void FDungeonForgeEditorModeToolkit::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection) {
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

	if (NewSelection.Num() == 0) {
		if (UDungeonForgeEditorMode* ForgeEdMode = GetForgeEditorMode()) {
			TWeakObjectPtr<UDungeonForgeGraph> ForgeGraph = ForgeEdMode->GetForgeGraph();
			SelectedObjects.Add(ForgeGraph);
		}
	}
	else {
		for (UObject* Object : NewSelection) {
			if (UDungeonForgeEditorGraphNode* ForgeEditorNode = Cast<UDungeonForgeEditorGraphNode>(Object)) {
				if (UDungeonForgeGraphNode* ForgeNode = ForgeEditorNode->GetForgeNode()) {
					SelectedObjects.Add(ForgeNode->GetSettings());
				}
			}
		}
	}

	if (ModeDetailsView.IsValid()) {
		ModeDetailsView->SetObjects(SelectedObjects, true);
	}
}

void FDungeonForgeEditorModeToolkit::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged) {
	
}

void FDungeonForgeEditorModeToolkit::OnNodeDoubleClicked(UEdGraphNode* Node) {
	
}

TSharedRef<SGraphEditor> FDungeonForgeEditorModeToolkit::CreateGraphEditorWidget() {
	GraphEditorCommands = MakeShareable(new FUICommandList);

	const FDungeonForgeEditorCommands& ForgeEditorCommands = FDungeonForgeEditorCommands::Get();
	
	// Froge Graph commands
	GraphEditorCommands->MapAction(
		ForgeEditorCommands.ToggleEnabled,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnToggleEnabled),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanToggleEnabled),
		FGetActionCheckState::CreateSP(this, &FDungeonForgeEditorModeToolkit::GetEnabledCheckState));

	GraphEditorCommands->MapAction(
		ForgeEditorCommands.ToggleDebug,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnToggleDebug),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanToggleDebug),
		FGetActionCheckState::CreateSP(this, &FDungeonForgeEditorModeToolkit::GetDebugCheckState));
	
	// Editing commands
	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::SelectAllNodes),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanSelectAllNodes));

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanDeleteSelectedNodes));

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CopySelectedNodes),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanCopySelectedNodes));

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanCutSelectedNodes));

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::PasteNodes),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanPasteNodes));

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::DuplicateNodes),
		FCanExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::CanDuplicateNodes));

	// Alignment Commands
	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesTop,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnAlignTop)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesMiddle,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnAlignMiddle)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesBottom,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnAlignBottom)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesLeft,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnAlignLeft)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesCenter,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnAlignCenter)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesRight,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnAlignRight)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().StraightenConnections,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnStraightenConnections)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnCreateComment)
	);

	// Distribution Commands
	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().DistributeNodesHorizontally,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnDistributeNodesH)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().DistributeNodesVertically,
		FExecuteAction::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnDistributeNodesV)
	);

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("ForgeGraphEditorCornerText", "Dungeon Forge Graph");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnSelectedNodesChanged);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnNodeTitleCommitted);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FDungeonForgeEditorModeToolkit::OnNodeDoubleClicked);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(ForgeEditorGraph.Get())
		.GraphEvents(InEvents)
		.ShowGraphStateOverlay(false);
}

void FDungeonForgeEditorModeToolkit::UnregisterAndRemoveFloatingTabs() const {
	if (FSlateApplication::IsInitialized()) {
		if (ModeUILayer.IsValid()) {
			TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();

			auto UnregisterTab = [&ModeUILayerPtr](const FName& InTabName) {
				TSharedPtr<SDockTab> Tab = ModeUILayerPtr->GetTabManager()->FindExistingLiveTab(FTabId(InTabName));
				if (Tab.IsValid()) {
					Tab->RequestCloseTab();
				}
				ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(InTabName);
			};

			UnregisterTab(FDungeonForgeTabsID::DetailsView);
		}
	}
}

void FDungeonForgeEditorModeToolkit::OnToggleEnabled() {
	const ECheckBoxState CheckState = GetEnabledCheckState();
	const bool bNewCheckState = !(CheckState != ECheckBoxState::Unchecked);

	// To prevent the changes on the editor node from being in the transaction, we delay reconstruction.
	TArray<FDungeonForgeDeferNodeReconstructScope> DeferredEditorNodes;

	if (GraphEditorWidget.IsValid())
	{
		FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("ForgeEditorToggleEnableTransactionMessage", "Forge Editor: Toggle Enable Nodes"), nullptr);

		UDungeonForgeGraph* ForgeGraph = ForgeEditorGraph.IsValid() ? ForgeEditorGraph->GetForgeGraph() : nullptr;
		if (!ensure(ForgeGraph))
		{
			return;
		}

		ForgeGraph->DisableNotificationsForEditor();

		bool bChanged = false;
		for (UObject* Object : GraphEditorWidget->GetSelectedNodes())
		{
			UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object);
			UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode ? ForgeEditorGraphNode->GetForgeNode() : nullptr;
			UDungeonForgeNodeSettingsInterface* ForgeSettingsInterface = ForgeNode ? ForgeNode->GetSettingsInterface() : nullptr;

			if (!ForgeSettingsInterface || !ForgeSettingsInterface->CanBeDisabled())
			{
				continue;
			}

			if (ForgeSettingsInterface->bEnabled != bNewCheckState)
			{
				DeferredEditorNodes.Emplace(ForgeEditorGraphNode);
				ForgeSettingsInterface->Modify();
				ForgeSettingsInterface->SetEnabled(bNewCheckState);
				bChanged = true;
			}
		}

		ForgeGraph->EnableNotificationsForEditor();

		if (bChanged)
		{
			GraphEditorWidget->NotifyGraphChanged();
		}
		else
		{
			Transaction.Cancel();
		}
	}
}

bool FDungeonForgeEditorModeToolkit::CanToggleEnabled() const {
	if (!GraphEditorWidget.IsValid()) {
		return false;
	}

	for (const UObject* Object : GraphEditorWidget->GetSelectedNodes()) {
		const UDungeonForgeEditorGraphNodeBase* PCGEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object);
		const UDungeonForgeGraphNode* ForgeNode = PCGEditorGraphNode ? PCGEditorGraphNode->GetForgeNode() : nullptr;
		if (!ForgeNode) {
			continue;
		}

		if (ForgeNode->GetSettingsInterface() && ForgeNode->GetSettingsInterface()->CanBeDisabled()) {
			return true;
		}
	}

	// Could not toggle enabled on anything in selection.
	return false;
}

ECheckBoxState FDungeonForgeEditorModeToolkit::GetEnabledCheckState() const {
	if (GraphEditorWidget.IsValid()) {
		bool bAllEnabled = true;
		bool bAnyEnabled = false;

		for (const UObject* Object : GraphEditorWidget->GetSelectedNodes()) {
			const UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object);
			const UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode ? ForgeEditorGraphNode->GetForgeNode() : nullptr;
			const UDungeonForgeNodeSettingsInterface* ForgeNodeSettingsInterface = ForgeNode ? ForgeNode->GetSettingsInterface() : nullptr;

			if (!ForgeNodeSettingsInterface || !ForgeNodeSettingsInterface->CanBeDisabled()) {
				continue;
			}

			bAllEnabled &= ForgeNodeSettingsInterface->bEnabled;
			bAnyEnabled |= ForgeNodeSettingsInterface->bEnabled;
		}

		if (bAllEnabled) {
			return ECheckBoxState::Checked;
		}
		else if (bAnyEnabled) {
			return ECheckBoxState::Undetermined;
		}
	}

	return ECheckBoxState::Unchecked;
}

void FDungeonForgeEditorModeToolkit::OnToggleDebug() {
	const ECheckBoxState CheckState = GetDebugCheckState();
	const bool bNewCheckState = !(CheckState != ECheckBoxState::Unchecked);

	if (GraphEditorWidget.IsValid()) {
		FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("ForgeEditorToggleDebugTransactionMessage", "Forge Editor: Toggle Debug Nodes"), nullptr);

		bool bChanged = false;
		for (UObject* Object : GraphEditorWidget->GetSelectedNodes()) {
			UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object);
			UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode ? ForgeEditorGraphNode->GetForgeNode() : nullptr;
			UDungeonForgeNodeSettingsInterface* ForgeSettingsInterface = ForgeNode ? ForgeNode->GetSettingsInterface() : nullptr;

			if (!ForgeSettingsInterface || !ForgeSettingsInterface->CanBeDebugged()) {
				continue;
			}

			if (ForgeSettingsInterface->bDebug != bNewCheckState) {
				ForgeSettingsInterface->Modify(/*bAlwaysMarkDirty=*/false);
				ForgeSettingsInterface->bDebug = bNewCheckState;
				ForgeNode->OnNodeChangedDelegate.Broadcast(ForgeNode, EDungeonForgeGraphChangeType::Settings);
				bChanged = true;

				// Update the state from the cached context
				if (ADungeon* DungeonActor = GetDungeonActor()) {
					if (UDungeonForgeBuilder* ForgeBuilder = Cast<UDungeonForgeBuilder>(DungeonActor->GetBuilder())) {
						if (const FDungeonForgeContext* NodeContext = ForgeBuilder->GetNodeInstanceContext(ForgeNode->NodeGuid)) {
							FDungeonForgeDebugUtils::UpdateNodeDebugVisualization(NodeContext);
						}
					}
				}
			}
		}

		if (!bChanged) {
			Transaction.Cancel();
		}
	}
}

bool FDungeonForgeEditorModeToolkit::CanToggleDebug() const {
	if (!GraphEditorWidget.IsValid()) {
		return false;
	}

	for (const UObject* Object : GraphEditorWidget->GetSelectedNodes()) {
		const UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object);
		const UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode ? ForgeEditorGraphNode->GetForgeNode() : nullptr;

		if (ForgeNode && ForgeNode->GetSettingsInterface()->CanBeDebugged())
		{
			return true;
		}
	}

	// Could not toggle debug on anything in selection.
	return false;
}

ECheckBoxState FDungeonForgeEditorModeToolkit::GetDebugCheckState() const {
	if (GraphEditorWidget.IsValid())
	{
		bool bAllDebug = true;
		bool bAnyDebug = false;

		for (const UObject* Object : GraphEditorWidget->GetSelectedNodes())
		{
			const UDungeonForgeEditorGraphNodeBase* ForgeEditorGraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(Object);
			const UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode ? ForgeEditorGraphNode->GetForgeNode() : nullptr;
			const UDungeonForgeNodeSettingsInterface* ForgeSettingsInterface = ForgeNode ? ForgeNode->GetSettingsInterface() : nullptr;

			if (!ForgeSettingsInterface || !ForgeSettingsInterface->CanBeDebugged())
			{
				continue;
			}

			bAllDebug &= ForgeSettingsInterface->bDebug;
			bAnyDebug |= ForgeSettingsInterface->bDebug;
		}

		if (bAllDebug)
		{
			return ECheckBoxState::Checked;
		}
		else if (bAnyDebug)
		{
			return ECheckBoxState::Undetermined;
		}
	}

	return ECheckBoxState::Unchecked;
}

FText FDungeonForgeEditorModeToolkit::GetToolPaletteDisplayName(FName InPaletteName) const {
	const TMap<FName, TSharedPtr<FUICommandInfo>>& Categories = FDungeonForgeEditorModeCommands::GetCategories();

	if (const TSharedPtr<FUICommandInfo>* CommandInfo = Categories.Find(InPaletteName))
	{
		return CommandInfo->Get()->GetLabel();
	}

	return FText::FromName(InPaletteName);
}

void FDungeonForgeEditorModeToolkit::OnToolStarted(UInteractiveToolManager* InManager, UInteractiveTool* InTool) {
	FModeToolkit::OnToolStarted(InManager, InTool);
	
	bInActiveTool = true;

	UpdateActiveToolProperties();
	
	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
	CurTool->OnPropertySetsModified.AddSP(this, &FDungeonForgeEditorModeToolkit::UpdateActiveToolProperties);
	CurTool->OnPropertyModifiedDirectlyByTool.AddSP(this, &FDungeonForgeEditorModeToolkit::InvalidateCachedDetailPanelState);

	ActiveToolName = CurTool->GetToolInfo().ToolDisplayName;
	ToolkitBuilder->SetActiveToolDisplayName(ActiveToolName);

	// try to update icon
	FString ActiveToolIdentifier = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveToolName(EToolSide::Left);
	ActiveToolIdentifier.InsertAt(0, ".");
	FName ActiveToolIconName = ISlateStyle::Join(FDungeonForgeEditorModeCommands::Get().GetContextName(), TCHAR_TO_ANSI(*ActiveToolIdentifier));
	ActiveToolIcon = FDungeonArchitectStyle::Get().GetOptionalBrush(ActiveToolIconName);


	GetToolkitHost()->AddViewportOverlayWidget(ToolShutdownViewportOverlayWidget.ToSharedRef());

	// Invalidate all the level viewports so that e.g. hitproxy buffers are cleared
	// (fixes the editor gizmo still being clickable despite not being visible)
	if (GIsEditor)
	{
		for (FLevelEditorViewportClient* Viewport : GEditor->GetLevelViewportClients())
		{
			Viewport->Invalidate();
		}
	}
}

void FDungeonForgeEditorModeToolkit::OnToolEnded(UInteractiveToolManager* InManager, UInteractiveTool* InTool) {
	FModeToolkit::OnToolEnded(InManager, InTool);
	bInActiveTool = false;

	if (IsHosted())
	{
		GetToolkitHost()->RemoveViewportOverlayWidget(ToolShutdownViewportOverlayWidget.ToSharedRef());
	}

	ModeDetailsView->SetObject(nullptr);
	ActiveToolName = FText::GetEmpty();
	if (HasToolkitBuilder())
	{
		ToolkitBuilder->SetActiveToolDisplayName(FText::GetEmpty());		
		ToolkitBuilder->SetActivePaletteCommandsVisibility(EVisibility::Visible);
	}

	ClearNotification();
	ClearWarning();
	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
	if ( CurTool )
	{
		CurTool->OnPropertySetsModified.RemoveAll(this);
		CurTool->OnPropertyModifiedDirectlyByTool.RemoveAll(this);
	}
}

void FDungeonForgeEditorModeToolkit::InvokeUI() {
	FModeToolkit::InvokeUI();
	
	//InlineContentHolder->SetContent(GetInlineContent().ToSharedRef());
	if (InlineContentHolder.IsValid()) {
		InlineContentHolder->SetContent(ForgeGraphHost.ToSharedRef());
	}
	
	if (TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin()) {
		TSharedPtr<FTabManager> TabManagerPtr = ModeUILayerPtr->GetTabManager();
		if (!TabManagerPtr) {
			return;
		}
		DetailsViewTab = TabManagerPtr->TryInvokeTab(FDungeonForgeTabsID::DetailsView);
	}
}

namespace DA::DungeonForgeLib {
	TSharedRef<SWidget> CreateForgeActorMissingPanel(FDungeonForgeEditorModeToolkit* InToolkit, const TFunction<void()>& RefreshPanelCallback) {
		// Show the helper widget
		return SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				// Message text
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Dungeon Forge Actor is not available in the scene."))
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Justification(ETextJustify::Center)
				]

				// Buttons container
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(10)
				[
					SNew(SVerticalBox)
					// Refresh Button
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
						.ToolTipText(FText::FromString("Search"))
						.OnClicked_Lambda([RefreshPanelCallback]() -> FReply {
							RefreshPanelCallback();
							return FReply::Handled();
						})
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(2.0f)
							[
								SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.Search"))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(2.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString("Search"))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							]
						]
					]

					// Create Dungeon Forge Actor Button
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.Padding(4)
							.AutoHeight()
							[
								SNew(SObjectPropertyEntryBox)
								.AllowedClass(UDungeonForgeAsset::StaticClass())
								.ThumbnailPool(UThumbnailManager::Get().GetSharedThumbnailPool())
								.ObjectPath(InToolkit, &FDungeonForgeEditorModeToolkit::GetForgeAssetPath)
								.OnObjectChanged(InToolkit, &FDungeonForgeEditorModeToolkit::OnForgeAssetChanged)
							]
							+SVerticalBox::Slot()
							.Padding(4)
							.AutoHeight()
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
								.ToolTipText(FText::FromString("Create a new Dungeon Actor in the scene"))
								.OnClicked_Lambda([RefreshPanelCallback, InToolkit]() -> FReply {
									if (InToolkit) {
										InToolkit->SpawnDungeonForgeActor();
									}
									RefreshPanelCallback();
									return FReply::Handled();
								})
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(2.0f)
									[
										SNew(SImage)
										.Image(FAppStyle::GetBrush("Icons.Plus"))
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(2.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString("Create New Dungeon Actor"))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
									]
								]
							]
						]
					]
				]
			];
	}

}

void FDungeonForgeEditorModeToolkit::UpdateGraphWidgetHost() {
	UDungeonForgeEditorMode* ForgeEdMode = GetForgeEditorMode();
	if (!ForgeEdMode) {
		return;
	}
	
	TWeakObjectPtr<UDungeonForgeGraph> ForgeGraph = ForgeEdMode->GetForgeGraph();
	if (UDungeonForgeGraph* ForgeGraphPtr = ForgeGraph.Get()) {
		ForgeEditorGraph = GetForgeEditorGraph(ForgeGraphPtr);
		ForgeEditorGraph->SetEditor(SharedThis(this));
	}
	
	if (ForgeEditorGraph.IsValid()) {
		GraphEditorWidget = CreateGraphEditorWidget();
		TSharedPtr<SWidget> GraphTabContent = SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SDungeonForgeEditorModeToolbar)
				.OnBuildButtonClicked(this, &FDungeonForgeEditorModeToolkit::HandleBuildButtonClicked)
			]
			+SVerticalBox::Slot()
			[
				GraphEditorWidget.ToSharedRef()
			];
		
		ForgeGraphHost->SetContent(GraphTabContent.ToSharedRef());
	}
	else {
		// Display a panel to search or create the dungeon forge actor
		TWeakPtr<FDungeonForgeEditorModeToolkit> WeakToolkit = SharedThis(this);
		TSharedRef<SWidget> DungeonForgeSearchPanel = DA::DungeonForgeLib::CreateForgeActorMissingPanel(this, [WeakToolkit]() {
			if (WeakToolkit.IsValid()) {
				TSharedPtr<FDungeonForgeEditorModeToolkit> Toolkit = WeakToolkit.Pin();
				Toolkit->UpdateGraphWidgetHost();
			}
		});
		ForgeGraphHost->SetContent(DungeonForgeSearchPanel);
	}
}

ADungeon* FDungeonForgeEditorModeToolkit::GetDungeonActor() const {
	if (UDungeonForgeEditorMode* ForgeEdMode = GetForgeEditorMode()) {
		return ForgeEdMode->GetDungeonActor();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE

