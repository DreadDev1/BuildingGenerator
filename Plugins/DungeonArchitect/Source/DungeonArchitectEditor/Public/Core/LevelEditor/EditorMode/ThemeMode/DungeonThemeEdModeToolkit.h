//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/Widgets/SMarkerListView.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolkit.h"

#include "StatusBarSubsystem.h"

class ADungeon;
class UDungeonThemeAsset;
class FThemeGraphEditorImpl;

class FDungeonThemeEdModeToolkit : public FModeToolkit
{
public:
	FDungeonThemeEdModeToolkit();
	
	// FModeToolkit interface 
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	virtual void InvokeUI() override;
	
	virtual void RequestModeUITabs() override;

	// IToolkit interface 
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;

	// set/clear notification message area
	virtual void PostNotification(const FText& Message);
	virtual void ClearNotification();
	
	virtual void OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	virtual void OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	void OnActiveViewportChanged(TSharedPtr<IAssetViewport> OldViewport, TSharedPtr<IAssetViewport> NewViewport) const;
	
	virtual FText GetActiveToolDisplayName() const override { return ActiveToolName; }
	virtual FText GetActiveToolMessage() const override { return ActiveToolMessage; }
	
	void ShowObjectDetails(const TArray<UObject*>& InObjects, bool bForceRefresh = false) const;
	TSharedPtr<IDetailsView> GetPropertyEditor() const { return PropertyEditor; }
	void RefreshMarkerListView() const;

private:
	void OnThemeSelectionChanged(ADungeon* InDungeon, UDungeonThemeAsset* InThemeAsset) const;
	void OnDungeonBuildClicked() const;
	void OnPerformLayoutClicked() const;

	TSharedRef<SDockTab> CreateThemeGraphTab(const FSpawnTabArgs& Args);
	void OnMarkerListDoubleClicked(const TSharedPtr<FMarkerListEntry>& Entry) const;
	
private:
	TSharedPtr<SWidget> ContentBody;
	TSharedPtr<IDetailsView> PropertyEditor;
	TSharedPtr<SMarkerListView> MarkerListView;
	
	TWeakPtr<SDockTab> ThemeGraphTab;
	FMinorTabConfig ThemeGraphTabInfo;
	TSharedPtr<SBox> ThemeGraphEditorHost;
	
	bool bInActiveTool = false;
	FText ActiveToolName;
	FText ActiveToolMessage;
	FStatusBarMessageHandle ActiveToolMessageHandle;
	
	const FSlateBrush* ActiveToolIcon = nullptr;
};

