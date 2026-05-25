//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeToolkit.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditablePaletteConfig.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorCommands.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Dialogs/Dialogs.h"
#include "Features/IModularFeatures.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/MultiBox/SToolBarButtonBlock.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IDetailsView.h"
#include "IToolPresetEditorModule.h"
#include "InteractiveToolManager.h"
#include "LevelEditorViewport.h"
#include "ModelingWidgets/SToolInputAssetComboPanel.h"
#include "Modules/ModuleManager.h"
#include "SPrimaryButton.h"
#include "STransformGizmoNumericalUIOverlay.h"
#include "ToolMenus.h"
#include "ToolPresetAsset.h"
#include "ToolPresetAssetSubsystem.h"
#include "ToolPresetSettings.h"
#include "ToolkitBuilder.h"
#include "Toolkits/AssetEditorModeUILayer.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "Tools/UEdMode.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DungeonToolsEditorModeToolkit"

static TAutoConsoleVariable<int32> CVarEnableToolPresets(
	TEXT("da.EnablePresets"),
	1,
	TEXT("Enable tool preset features and UX"));


class FRecentPresetCollectionProvider : public SToolInputAssetComboPanel::IRecentAssetsProvider
{
public:
	//~ SToolInputAssetComboPanel::IRecentAssetsProvider interface		
	virtual TArray<FAssetData> GetRecentAssetsList() override { return RecentPresetCollectionList; }		
	virtual void NotifyNewAsset(const FAssetData& NewAsset) {
		RecentPresetCollectionList.AddUnique(NewAsset);
	};

protected:
	TArray<FAssetData> RecentPresetCollectionList;
};


namespace FDungeonToolsEditorModeToolkitLocals
{
	typedef TFunction<void(UInteractiveToolsPresetCollectionAsset& Preset, UInteractiveTool& Tool)> PresetAndToolFunc;
	typedef TFunction<void(UInteractiveToolsPresetCollectionAsset& Preset)> PresetOnlyFunc;

	void ExecuteWithPreset(const FSoftObjectPath& PresetPath, PresetOnlyFunc Function)
	{
		UInteractiveToolsPresetCollectionAsset* Preset = nullptr;

		UToolPresetAssetSubsystem* PresetAssetSubsystem = GEditor->GetEditorSubsystem<UToolPresetAssetSubsystem>();

		if (PresetPath.IsNull() && ensure(PresetAssetSubsystem))
		{
			Preset = PresetAssetSubsystem->GetDefaultCollection();
		}
		if (PresetPath.IsAsset())
		{
			Preset = Cast<UInteractiveToolsPresetCollectionAsset>(PresetPath.TryLoad());
		}
		if (!Preset)
		{
			return;
		}
		Function(*Preset);


	}

	void ExecuteWithPresetAndTool(UEdMode& EdMode, EToolSide ToolSide, const FSoftObjectPath& PresetPath, PresetAndToolFunc Function)
	{
		UInteractiveToolsPresetCollectionAsset* Preset = nullptr;		
		UInteractiveTool* Tool = EdMode.GetToolManager()->GetActiveTool(EToolSide::Left);

		UToolPresetAssetSubsystem* PresetAssetSubsystem = GEditor->GetEditorSubsystem<UToolPresetAssetSubsystem>();

		if (PresetPath.IsNull() && ensure(PresetAssetSubsystem))
		{
			Preset = PresetAssetSubsystem->GetDefaultCollection();
		}
		if (PresetPath.IsAsset())
		{
			Preset = Cast<UInteractiveToolsPresetCollectionAsset>(PresetPath.TryLoad());
		}
		if (!Preset || !Tool)
		{
			return;
		}
		Function(*Preset, *Tool);
	}

	void ExecuteWithPresetAndTool(UEdMode& EdMode, EToolSide ToolSide, UInteractiveToolsPresetCollectionAsset& Preset, PresetAndToolFunc Function)
	{
		UInteractiveTool* Tool = EdMode.GetToolManager()->GetActiveTool(EToolSide::Left);

		if (!Tool)
		{
			return;
		}
		Function(Preset, *Tool);
	}

	
	void ProcessChildWidgetsByType(
		const TSharedRef<SWidget>& RootWidget,
		const FString& WidgetType,
		TFunction<bool(TSharedRef<SWidget>&)> ProcessFunc)
	{
		auto ProcessChildWidgets = [ProcessFunc](const TSharedRef<SWidget>& Widget, const FString& WidgetType, auto& FindRef) -> void
		{
			FChildren* Children = Widget->GetChildren();
			const int32 NumChild = Children ? Children->NumSlot() : 0;
			for (int32 ChildIdx = 0; ChildIdx < NumChild; ++ChildIdx)
			{
				TSharedRef<SWidget> ChildWidget = Children->GetChildAt(ChildIdx);
				if (ChildWidget->GetTypeAsString().Compare(WidgetType) == 0 && !ProcessFunc(ChildWidget))
				{
					break;
				}
				FindRef(ChildWidget, WidgetType, FindRef);
			}
		};
		ProcessChildWidgets(RootWidget, WidgetType, ProcessChildWidgets);
	}
}

FDungeonToolsEditorModeToolkit::FDungeonToolsEditorModeToolkit()
{
	UDungeonToolsEditablePaletteConfig::Initialize();
	UDungeonToolsEditablePaletteConfig::Get()->LoadEditorConfig();

	UToolPresetUserSettings::Initialize();
	UToolPresetUserSettings::Get()->LoadEditorConfig();

	RecentPresetCollectionProvider = MakeShared< FRecentPresetCollectionProvider>();
	CurrentPreset = MakeShared<FAssetData>();
}

FDungeonToolsEditorModeToolkit::~FDungeonToolsEditorModeToolkit()
{
	if (IsHosted())
	{
		if (GizmoNumericalUIOverlayWidget.IsValid())
		{
			GetToolkitHost()->RemoveViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef());
			GizmoNumericalUIOverlayWidget.Reset();
		}
	}
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolNotificationMessage.RemoveAll(this);
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolWarningMessage.RemoveAll(this);

	RecentPresetCollectionProvider = nullptr;
}


void FDungeonToolsEditorModeToolkit::CustomizeModeDetailsViewArgs(FDetailsViewArgs& ArgsInOut)
{
	//ArgsInOut.ColumnWidth = 0.3f;
}


TSharedPtr<SWidget> FDungeonToolsEditorModeToolkit::GetInlineContent() const
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.VAlign(VAlign_Fill)
		[
			ToolkitWidget.ToSharedRef()
		];
}



