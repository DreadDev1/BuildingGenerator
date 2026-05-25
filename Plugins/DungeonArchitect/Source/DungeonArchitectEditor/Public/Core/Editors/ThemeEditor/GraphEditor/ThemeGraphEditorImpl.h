//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "Types/SlateEnums.h"

class SGraphEditor;
class IDetailsView;
class FDragDropEvent;
class UEdGraphNode_DungeonThemeActorBase;
class SThemeEditorDropTarget;
struct FDungeonBuildSettings;

typedef TSharedPtr<class FDungeonArchitectThemeGraphHandler> FDungeonArchitectThemeEditorActionsPtr;

struct FThemeGraphEditorImplSettings {
	TWeakObjectPtr<UEdGraph> ThemeGraph;
	TWeakPtr<IDetailsView> PropertyEditor;
};

class FThemeGraphEditorImpl : public TSharedFromThis<FThemeGraphEditorImpl> {
public:
	~FThemeGraphEditorImpl();

	void Init(const FThemeGraphEditorImplSettings& InSettings);
	void Tick(float DeltaTime);
	void ShowObjectDetails(UObject* ObjectProperties, bool bForceRefresh = false) const;
	void SetBuilderClass(TSubclassOf<class UDungeonBuilder> InBuilderClass);
	
    void SetEditorDefaultPropertiesGetter(const TFunction<UObject*()>& InFuncGetEditorProperties);
	void SelectAndFocusOnNode(UEdGraphNode* InNode) const;
	void SetIsEditable(bool bInEnabled);
	bool IsEditable() const;
	
	TSharedPtr<SGraphEditor> GetGraphEditor() const { return GraphEditor; }
	TSharedPtr<SWidget> GetGraphEditorHost() const { return GraphEditorHost; }

	DECLARE_DELEGATE_OneParam(FSingleNodeDelegate, UEdGraphNode*);
	DECLARE_DELEGATE_OneParam(FMultiNodeDelegate, const TSet<UObject*>&);
	DECLARE_DELEGATE_OneParam(FBuildDungeonDelegate, const FDungeonBuildSettings&);
	
	FMultiNodeDelegate& GetOnSelectedNodesChanged() { return OnSelectedNodesChanged; }
	FSingleNodeDelegate& GetOnNodeDoubleClicked() { return OnNodeDoubleClicked; }
	FOnGraphChanged::FDelegate& GetOnGraphChanged() { return OnGraphChanged; }
	FOnThemeGraphPropertyChanged::FDelegate& GetOnNodePropertyChanged() { return OnNodePropertyChanged; }
	FBuildDungeonDelegate& GetOnBuildDungeon() { return OnBuildDungeon; }
	
private:
	void HandleAssetsDropped(const FDragDropEvent& InEvent, TArrayView<FAssetData> InAssets) const;
	bool AreAssetsAcceptableForDrop(TArrayView<FAssetData> InAssets, FText& OutReason) const;
	void NodeDoubleClicked(class UEdGraphNode* Node) const;
	void SelectedNodesChanged(const TSet<UObject*>& SelectedNodes) const;
	void NotifyGraphChange(const FEdGraphEditAction& Action);
	void NotifyBuildDungeon(const FDungeonBuildSettings& InBuildSettings);
	void NodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent);
	void InitThemeGraph(class UEdGraph_DungeonTheme* ThemeGraph) const;
	void CreateGraphEditorWidget(UEdGraph* InGraph);
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
	FVector2D GetAssetDropGridLocation() const;
	
	bool DisplayAsReadOnly() const;

private:
	// Graph Editor Widgets
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<SWidget> GraphEditorHost;
	TSharedPtr<SThemeEditorDropTarget> AssetDropTarget;

	// Graph Handlers
	FDelegateHandle OnGraphChangedDelegateHandle;
	FDelegateHandle OnNodePropertyChangedDelegateHandle;
	FDungeonArchitectThemeEditorActionsPtr ThemeGraphHandler;
	bool bGraphStateChanged = false;
	bool bGraphIsEditable = true;
	bool bNeedsFullRebuild = false;

	FThemeGraphEditorImplSettings Settings;
	TFunction<UObject*()> EditorDefaultPropertiesGetter = [](){ return nullptr; };

	FMultiNodeDelegate OnSelectedNodesChanged;
	FSingleNodeDelegate OnNodeDoubleClicked;
	FOnGraphChanged::FDelegate OnGraphChanged;
	FOnThemeGraphPropertyChanged::FDelegate OnNodePropertyChanged;
	FBuildDungeonDelegate OnBuildDungeon;
};

