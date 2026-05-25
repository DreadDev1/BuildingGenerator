//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonLegacyEdModeToolkit.h"

#include "AssetRegistry/AssetData.h"
#include "Misc/NotifyHook.h"
#include "StatusBarSubsystem.h"

class UEdGraphNode;
class SGraphEditor;
class ADungeon;
class UDungeonForgeEditorMode;
class UDungeonForgeGraph;
class UDungeonForgeEditorGraph;
class FDungeonForgeOutliner;
class UDungeonForgeLayerHierarchy;
class STransformGizmoNumericalUIOverlay;

struct FDungeonForgeTabsID {
	static const FName DetailsView;
};


class FDungeonForgeEditorModeToolkit : public FModeToolkit, public FNotifyHook
{
public:
	FDungeonForgeEditorModeToolkit();
	virtual ~FDungeonForgeEditorModeToolkit() override;

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;
	virtual FText GetToolPaletteDisplayName(FName InPaletteName) const override;
	virtual bool HasIntegratedToolPalettes() const override { return false; }
	virtual bool HasExclusiveToolPalettes() const override { return false; }
	virtual void OnToolStarted(UInteractiveToolManager* InManager, UInteractiveTool* InTool) override;
	virtual void OnToolEnded(UInteractiveToolManager* InManager, UInteractiveTool* InTool) override;
	virtual void InvokeUI() override;
	virtual FText GetActiveToolDisplayName() const override { return ActiveToolName; }
	virtual FText GetActiveToolMessage() const override { return ActiveToolMessage; }
	virtual void CustomizeModeDetailsViewArgs(FDetailsViewArgs& ArgsInOut) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	/** IToolkit interface */
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged ) override;

	// set/clear notification message area
	virtual void PostNotification(const FText& Message);
	virtual void ClearNotification();
	
	// set/clear warning message area
	virtual void PostWarning(const FText& Message);
	virtual void ClearWarning();
	
	void SetSelectedObject(UObject* InObject) const;
	void SetSelectedObjects(TArray<UObject*> InObjects) const;

	/** Select every node in the graph */
	void SelectAllNodes();
	/** Whether we can select every node */
	bool CanSelectAllNodes() const;

	/** Delete all selected nodes in the graph */
	void DeleteSelectedNodes();
	/** Whether we can delete all selected nodes */
	bool CanDeleteSelectedNodes() const;

	/** Copy all selected nodes in the graph */
	void CopySelectedNodes();
	/** Whether we can copy all selected nodes */
	bool CanCopySelectedNodes() const;

	/** Cut all selected nodes in the graph */
	void CutSelectedNodes();
	/** Whether we can cut all selected nodes */
	bool CanCutSelectedNodes() const;

	/** Paste nodes in the graph */
	void PasteNodes();
	/** Paste nodes in the graph at location*/
	void PasteNodesHere(const FVector2D& Location);
	/** Whether we can paste nodes */
	bool CanPasteNodes() const;

	/** Duplicate the currently selected nodes */
	void DuplicateNodes();
	/** Whether we are able to duplicate the currently selected nodes */
	bool CanDuplicateNodes() const;

	FString GetForgeAssetPath() const;
	void OnForgeAssetChanged(const FAssetData& InAssetData);
	void SpawnDungeonForgeActor();
	
	void BindGizmoNumericalUI() const;

protected:
	//~ Begin FModeToolkit
	virtual void RequestModeUITabs() override;
	//~ End FModeToolkit

	TSharedRef<SDockTab> CreateDetailsViewTab(const FSpawnTabArgs& Args);
	UDungeonForgeEditorMode* GetForgeEditorMode() const;
		
	void OnAlignTop();
	void OnAlignMiddle();
	void OnAlignBottom();
	void OnAlignLeft();
	void OnAlignCenter();
	void OnAlignRight();
	void OnStraightenConnections();
	void OnDistributeNodesH();
	void OnDistributeNodesV();
	void OnCreateComment();

	void HandleBuildButtonClicked() const;
	
	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection);

	/** Called when the title of a node is changed */
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
	
	/**
	 * Called when a node is double clicked
	 *
	 * @param Node - The Node that was clicked
	 */
	void OnNodeDoubleClicked(UEdGraphNode* Node);
	
	
private:
	/** A utility function to register the tool palettes with the ToolkitBuilder */
	void RegisterPalettes();
	void UpdateGraphWidgetHost();
	ADungeon* GetDungeonActor() const;
	
	void UpdateActiveToolProperties();
	void InvalidateCachedDetailPanelState(UObject* ChangedObject);

	TObjectPtr<UDungeonForgeEditorGraph> GetForgeEditorGraph();
	TObjectPtr<UDungeonForgeEditorGraph> GetForgeEditorGraph(UDungeonForgeGraph* InForgeGraph);

	FText GetToolWarningText() const;
	void OnActiveViewportChanged(TSharedPtr<IAssetViewport>, TSharedPtr<IAssetViewport> );
	void MakeToolShutdownOverlayWidget();

	/** Create new graph editor widget */
	TSharedRef<SGraphEditor> CreateGraphEditorWidget();
	
	void UnregisterAndRemoveFloatingTabs() const;
	
	/** Toggle node enabled state for selected nodes */
	void OnToggleEnabled();
	/** Whether we can toggle enabled state of selected nodes */
	bool CanToggleEnabled() const;
	/** Whether selected nodes are enabled or not */
	ECheckBoxState GetEnabledCheckState() const;
	
	/** Toggle node debug state for selected nodes */
	void OnToggleDebug();
	/** Whether we can toggle debug state of selected nodes */
	bool CanToggleDebug() const;
	/** Whether selected nodes are being debugged or not */
	ECheckBoxState GetDebugCheckState() const;
	
private:
	bool bInActiveTool = false;
	FText ActiveToolName;
	FText ActiveToolMessage;
	FStatusBarMessageHandle ActiveToolMessageHandle;
	const FSlateBrush* ActiveToolIcon = nullptr;
	TSharedPtr<SWidget> ToolShutdownViewportOverlayWidget;
	TSharedPtr<STransformGizmoNumericalUIOverlay> GizmoNumericalUIOverlayWidget;
	
	TSharedPtr<STextBlock> ModeWarningArea;
	TSharedPtr<STextBlock> ToolWarningArea;
	
	TSharedPtr<SBorder> ForgeGraphHost;

	TWeakPtr<SDockTab> DetailsViewTab;
	
	TSharedPtr<FUICommandList> GraphEditorCommands;
	TSharedPtr<SGraphEditor> GraphEditorWidget;
	
	TWeakObjectPtr<UDungeonForgeEditorGraph> ForgeEditorGraph = nullptr;

	FAssetData ForgeAssetSearchWidgetPath;
};