void FDungeonToolsEditorModeToolkit::RegisterPalettes()
{
	const FDungeonToolsEditorCommands& Commands = FDungeonToolsEditorCommands::Get();
	const TSharedPtr<FUICommandList> CommandList = GetToolkitCommands();

	UEdMode* ScriptableMode = GetScriptableEditorMode().Get();
	UDungeonToolsModeCustomizationSettings* UISettings = GetMutableDefault<UDungeonToolsModeCustomizationSettings>();
	UDungeonToolsEditorModeSettings* ModelingModeSettings = GetMutableDefault<UDungeonToolsEditorModeSettings>();
	UISettings->OnSettingChanged().AddSP(SharedThis(this), &FDungeonToolsEditorModeToolkit::UpdateCategoryButtonLabelVisibility);
	
	ToolkitSections = MakeShared<FToolkitSections>();
	FToolkitBuilderArgs ToolkitBuilderArgs(ScriptableMode->GetModeInfo().ToolbarCustomizationName);
	ToolkitBuilderArgs.ToolkitCommandList = GetToolkitCommands();
	ToolkitBuilderArgs.ToolkitSections = ToolkitSections;
	ToolkitBuilderArgs.SelectedCategoryTitleVisibility = EVisibility::Collapsed;
	// This lets us re-show the buttons if the user clicks a category with a tool still active.
	ToolkitBuilderArgs.CategoryReclickBehavior = FToolkitBuilder::ECategoryReclickBehavior::TreatAsChanged;
	ToolkitBuilder = MakeShared<FToolkitBuilder>(ToolkitBuilderArgs);
	ToolkitBuilder->SetCategoryButtonLabelVisibility(UISettings->bShowCategoryButtonLabels);

	// We need to specify the modeling mode specific config instance because this cannot exist at the base toolkit level due to dependency issues currently
	FGetEditableToolPaletteConfigManager GetConfigManager = FGetEditableToolPaletteConfigManager::CreateStatic(&UDungeonToolsEditablePaletteConfig::GetAsConfigManager);

	FavoritesPalette =
		MakeShareable(new FEditablePalette(
			Commands.LoadFavoritesTools.ToSharedRef(),
			Commands.AddToFavorites.ToSharedRef(),
			Commands.RemoveFromFavorites.ToSharedRef(),
			"DungeonToolsEditorModeFavoritesPalette",
			GetConfigManager));
	
	ToolkitBuilder->AddPalette(StaticCastSharedPtr<FEditablePalette>(FavoritesPalette));

	// Layout designer tools
	ToolkitBuilder->AddPalette(MakeShareable(new FToolPalette(Commands.LoadLayoutTools.ToSharedRef(), {
		Commands.BeginLayoutSegmentManageTool
	})));

	// Grid builder tools
	ToolkitBuilder->AddPalette(MakeShareable(new FToolPalette(Commands.LoadGridTools.ToSharedRef(), {
		Commands.BeginGridPaintTool
	})));
	
	// Snap tools
	ToolkitBuilder->AddPalette(MakeShareable(new FToolPalette(Commands.LoadSnapTools.ToSharedRef(), {
		Commands.BeginSnapStitchModuleTool,
		Commands.BeginSnapConnectionTool
	})));

	// Flow tools
	ToolkitBuilder->AddPalette(MakeShareable(new FToolPalette(Commands.LoadFlowTools.ToSharedRef(), {
		Commands.BeginFlowNodeTool
	})));
	
	ToolkitBuilder->SetActivePaletteOnLoad(Commands.LoadGridTools.Get());
	ToolkitBuilder->UpdateWidget();


	// if selected palette changes, make sure we are showing the palette command buttons, which may be hidden by active Tool
	ActivePaletteChangedHandle = ToolkitBuilder->OnActivePaletteChanged.AddLambda([this]()
	{
		ToolkitBuilder->SetActivePaletteCommandsVisibility(EVisibility::Visible);
	});
}




void FDungeonToolsEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	const UDungeonToolsModeCustomizationSettings* UISettings = GetDefault<UDungeonToolsModeCustomizationSettings>();
	bUsesToolkitBuilder = !UISettings->bUseLegacyModelingPalette;
	
	// Have to create the ToolkitWidget here because FModeToolkit::Init() is going to ask for it and add
	// it to the Mode panel, and not ask again afterwards. However we have to call Init() to get the 
	// ModeDetailsView created, that we need to add to the ToolkitWidget. So, we will create the Widget
	// here but only add the rows to it after we call Init()

	const TSharedPtr<SVerticalBox> ToolkitWidgetVBox = SNew(SVerticalBox);
	
	if ( !bUsesToolkitBuilder )
	{
		SAssignNew(ToolkitWidget, SBorder)
			.HAlign(HAlign_Fill)
			.Padding(4)
			[
				ToolkitWidgetVBox->AsShared()
			];
	}

	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	GetToolkitHost()->OnActiveViewportChanged().AddSP(this, &FDungeonToolsEditorModeToolkit::OnActiveViewportChanged);

	ModeWarningArea = SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f)));
	ModeWarningArea->SetText(FText::GetEmpty());
	ModeWarningArea->SetVisibility(EVisibility::Collapsed);

	ModeHeaderArea = SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	ModeHeaderArea->SetText(LOCTEXT("SelectToolLabel", "Select a Tool from the Toolbar"));
	ModeHeaderArea->SetJustification(ETextJustify::Center);


	ToolWarningArea = SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f)));
	ToolWarningArea->SetText(FText::GetEmpty());

	ToolPresetArea = MakePresetPanel();

	// add the various sections to the mode toolbox
	ToolkitWidgetVBox->AddSlot().AutoHeight().HAlign(HAlign_Fill).Padding(5)
		[
			ToolPresetArea->AsShared()
		];
	ToolkitWidgetVBox->AddSlot().AutoHeight().HAlign(HAlign_Fill).Padding(5)
		[
			ModeWarningArea->AsShared()
		];

	if (bUsesToolkitBuilder)
	{
		RegisterPalettes();
	}
	else
	{
		ToolkitWidgetVBox->AddSlot().AutoHeight().HAlign(HAlign_Fill).Padding(5)
			[
				ModeHeaderArea->AsShared()
			];
		ToolkitWidgetVBox->AddSlot().AutoHeight().HAlign(HAlign_Fill).Padding(5)
		[
			ToolWarningArea->AsShared()
		];	
		ToolkitWidgetVBox->AddSlot().HAlign(HAlign_Fill).FillHeight(1.f)
		[
			ModeDetailsView->AsShared()
		];
	}
		

	ClearNotification();
	ClearWarning();

	if (HasToolkitBuilder())
	{
		ToolkitSections->ModeWarningArea = ModeWarningArea;
		ToolkitSections->ToolPresetArea = ToolPresetArea;
		ToolkitSections->DetailsView = ModeDetailsView;
		ToolkitSections->ToolWarningArea = ToolWarningArea;
		//ToolkitSections->Footer = MakeAssetConfigPanel();

		SAssignNew(ToolkitWidget, SBorder)
		.HAlign(HAlign_Fill)
		.Padding(0)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			ToolkitBuilder->GenerateWidget()->AsShared()
		];	
	}

	ActiveToolName = FText::GetEmpty();
	ActiveToolMessage = FText::GetEmpty();

	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolNotificationMessage.AddSP(this, &FDungeonToolsEditorModeToolkit::PostNotification);
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolWarningMessage.AddSP(this, &FDungeonToolsEditorModeToolkit::PostWarning);

	MakeToolShutdownOverlayWidget();

	// Note that the numerical UI widget should be created before making the selection palette so that
	// it can be bound to the buttons there.
	MakeGizmoNumericalUIOverlayWidget();
	GetToolkitHost()->AddViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef());

	CurrentPresetPath = FSoftObjectPath(); // Default to the default collection by leaving this null.
}

