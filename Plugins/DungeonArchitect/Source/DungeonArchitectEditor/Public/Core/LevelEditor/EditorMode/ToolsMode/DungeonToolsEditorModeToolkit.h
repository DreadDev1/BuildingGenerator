//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "StatusBarSubsystem.h"
#include "Toolkits/BaseToolkit.h"

class IAssetViewport;
class SEditableTextBox;
class STextComboBox;
namespace ETextCommit { enum Type : int; }

#define LOCTEXT_NAMESPACE "DungeonToolsEditorModeToolkit"

class STransformGizmoNumericalUIOverlay;
class IDetailsView;
class SButton;
class STextBlock;
class UInteractiveToolsPresetCollectionAsset;
class FRecentPresetCollectionProvider;
struct FAssetData;


struct FDungeonToolPresetOption
{
	FString PresetLabel;
	FString PresetTooltip;
	FSlateIcon PresetIcon;
	int32 PresetIndex;
	FSoftObjectPath PresetCollection;
};

class FDungeonToolsEditorModeToolkit : public FModeToolkit {
public:
	FDungeonToolsEditorModeToolkit();
	~FDungeonToolsEditorModeToolkit();

	
	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override;

	// initialize toolkit widgets that need to wait until mode is initialized/entered
	virtual void InitializeAfterModeSetup();

	// set/clear notification message area
	virtual void PostNotification(const FText& Message);
	virtual void ClearNotification();

	// set/clear warning message area
	virtual void PostWarning(const FText& Message);
	virtual void ClearWarning();

	/** Returns the Mode specific tabs in the mode toolbar **/ 
	virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const;
	virtual FText GetToolPaletteDisplayName(FName PaletteName) const; 
	virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder);
	virtual void OnToolPaletteChanged(FName PaletteName) override;
	virtual bool HasIntegratedToolPalettes() const { return false; }
	virtual bool HasExclusiveToolPalettes() const { return false; }

	virtual FText GetActiveToolDisplayName() const override { return ActiveToolName; }
	virtual FText GetActiveToolMessage() const override { return ActiveToolMessage; }

	virtual void ShowRealtimeAndModeWarnings(bool bShowRealtimeWarning);

	virtual void OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	virtual void OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;

	virtual void CustomizeModeDetailsViewArgs(FDetailsViewArgs& ArgsInOut) override;

	void OnActiveViewportChanged(TSharedPtr<IAssetViewport>, TSharedPtr<IAssetViewport> );

	virtual void InvokeUI() override;
	virtual void ExtendSecondaryModeToolbar(UToolMenu *InModeToolbarMenu) override {}

	virtual void ForceToolPaletteRebuild();

	// The mode is expected to call this after setting up the gizmo context object so that any subsequently
	// created gizmos are bound to the numerical UI.
	void BindGizmoNumericalUI();

	// @return true if we are currently in a tool, used to enable/disable UI things
	bool IsInActiveTool() const { return bInActiveTool; }

	// This is exposed only for the convenience of being able to create the numerical UI submenu
	// in a non-member function in ModelingModeToolkit_Toolbars.cpp
	TSharedPtr<STransformGizmoNumericalUIOverlay> GetGizmoNumericalUIOverlayWidget() { return GizmoNumericalUIOverlayWidget; }

private:
	const static TArray<FName> PaletteNames_Standard;

	bool bInActiveTool = false;
	FText ActiveToolName;
	FText ActiveToolMessage;
	FStatusBarMessageHandle ActiveToolMessageHandle;
	const FSlateBrush* ActiveToolIcon = nullptr;

	TSharedPtr<SWidget> ToolkitWidget;
	void UpdateActiveToolProperties();
	void InvalidateCachedDetailPanelState(UObject* ChangedObject);

	/** A utility function to register the tool palettes with the ToolkitBuilder */
	void RegisterPalettes();
	FDelegateHandle ActivePaletteChangedHandle;

	TSharedPtr<SWidget> ToolShutdownViewportOverlayWidget;
	void MakeToolShutdownOverlayWidget();
	
	void MakeGizmoNumericalUIOverlayWidget();
	TSharedPtr<STransformGizmoNumericalUIOverlay> GizmoNumericalUIOverlayWidget;

	TSharedRef<SWidget> MakeMenu_ModelingModeConfigSettings();

	TSharedPtr<STextBlock> ModeWarningArea;
	TSharedPtr<STextBlock> ModeHeaderArea;
	TSharedPtr<STextBlock> ToolWarningArea;
	TSharedPtr<SButton> AcceptButton;
	TSharedPtr<SButton> CancelButton;
	TSharedPtr<SButton> CompletedButton;

	FText ActiveWarning{};

	// Presets
	TSharedPtr<SWidget> ToolPresetArea;
	TSharedPtr<SWidget> MakePresetPanel();
	TSharedPtr<FAssetData> CurrentPreset;
	FSoftObjectPath CurrentPresetPath;
	FText CurrentPresetLabel;
	FString NewPresetLabel;
	FString NewPresetTooltip;
	FSlateIcon NewPresetIcon;

	TArray<TSharedPtr<FDungeonToolPresetOption>> AvailablePresetsForTool;
	TSharedPtr<SEditableComboBox<TSharedPtr<FString>>> PresetComboBox;
	TSharedPtr<FRecentPresetCollectionProvider> RecentPresetCollectionProvider;

	TSharedRef<SWidget> GetPresetCreateButtonContent();

	void CreateNewPresetInCollection(const FString& PresetLabel, FSoftObjectPath CollectionPath, const FString& ToolTip, FSlateIcon Icon);
	void LoadPresetFromCollection(const int32 PresetIndex, FSoftObjectPath CollectionPath);
	void UpdatePresetInCollection(const FDungeonToolPresetOption& PresetToEditIn, bool bUpdateStoredPresetValues);

	FString GetCurrentPresetPath() { return CurrentPresetPath.GetAssetPathString(); }
	void HandlePresetAssetChanged(const FAssetData& InAssetData);
	bool HandleFilterPresetAsset(const FAssetData& InAssetData);
	
	void RebuildPresetListForTool(bool bSettingsOpened);
	TSharedRef<SWidget> MakePresetComboWidget(TSharedPtr<FString> InItem);
	bool IsPresetEnabled() const;
	void ClearPresetComboList();

	/**
	 *  Updates the category button labels' visibility based off of the current editor preference for it
	 */
	void UpdateCategoryButtonLabelVisibility(UObject* Obj, FPropertyChangedEvent& ChangeEvent);

	TSharedPtr<FEditablePalette> FavoritesPalette;

	bool bFirstInitializeAfterModeSetup = true;
};

#undef LOCTEXT_NAMESPACE

