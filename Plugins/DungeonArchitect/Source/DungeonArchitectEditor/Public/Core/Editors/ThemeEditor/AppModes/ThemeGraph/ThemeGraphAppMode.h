//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeEditorAppModeBase.h"

#include "Framework/Commands/Commands.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

struct FEdGraphEditAction;
class SGraphEditor;
class FThemeGraphEditorImpl;
class SThemePreviewViewport;
class SGraphPalette_PropActions;
class SMarkerListView;
struct FMarkerListEntry;
class SAdvancedPreviewDetailsTab;
class UDungeonThemeAsset;
struct FDungeonBuildSettings;

class FThemeGraphAppMode : public FThemeEditorAppModeBase {
public:
	FThemeGraphAppMode(TSharedPtr<FDungeonArchitectThemeEditor> InThemeEditor, TSharedPtr<SThemePreviewViewport> InPreviewViewport);
	virtual void Init(TSubclassOf<UDungeonBuilder> InBuilderClass) override;
	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;
	virtual void Tick(float DeltaTime) override;
	
	void ShowObjectDetails(UObject* ObjectProperties, bool bForceRefresh = false) const;
	void SetBuilderClass(TSubclassOf<class UDungeonBuilder> InBuilderClass);
	TSharedPtr<SGraphEditor> GetGraphEditor() const;

	void RecreateDefaultMarkerNodes(const TSubclassOf<UDungeonBuilder>& InBuilderClass) const;

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged ) override;
	//~ End FNotifyHook Interface

	void HandlePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent) const;

private:
	void BindCommands(TSharedRef<FUICommandList> ToolkitCommands);
	void HandleBuildFullDungeon() const;
	void HandleBuildDungeon(const FDungeonBuildSettings& InBuildSettings) const;
	void RefreshMarkerListView() const;
	void OnMarkerListDoubleClicked(const TSharedPtr<FMarkerListEntry>& Entry) const;
	
	TSharedPtr<IDetailsView> FindLevelEditorPropertyEditor() const;
	void NotifyGraphChange(const FEdGraphEditAction& Action);
	void NodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent);
	void SelectedNodesChanged(const TSet<UObject*>& SelectedNodes) const;
	void NodeDoubleClicked(class UEdGraphNode* Node) const;
	
	
	UDungeonThemeAsset* GetThemeAsset() const;
	
private:
	// App mode managed objects
	FWorkflowAllowedTabSet TabFactories;
	TSharedPtr<SGraphPalette_PropActions> ActionPalette;
	TSharedPtr<SMarkerListView> MarkerListView;
	TSharedPtr<IDetailsView> PropertyEditor;
	TWeakPtr<SThemePreviewViewport> PreviewViewportPtr;
	TSharedPtr<SWidget> PreviewSettingsWidget;

	TSharedPtr<FThemeGraphEditorImpl> EditorImpl;

};

class DUNGEONARCHITECTEDITOR_API FThemeGraphAppModeCommands : public TCommands<FThemeGraphAppModeCommands> {
public:
	FThemeGraphAppModeCommands();
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> Build;
};