void FDungeonToolsEditorModeToolkit::MakeToolShutdownOverlayWidget()
{
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
				.Text(this, &FDungeonToolsEditorModeToolkit::GetActiveToolDisplayName)
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

void FDungeonToolsEditorModeToolkit::MakeGizmoNumericalUIOverlayWidget()
{
	GizmoNumericalUIOverlayWidget = SNew(STransformGizmoNumericalUIOverlay)
		.DefaultLeftPadding(15)
		// Position above the little axis visualization
		.DefaultVerticalPadding(75)
		.bPositionRelativeToBottom(true);
}

TSharedRef<SWidget> FDungeonToolsEditorModeToolkit::MakeMenu_ModelingModeConfigSettings() {
	return SNullWidget::NullWidget;
}


TSharedRef<SWidget> FDungeonToolsEditorModeToolkit::MakePresetComboWidget(TSharedPtr<FString> InItem)
{
	return
		SNew(STextBlock)
		.Text(FText::FromString(*InItem));
}

TSharedPtr<SWidget> FDungeonToolsEditorModeToolkit::MakePresetPanel()
{
	const TSharedPtr<SVerticalBox> Content = SNew(SVerticalBox);
	UDungeonToolsEditorModeSettings* Settings = GetMutableDefault<UDungeonToolsEditorModeSettings>();

	const bool bEnableToolPresets = (CVarEnableToolPresets.GetValueOnGameThread() > 0);	
	if (!bEnableToolPresets)
	{
		return SNew(SVerticalBox);
	}

	const TSharedPtr<SHorizontalBox> NewContent = SNew(SHorizontalBox);
	
	auto IsToolActive = [this]() {
		if (this->OwningEditorMode.IsValid())
		{
			return this->OwningEditorMode->GetToolManager()->GetActiveTool(EToolSide::Left) != nullptr ? EVisibility::Visible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	};

	NewContent->AddSlot().HAlign(HAlign_Right)
	[
		SNew(SComboButton)
		.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
		.OnGetMenuContent(this, &FDungeonToolsEditorModeToolkit::GetPresetCreateButtonContent)
		.HasDownArrow(true)
		.Visibility_Lambda(IsToolActive)
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ModelingPresetPanelHeader", "Presets"))
				.Justification(ETextJustify::Center)
				.Font(FAppStyle::Get().GetFontStyle("EditorModesPanel.CategoryFontStyle"))
			]

		]
	];

	TSharedPtr<SHorizontalBox> AssetConfigPanel = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			NewContent->AsShared()
		];

	return AssetConfigPanel;
}

bool FDungeonToolsEditorModeToolkit::IsPresetEnabled() const
{
	return CurrentPresetPath.IsAsset();
}


TSharedRef<SWidget> FDungeonToolsEditorModeToolkit::GetPresetCreateButtonContent()
{
	RebuildPresetListForTool(false);

		auto OpenNewPresetDialog = [this]()
	{
		NewPresetLabel.Empty();
		NewPresetTooltip.Empty();

		// Set the result if they just click Ok
		SGenericDialogWidget::FArguments FolderDialogArguments;
		FolderDialogArguments.OnOkPressed_Lambda([this]()
			{
				CreateNewPresetInCollection(NewPresetLabel,
					CurrentPresetPath,
					NewPresetTooltip,
					NewPresetIcon);
			});

		// Present the Dialog
		SGenericDialogWidget::OpenDialog(LOCTEXT("ToolPresets_CreatePreset", "Create new preset from active tool's settings"),
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToolPresets_CreatePresetLabel", "Label"))
				]
				+ SHorizontalBox::Slot()
				.MaxWidth(300)
				[
					SNew(SEditableTextBox)
					// Cap the number of characters sent out of the text box, so we don't overflow menus and tooltips
					.OnTextCommitted_Lambda([this](const FText& NewLabel, const ETextCommit::Type&) { NewPresetLabel = NewLabel.ToString().Left(255); })
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.ToolTipText(LOCTEXT("ToolPresets_CreatePresetLabel_Tooltip", "A short, descriptive identifier for the new preset."))
				]
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToolPresets_CreatePresetTooltip", "Tooltip"))
				]
				+ SHorizontalBox::Slot()
				.MaxWidth(300)
				[
					SNew(SBox)
					.MinDesiredHeight(44.f)
					.MaxDesiredHeight(44.0f)
					[
						SNew(SMultiLineEditableTextBox)
						// Cap the number of characters sent out of the text box, so we don't overflow menus and tooltips
						.OnTextCommitted_Lambda([this](const FText& NewToolTip, const ETextCommit::Type&) { NewPresetTooltip = NewToolTip.ToString().Left(2048); })
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.AllowMultiLine(false)
						.AutoWrapText(true)
						.WrappingPolicy(ETextWrappingPolicy::DefaultWrapping)
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.ToolTipText(LOCTEXT("ToolPresets_CreatePresetTooltip_Tooltip", "A descriptive tooltip for the new preset."))
					]
				]
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SToolInputAssetComboPanel)
					.AssetClassType(UInteractiveToolsPresetCollectionAsset::StaticClass())
					.OnSelectionChanged(this, &FDungeonToolsEditorModeToolkit::HandlePresetAssetChanged)
					.ToolTipText(LOCTEXT("ToolPresets_CreatePresetCollection_Tooltip", "The asset in which to store this new preset."))
					//.RecentAssetsProvider(RecentPresetCollectionProvider) // TODO: Improve this widget before enabling this feature
					.InitiallySelectedAsset(*CurrentPreset)
					.FlyoutTileSize(FVector2D(80, 80))
					.ComboButtonTileSize(FVector2D(80, 80))
					.AssetThumbnailLabel(EThumbnailLabel::AssetName)
					.bForceShowPluginContent(true)
					.bForceShowEngineContent(true)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(10, 5)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ToolPresets_CreatePresetCollection", "Collection"))
						.Font(FSlateFontInfo(FCoreStyle::GetDefaultFont(),12, "Bold"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(10, 5)
					[
						SNew(STextBlock)
						.Text_Lambda([this](){
						if (CurrentPresetLabel.IsEmpty())
						{
							return LOCTEXT("NewPresetNoCollectionSpecifiedMessage", "None - Preset will be added to the default Personal Presets Collection.");
						}
						else {
							return CurrentPresetLabel;
						}})
					]

				]					
			],
			FolderDialogArguments, true);
	};

	constexpr bool bShouldCloseMenuAfterSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseMenuAfterSelection, nullptr);

	constexpr bool bNoIndent = false;
	constexpr bool bSearchable = false;
	static const FName NoExtensionHook = NAME_None;


	{
		FMenuEntryParams MenuEntryParams;

		typedef TMap<FString, TArray<TSharedPtr<FDungeonToolPresetOption>>> FPresetsByNameMap;
		FPresetsByNameMap PresetsByCollectionName;
		for (TSharedPtr<FDungeonToolPresetOption> ToolPresetOption : AvailablePresetsForTool)
		{
			FDungeonToolsEditorModeToolkitLocals::ExecuteWithPreset(ToolPresetOption->PresetCollection,
				[this, &PresetsByCollectionName, &ToolPresetOption](UInteractiveToolsPresetCollectionAsset& Preset) {
					PresetsByCollectionName.FindOrAdd(Preset.CollectionLabel.ToString()).Add(ToolPresetOption);
				});
		}

		for (FPresetsByNameMap::TConstIterator Iterator = PresetsByCollectionName.CreateConstIterator(); Iterator; ++Iterator)
		{
			MenuBuilder.BeginSection(NoExtensionHook, FText::FromString(Iterator.Key()));
			for (const TSharedPtr<FDungeonToolPresetOption>& ToolPresetOption : Iterator.Value())
			{
				FUIAction ApplyPresetAction;
				ApplyPresetAction.ExecuteAction = FExecuteAction::CreateLambda([this, ToolPresetOption]()
					{
						LoadPresetFromCollection(ToolPresetOption->PresetIndex, ToolPresetOption->PresetCollection);
					});

				ApplyPresetAction.CanExecuteAction = FCanExecuteAction::CreateLambda([this, ToolPresetOption]()
					{
						return this->OwningEditorMode->GetToolManager()->GetActiveTool(EToolSide::Left) != nullptr;
					});

				MenuBuilder.AddMenuEntry(
					FText::FromString(ToolPresetOption->PresetLabel),
					FText::FromString(ToolPresetOption->PresetTooltip),
					ToolPresetOption->PresetIcon,
					ApplyPresetAction);
			}
			MenuBuilder.EndSection();
		}

		MenuBuilder.BeginSection(NoExtensionHook, LOCTEXT("ModelingPresetPanelHeaderManagePresets", "Manage Presets"));


		FUIAction CreateNewPresetAction;
		CreateNewPresetAction.ExecuteAction = FExecuteAction::CreateLambda(OpenNewPresetDialog);
		MenuBuilder.AddMenuEntry(
			FText(LOCTEXT("ModelingPresetPanelCreateNewPreset", "Create New Preset")),
			FText(LOCTEXT("ModelingPresetPanelCreateNewPresetTooltip", "Create New Preset in specified Collection")),
			FSlateIcon( FAppStyle::Get().GetStyleSetName(), "Icons.Plus"),
			CreateNewPresetAction);


		FUIAction OpenPresetManangerAction;
		OpenPresetManangerAction.ExecuteAction = FExecuteAction::CreateLambda([this]() {
			IToolPresetEditorModule::Get().ExecuteOpenPresetEditor();
		});
		MenuBuilder.AddMenuEntry(
			FText(LOCTEXT("ModelingPresetPanelOpenPresetMananger", "Manage Presets...")),
			FText(LOCTEXT("ModelingPresetPanelOpenPresetManagerTooltip", "Open Preset Manager to manage presets and their collections")),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Settings"),
			OpenPresetManangerAction);

		MenuBuilder.EndSection();

	}

	return MenuBuilder.MakeWidget();
}

void FDungeonToolsEditorModeToolkit::ClearPresetComboList()
{
	AvailablePresetsForTool.Empty();
}

void FDungeonToolsEditorModeToolkit::RebuildPresetListForTool(bool bSettingsOpened)
{	
	TObjectPtr<UToolPresetUserSettings> UserSettings = UToolPresetUserSettings::Get();
	UserSettings->LoadEditorConfig();

	// We need to generate a combined list of Project Loaded and User available presets to intersect the enabled set against...
	const UToolPresetProjectSettings* ProjectSettings = GetDefault<UToolPresetProjectSettings>();
	TSet<FSoftObjectPath> AllUserPresets;
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FAssetData> AssetData;
	FARFilter Filter;
	Filter.ClassPaths.Add(UInteractiveToolsPresetCollectionAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName("/ToolPresets"));
	Filter.bRecursiveClasses = false;
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = false;
	
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);
	for (int i = 0; i < AssetData.Num(); i++) {
		UInteractiveToolsPresetCollectionAsset* Object = Cast<UInteractiveToolsPresetCollectionAsset>(AssetData[i].GetAsset());
		if (Object)
		{
			AllUserPresets.Add(Object->GetPathName());
		}
	}

	TSet<FSoftObjectPath> AvailablePresetCollections = UserSettings->EnabledPresetCollections.Intersect( ProjectSettings->LoadedPresetCollections.Union(AllUserPresets));
	if (UserSettings->bDefaultCollectionEnabled)
	{
		AvailablePresetCollections.Add(FSoftObjectPath());
	}


	AvailablePresetsForTool.Empty();
	for (const FSoftObjectPath& PresetCollection : AvailablePresetCollections)
	{
		FDungeonToolsEditorModeToolkitLocals::ExecuteWithPresetAndTool(*OwningEditorMode, EToolSide::Left, PresetCollection,
			[this, PresetCollection](UInteractiveToolsPresetCollectionAsset& Preset, UInteractiveTool& Tool) {
		
				if (!Preset.PerToolPresets.Contains(Tool.GetClass()->GetName()))
				{
					return;
				}
				AvailablePresetsForTool.Reserve(Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets.Num());
				for (int32 PresetIndex = 0; PresetIndex < Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets.Num(); ++PresetIndex)
				{
					const FInteractiveToolPresetDefinition& PresetDef = Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets[PresetIndex];
					if (!PresetDef.IsValid())
					{
						continue;
					}
					TSharedPtr<FDungeonToolPresetOption> NewOption = MakeShared<FDungeonToolPresetOption>();
					if (PresetDef.Label.Len() > 50)
					{
						NewOption->PresetLabel = PresetDef.Label.Left(50) + FString("...");
					}
					else
					{
						NewOption->PresetLabel = PresetDef.Label;
					}
					if (PresetDef.Tooltip.Len() > 2048)
					{
						NewOption->PresetTooltip = PresetDef.Tooltip.Left(2048) + FString("...");
					}
					else
					{
						NewOption->PresetTooltip = PresetDef.Tooltip;
					}
					NewOption->PresetIndex = PresetIndex;
					NewOption->PresetCollection = PresetCollection;

					AvailablePresetsForTool.Add(NewOption);
				}
			});
	}
}

void FDungeonToolsEditorModeToolkit::HandlePresetAssetChanged(const FAssetData& InAssetData)
{
	CurrentPresetPath.SetPath(InAssetData.GetObjectPathString());
	CurrentPresetLabel = FText();
	*CurrentPreset = InAssetData;

	UInteractiveToolsPresetCollectionAsset* Preset = nullptr;
	if (CurrentPresetPath.IsAsset())
	{
		Preset = Cast<UInteractiveToolsPresetCollectionAsset>(CurrentPresetPath.TryLoad());
	}
	if (Preset)
	{
		CurrentPresetLabel = Preset->CollectionLabel;
	}
	
}

bool FDungeonToolsEditorModeToolkit::HandleFilterPresetAsset(const FAssetData& InAssetData)
{
	return false;
}

void FDungeonToolsEditorModeToolkit::LoadPresetFromCollection(const int32 PresetIndex, FSoftObjectPath CollectionPath)
{
	FDungeonToolsEditorModeToolkitLocals::ExecuteWithPresetAndTool(*OwningEditorMode, EToolSide::Left, CollectionPath,
	[this, PresetIndex](UInteractiveToolsPresetCollectionAsset& Preset, UInteractiveTool& Tool) {
		TArray<UObject*> PropertySets = Tool.GetToolProperties();

		// We only want to load the properties that are actual property sets, since the tool might have added other types of objects we don't
		// want to deserialize.
		PropertySets.RemoveAll([this](UObject* Object) {
			return Cast<UInteractiveToolPropertySet>(Object) == nullptr;
		});

		if (!Preset.PerToolPresets.Contains(Tool.GetClass()->GetName()))			
		{
			return;
		}
		if(PresetIndex < 0 || PresetIndex >= Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets.Num())
		{
			return;
		}

		Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets[PresetIndex].LoadStoredPropertyData(PropertySets);

	});
}

void FDungeonToolsEditorModeToolkit::CreateNewPresetInCollection(const FString& PresetLabel, FSoftObjectPath CollectionPath, const FString& ToolTip, FSlateIcon Icon)
{
	FDungeonToolsEditorModeToolkitLocals::ExecuteWithPresetAndTool(*OwningEditorMode, EToolSide::Left, CurrentPresetPath,
	[this, PresetLabel, ToolTip, Icon, CollectionPath](UInteractiveToolsPresetCollectionAsset& Preset, UInteractiveTool& Tool) {

		TArray<UObject*> PropertySets = Tool.GetToolProperties();

		// We only want to add the properties that are actual property sets, since the tool might have added other types of objects we don't
		// want to serialize.
		PropertySets.RemoveAll([this](UObject* Object) {
			return Cast<UInteractiveToolPropertySet>(Object) == nullptr;
		});

		Preset.PerToolPresets.FindOrAdd(Tool.GetClass()->GetName()).ToolLabel = ActiveToolName;
		if (ensure(ActiveToolIcon))
		{
			Preset.PerToolPresets.FindOrAdd(Tool.GetClass()->GetName()).ToolIcon = *ActiveToolIcon;
		}
		FInteractiveToolPresetDefinition PresetValuesToCreate;
		if (PresetLabel.IsEmpty())
		{
			PresetValuesToCreate.Label = FString::Printf(TEXT("Unnamed_Preset-%d"), Preset.PerToolPresets.FindOrAdd(Tool.GetClass()->GetName()).NamedPresets.Num()+1);
		}
		else
		{
			PresetValuesToCreate.Label = PresetLabel;
		}
		PresetValuesToCreate.Tooltip = ToolTip;
		//PresetValuesToCreate.Icon = Icon;

		PresetValuesToCreate.SetStoredPropertyData(PropertySets);


		Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets.Add(PresetValuesToCreate);
		Preset.MarkPackageDirty();

		// Finally add this to the current tool's preset list, since we know it should be there.
		TSharedPtr<FDungeonToolPresetOption> NewOption = MakeShared<FDungeonToolPresetOption>();
		NewOption->PresetLabel = PresetLabel;
		NewOption->PresetTooltip = ToolTip;
		NewOption->PresetIndex = Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets.Num() - 1;
		NewOption->PresetCollection = CollectionPath;

		AvailablePresetsForTool.Add(NewOption);
		
	});

	UToolPresetAssetSubsystem* PresetAssetSubsystem = GEditor->GetEditorSubsystem<UToolPresetAssetSubsystem>();
	if (CollectionPath.IsNull() && ensure(PresetAssetSubsystem))
	{
		ensure(PresetAssetSubsystem->SaveDefaultCollection());
	}
}

void FDungeonToolsEditorModeToolkit::UpdatePresetInCollection(const FDungeonToolPresetOption& PresetToEditIn, bool bUpdateStoredPresetValues)
{
	FDungeonToolsEditorModeToolkitLocals::ExecuteWithPresetAndTool(*OwningEditorMode, EToolSide::Left, PresetToEditIn.PresetCollection,
		[this, PresetToEditIn, bUpdateStoredPresetValues](UInteractiveToolsPresetCollectionAsset& Preset, UInteractiveTool& Tool) {
			TArray<UObject*> PropertySets = Tool.GetToolProperties();

			// We only want to add the properties that are actual property sets, since the tool might have added other types of objects we don't
			// want to serialize.
			PropertySets.RemoveAll([this](UObject* Object) {
				return Cast<UInteractiveToolPropertySet>(Object) == nullptr;
			});

			if (!Preset.PerToolPresets.Contains(Tool.GetClass()->GetName()))
			{
				return;
			}
			if (PresetToEditIn.PresetIndex < 0 || PresetToEditIn.PresetIndex >= Preset.PerToolPresets[Tool.GetClass()->GetName()].NamedPresets.Num() )
			{
				return;
			}

			if (bUpdateStoredPresetValues)
			{
				Preset.PerToolPresets.FindOrAdd(Tool.GetClass()->GetName()).NamedPresets[PresetToEditIn.PresetIndex].SetStoredPropertyData(PropertySets);
				Preset.MarkPackageDirty();
			}

			Preset.PerToolPresets.FindOrAdd(Tool.GetClass()->GetName()).NamedPresets[PresetToEditIn.PresetIndex].Label = PresetToEditIn.PresetLabel;
			Preset.PerToolPresets.FindOrAdd(Tool.GetClass()->GetName()).NamedPresets[PresetToEditIn.PresetIndex].Tooltip = PresetToEditIn.PresetTooltip;

		});

	RebuildPresetListForTool(false);

	UToolPresetAssetSubsystem* PresetAssetSubsystem = GEditor->GetEditorSubsystem<UToolPresetAssetSubsystem>();
	if (PresetToEditIn.PresetCollection.IsNull() && ensure(PresetAssetSubsystem))
	{
		ensure(PresetAssetSubsystem->SaveDefaultCollection());
	}
}




void FDungeonToolsEditorModeToolkit::InitializeAfterModeSetup()
{
	if (bFirstInitializeAfterModeSetup)
	{

		bFirstInitializeAfterModeSetup = false;
	}

}



void FDungeonToolsEditorModeToolkit::UpdateActiveToolProperties()
{
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

void FDungeonToolsEditorModeToolkit::InvalidateCachedDetailPanelState(UObject* ChangedObject)
{
	ModeDetailsView->InvalidateCachedState();
}


void FDungeonToolsEditorModeToolkit::PostNotification(const FText& Message)
{
	ClearNotification();

	ActiveToolMessage = Message;

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		ActiveToolMessageHandle = GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PushStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessage);
	}
}

void FDungeonToolsEditorModeToolkit::ClearNotification()
{
	ActiveToolMessage = FText::GetEmpty();

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PopStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessageHandle);
	}
	ActiveToolMessageHandle.Reset();
}


void FDungeonToolsEditorModeToolkit::PostWarning(const FText& Message)
{
	ToolWarningArea->SetText(Message);
	ToolWarningArea->SetVisibility(EVisibility::Visible);
}

void FDungeonToolsEditorModeToolkit::ClearWarning()
{
	ToolWarningArea->SetText(FText());
	ToolWarningArea->SetVisibility(EVisibility::Collapsed);
}

FName FDungeonToolsEditorModeToolkit::GetToolkitFName() const
{
	return FName("ModelingToolsEditorMode");
}

FText FDungeonToolsEditorModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("ModelingToolsEditorModeToolkit", "DisplayName", "ModelingToolsEditorMode Tool");
}

static const FName PrimitiveTabName(TEXT("Shapes"));
static const FName CreateTabName(TEXT("Create"));
static const FName AttributesTabName(TEXT("Attributes"));
static const FName TriModelingTabName(TEXT("TriModel"));
static const FName PolyModelingTabName(TEXT("PolyModel"));
static const FName MeshProcessingTabName(TEXT("MeshOps"));
static const FName UVTabName(TEXT("UVs"));
static const FName TransformTabName(TEXT("Transform"));
static const FName DeformTabName(TEXT("Deform"));
static const FName VolumesTabName(TEXT("Volumes"));
static const FName PrototypesTabName(TEXT("Prototypes"));
static const FName PolyEditTabName(TEXT("PolyEdit"));
static const FName VoxToolsTabName(TEXT("VoxOps"));
static const FName LODToolsTabName(TEXT("LODs"));
static const FName BakingToolsTabName(TEXT("Baking"));
static const FName ModelingFavoritesTabName(TEXT("Favorites"));

static const FName SelectionActionsTabName(TEXT("Selection"));


const TArray<FName> FDungeonToolsEditorModeToolkit::PaletteNames_Standard = { PrimitiveTabName, CreateTabName, PolyModelingTabName, TriModelingTabName, DeformTabName, TransformTabName, MeshProcessingTabName, VoxToolsTabName, AttributesTabName, UVTabName, BakingToolsTabName, VolumesTabName, LODToolsTabName };


void FDungeonToolsEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames = PaletteNames_Standard;

	TArray<FName> ExistingNames;
	for ( FName Name : PaletteNames )
	{
		ExistingNames.Add(Name);
	}

	UDungeonToolsModeCustomizationSettings* UISettings = GetMutableDefault<UDungeonToolsModeCustomizationSettings>();

	// if user has provided custom ordering of tool palettes in the Editor Settings, try to apply them
	if (UISettings->ToolSectionOrder.Num() > 0)
	{
		TArray<FName> NewPaletteNames;
		for (FString SectionName : UISettings->ToolSectionOrder)
		{
			for (int32 k = 0; k < PaletteNames.Num(); ++k)
			{
				if (SectionName.Equals(PaletteNames[k].ToString(), ESearchCase::IgnoreCase)
				 || SectionName.Equals(GetToolPaletteDisplayName(PaletteNames[k]).ToString(), ESearchCase::IgnoreCase))
				{
					NewPaletteNames.Add(PaletteNames[k]);
					PaletteNames.RemoveAt(k);
					break;
				}
			}
		}
		NewPaletteNames.Append(PaletteNames);
		PaletteNames = MoveTemp(NewPaletteNames);
	}

	// if user has provided a list of favorite tools, add that palette to the list
	if (FavoritesPalette && FavoritesPalette->GetPaletteCommandNames().Num() > 0)
	{
		PaletteNames.Insert(ModelingFavoritesTabName, 0);
	}

}


FText FDungeonToolsEditorModeToolkit::GetToolPaletteDisplayName(FName Palette) const
{
	return FText::FromName(Palette);
}

void FDungeonToolsEditorModeToolkit::BuildToolPalette(FName PaletteIndex, class FToolBarBuilder& ToolbarBuilder) 
{
}


void FDungeonToolsEditorModeToolkit::InvokeUI()
{
	FModeToolkit::InvokeUI();

	// FModeToolkit::UpdatePrimaryModePanel() wrapped our GetInlineContent() output in a SScrollBar widget,
	// however this doesn't make sense as we want to dock panels to the "top" and "bottom" of our mode panel area,
	// and the details panel in the middle has it's own scrollbar already. The SScrollBar is hardcoded as the content
	// of FModeToolkit::InlineContentHolder so we can just replace it here
	InlineContentHolder->SetContent(GetInlineContent().ToSharedRef());

	// if the ToolkitBuilder is being used, it will make up the UI
	if (HasToolkitBuilder())
	{
		return;
	}

	//
	// Apply custom section header colors.
	// See comments below, this is done via directly manipulating Slate widgets generated deep inside BaseToolkit.cpp,
	// and will stop working if the Slate widget structure changes
	//

	UDungeonToolsModeCustomizationSettings* UISettings = GetMutableDefault<UDungeonToolsModeCustomizationSettings>();

	// Generate a map for tool specific colors
	TMap<FString, FLinearColor> SectionIconColorMap;
	TMap<FString, TMap<FString, FLinearColor>> SectionToolIconColorMap;
	for (const FDungeonModeCustomToolColor& ToolColor : UISettings->ToolColors)
	{
		FString SectionName, ToolName;
		ToolColor.ToolName.Split(".", &SectionName, &ToolName);
		SectionName.ToLowerInline();
		if (ToolName.Len() > 0)
		{
			if (!SectionToolIconColorMap.Contains(SectionName))
			{
				SectionToolIconColorMap.Emplace(SectionName, TMap<FString, FLinearColor>());
			}
			SectionToolIconColorMap[SectionName].Add(ToolName, ToolColor.Color);
		}
		else
		{
			SectionIconColorMap.Emplace(ToolColor.ToolName.ToLower(), ToolColor.Color);
		}
	}

	for (FEdModeToolbarRow& ToolbarRow : ActiveToolBarRows)
	{
		// Update section header colors
		for (FDungeonModeCustomSectionColor ToolColor : UISettings->SectionColors)
		{
			if (ToolColor.SectionName.Equals(ToolbarRow.PaletteName.ToString(), ESearchCase::IgnoreCase)
			 || ToolColor.SectionName.Equals(ToolbarRow.DisplayName.ToString(), ESearchCase::IgnoreCase))
			{
				// code below is highly dependent on the structure of the ToolbarRow.ToolbarWidget. Currently this is 
				// a SMultiBoxWidget, a few levels below a SExpandableArea. The SExpandableArea contains a SVerticalBox
				// with the header as a SBorder in Slot 0. The code will fail gracefully if this structure changes.

				TSharedPtr<SWidget> ExpanderVBoxWidget = (ToolbarRow.ToolbarWidget.IsValid() && ToolbarRow.ToolbarWidget->GetParentWidget().IsValid()) ?
					ToolbarRow.ToolbarWidget->GetParentWidget()->GetParentWidget() : TSharedPtr<SWidget>();
				if (ExpanderVBoxWidget.IsValid() && ExpanderVBoxWidget->GetTypeAsString().Compare(TEXT("SVerticalBox")) == 0)
				{
					TSharedPtr<SVerticalBox> ExpanderVBox = StaticCastSharedPtr<SVerticalBox>(ExpanderVBoxWidget);
					if (ExpanderVBox.IsValid() && ExpanderVBox->NumSlots() > 0)
					{
						const TSharedRef<SWidget>& SlotWidgetRef = ExpanderVBox->GetSlot(0).GetWidget();
						TSharedPtr<SWidget> SlotWidgetPtr(SlotWidgetRef);
						if (SlotWidgetPtr.IsValid() && SlotWidgetPtr->GetTypeAsString().Compare(TEXT("SBorder")) == 0)
						{
							TSharedPtr<SBorder> TopBorder = StaticCastSharedPtr<SBorder>(SlotWidgetPtr);
							if (TopBorder.IsValid())
							{
								// The border image needs to be swapped to a white one so that the color applied to it is
								// not darkened by the usual brush. Note that we can't just create a new FSlateRoundedBoxBrush
								// with the proper color in-line because a brush needs to be associated with a style set that
								// is responsible for freeing it, else it will leak (or we could own the brush in the toolkit,
								// but this is frowned upon).
								TopBorder->SetBorderImage(FDungeonArchitectStyle::Get().GetBrush("ModelingMode.WhiteExpandableAreaHeader"));
								TopBorder->SetBorderBackgroundColor(FSlateColor(ToolColor.Color));
							}
						}
					}
				}
				break;
			}
		}

		// Update tool colors
		FLinearColor* SectionIconColor = SectionIconColorMap.Find(ToolbarRow.PaletteName.ToString().ToLower());
		if (!SectionIconColor)
		{
			SectionIconColor = SectionIconColorMap.Find(ToolbarRow.DisplayName.ToString().ToLower());
		}
		TMap<FString, FLinearColor>* SectionToolIconColors = SectionToolIconColorMap.Find(ToolbarRow.PaletteName.ToString().ToLower());
		if (!SectionToolIconColors)
		{
			SectionToolIconColors = SectionToolIconColorMap.Find(ToolbarRow.DisplayName.ToString().ToLower());
		}
		if (SectionIconColor || SectionToolIconColors)
		{
			// code below is highly dependent on the structure of the ToolbarRow.ToolbarWidget. Currently this is 
			// a SMultiBoxWidget. The code will fail gracefully if this structure changes.
			
			if (ToolbarRow.ToolbarWidget.IsValid() && ToolbarRow.ToolbarWidget->GetTypeAsString().Compare(TEXT("SMultiBoxWidget")) == 0)
			{
				auto FindFirstChildWidget = [](const TSharedPtr<SWidget>& Widget, const FString& WidgetType)
				{
					TSharedPtr<SWidget> Result;
					FDungeonToolsEditorModeToolkitLocals::ProcessChildWidgetsByType(Widget->AsShared(), WidgetType,[&Result](TSharedRef<SWidget> Widget)
					{
						Result = TSharedPtr<SWidget>(Widget);
						// Stop processing after first occurrence
						return false;
					});
					return Result;
				};

				TSharedPtr<SWidget> PanelWidget = FindFirstChildWidget(ToolbarRow.ToolbarWidget, TEXT("SUniformWrapPanel"));
				if (PanelWidget.IsValid())
				{
					// This contains each of the FToolBarButtonBlock items for this row.
					FChildren* PanelChildren = PanelWidget->GetChildren();
					const int32 NumChild = PanelChildren ? PanelChildren->NumSlot() : 0;
					for (int32 ChildIdx = 0; ChildIdx < NumChild; ++ChildIdx)
					{
						const TSharedRef<SWidget> ChildWidgetRef = PanelChildren->GetChildAt(ChildIdx);
						TSharedPtr<SWidget> ChildWidgetPtr(ChildWidgetRef);
						if (ChildWidgetPtr.IsValid() && ChildWidgetPtr->GetTypeAsString().Compare(TEXT("SToolBarButtonBlock")) == 0)
						{
							TSharedPtr<SToolBarButtonBlock> ToolBarButton = StaticCastSharedPtr<SToolBarButtonBlock>(ChildWidgetPtr);
							if (ToolBarButton.IsValid())
							{
								TSharedPtr<SWidget> LayeredImageWidget = FindFirstChildWidget(ToolBarButton, TEXT("SLayeredImage"));
								TSharedPtr<SWidget> TextBlockWidget = FindFirstChildWidget(ToolBarButton, TEXT("STextBlock"));
								if (LayeredImageWidget.IsValid() && TextBlockWidget.IsValid())
								{
									TSharedPtr<SImage> ImageWidget = StaticCastSharedPtr<SImage>(LayeredImageWidget);
									TSharedPtr<STextBlock> TextWidget = StaticCastSharedPtr<STextBlock>(TextBlockWidget);
									// Check if this Section.Tool has an explicit color entry. If not, fallback
									// to any Section-wide color entry, otherwise leave the tint alone.
									FLinearColor* TintColor = SectionToolIconColors ? SectionToolIconColors->Find(TextWidget->GetText().ToString()) : nullptr;
									if (!TintColor)
									{
										const FString* SourceText = FTextInspector::GetSourceString(TextWidget->GetText());
										TintColor = SectionToolIconColors && SourceText ? SectionToolIconColors->Find(*SourceText) : nullptr;
										if (!TintColor)
										{
											TintColor = SectionIconColor;
										}
									}
									if (TintColor)
									{
										ImageWidget->SetColorAndOpacity(FSlateColor(*TintColor));
									}
								}
							}
						}
					}
				}
			}
		}
	}
}



void FDungeonToolsEditorModeToolkit::ForceToolPaletteRebuild()
{
	// disabling this for now because it resets the expand/contract state of all the palettes...

	//UGeometrySelectionManager* SelectionManager = Cast<UModelingToolsEditorMode>(GetScriptableEditorMode())->GetSelectionManager();
	//if (SelectionManager)
	//{
	//	bool bHasActiveSelection = SelectionManager->HasSelection();
	//	if (bShowActiveSelectionActions != bHasActiveSelection)
	//	{
	//		bShowActiveSelectionActions = bHasActiveSelection;
	//		this->RebuildModeToolPalette();
	//	}
	//}
	//else
	//{
	//	bShowActiveSelectionActions = false;
	//}
}

void FDungeonToolsEditorModeToolkit::BindGizmoNumericalUI()
{
	if (ensure(GizmoNumericalUIOverlayWidget.IsValid()))
	{
		ensure(GizmoNumericalUIOverlayWidget->BindToGizmoContextObject(GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)));
	}
}

void FDungeonToolsEditorModeToolkit::OnToolPaletteChanged(FName PaletteName) 
{
}

void FDungeonToolsEditorModeToolkit::ShowRealtimeAndModeWarnings(bool bShowRealtimeWarning)
{
	FText WarningText{};
	if (GEditor->bIsSimulatingInEditor)
	{
		WarningText = LOCTEXT("ModelingModeToolkitSimulatingWarning", "Cannot use Modeling Tools while simulating.");
	}
	else if (GEditor->PlayWorld != NULL)
	{
		WarningText = LOCTEXT("ModelingModeToolkitPIEWarning", "Cannot use Modeling Tools in PIE.");
	}
	else if (bShowRealtimeWarning)
	{
		WarningText = LOCTEXT("ModelingModeToolkitRealtimeWarning", "Realtime Mode is required for Modeling Tools to work correctly. Please enable Realtime Mode in the Viewport Options or with the Ctrl+r hotkey.");
	}
	if (!WarningText.IdenticalTo(ActiveWarning))
	{
		ActiveWarning = WarningText;
		ModeWarningArea->SetVisibility(ActiveWarning.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
		ModeWarningArea->SetText(ActiveWarning);
	}
}

void FDungeonToolsEditorModeToolkit::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	bInActiveTool = true;

	UpdateActiveToolProperties();

	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
	CurTool->OnPropertySetsModified.AddSP(this, &FDungeonToolsEditorModeToolkit::UpdateActiveToolProperties);
	CurTool->OnPropertyModifiedDirectlyByTool.AddSP(this, &FDungeonToolsEditorModeToolkit::InvalidateCachedDetailPanelState);

	ModeHeaderArea->SetVisibility(EVisibility::Collapsed);
	ActiveToolName = CurTool->GetToolInfo().ToolDisplayName;

	if (HasToolkitBuilder())
	{
		ToolkitBuilder->SetActiveToolDisplayName(ActiveToolName);
		if (const UDungeonToolsModeCustomizationSettings* Settings = GetDefault<UDungeonToolsModeCustomizationSettings>())
		{
			if (Settings->bAlwaysShowToolButtons == false)
			{
				ToolkitBuilder->SetActivePaletteCommandsVisibility(EVisibility::Collapsed);
			}
		}
	}

	// try to update icon
	FString ActiveToolIdentifier = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveToolName(EToolSide::Left);
	ActiveToolIdentifier.InsertAt(0, ".");
	FName ActiveToolIconName = ISlateStyle::Join(FDungeonToolsEditorCommands::Get().GetContextName(), TCHAR_TO_ANSI(*ActiveToolIdentifier));
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
	RebuildPresetListForTool(false);
}

void FDungeonToolsEditorModeToolkit::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
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

	ModeHeaderArea->SetVisibility(EVisibility::Visible);
	ModeHeaderArea->SetText(LOCTEXT("SelectToolLabel", "Select a Tool from the Toolbar"));
	ClearNotification();
	ClearWarning();
	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
	if ( CurTool )
	{
		CurTool->OnPropertySetsModified.RemoveAll(this);
		CurTool->OnPropertyModifiedDirectlyByTool.RemoveAll(this);
	}

	ClearPresetComboList();
}

void FDungeonToolsEditorModeToolkit::OnActiveViewportChanged(TSharedPtr<IAssetViewport> OldViewport, TSharedPtr<IAssetViewport> NewViewport)
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

void FDungeonToolsEditorModeToolkit::UpdateCategoryButtonLabelVisibility(UObject* Obj, FPropertyChangedEvent& ChangeEvent)
{
	UDungeonToolsModeCustomizationSettings* UISettings = GetMutableDefault<UDungeonToolsModeCustomizationSettings>();
	ToolkitBuilder->SetCategoryButtonLabelVisibility(UISettings != nullptr  ? UISettings->bShowCategoryButtonLabels : true);
	ToolkitBuilder->RefreshCategoryToolbarWidget();
}



#undef LOCTEXT_NAMESPACE

