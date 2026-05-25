//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeEditorAppModeBase.h"
#include "Frameworks/GraphGrammar/Editor/SEditableListView.h"
#include "Frameworks/MarkerGenerator/MarkerGenLayer.h"
#include "Frameworks/MarkerGenerator/MarkerGenPattern.h"

#include "WorkflowOrientedApp/WorkflowTabManager.h"

class SGraphEditor;
struct FEdGraphEditAction;
class IMGPatternEditor;
class IMGAppModeEditorInterface;
class SThemePreviewViewport;
class SMGPatternEditorWidget;
class UMarkerGenLayer;
class UMarkerGenModel;
class UMGPatternGraph;
class FMGPatternGraphHandler;

class IMGAppModeInterface {
public:
	virtual ~IMGAppModeInterface() {}
	virtual TSharedPtr<IMGPatternEditor> CreatePatternEditor() = 0;
};

class FMarkerGeneratorAppMode : public FThemeEditorAppModeBase {
public:
	FMarkerGeneratorAppMode(TSharedPtr<FDungeonArchitectThemeEditor> InThemeEditor, TSharedPtr<SThemePreviewViewport> InPreviewViewport);
	virtual void Init(TSubclassOf<UDungeonBuilder> InBuilderClass) override;
	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;
	virtual void SetBuilderClass(TSubclassOf<UDungeonBuilder> InBuilderClass) override;

	void UpdatePatternRuleGraphEditor();

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged ) override;
	//~ End FNotifyHook Interface

	
	////////// Layer ListView Handlers //////////
	void OnLayerSelectionChanged(TObjectPtr<UMarkerGenLayer> Item, ESelectInfo::Type SelectInfo);
	FText GetLayerListRowText(TObjectPtr<UMarkerGenLayer> InItem) const;
	FText GetLayerListRowFeatureText(TObjectPtr<UMarkerGenLayer> InItem) const;
	const TArray<TObjectPtr<UMarkerGenLayer>>* GetLayerList() const;
	void OnLayerDelete(TObjectPtr<UMarkerGenLayer> InItem);
	void OnLayerAdd() const;
	void OnLayerReordered(TObjectPtr<UMarkerGenLayer> Source, TObjectPtr<UMarkerGenLayer> Dest) const;
	EVisibility IsLayerListRowFeatureVisible(TObjectPtr<UMarkerGenLayer> InLayer) const;
	FSlateColor GetLayerListRowColor(TObjectPtr<UMarkerGenLayer> InLayer) const;
	TSharedPtr<SWidget> CreateLayerListItemWidget(TObjectPtr<UMarkerGenLayer> InLayer);
	////////// End of ListView Handlers //////////

private:
	void BindCommands(TSharedRef<FUICommandList> ToolkitCommands);
	UMarkerGenLayer* CreateNewLayer(UObject* InOuter, const FText& InLayerName) const;
	UMarkerGenModel* GetMarkerGenerationModel() const;
	void SetActiveLayer(UMarkerGenLayer* InLayer);
	void SetActivePatternRuleGraph(UMarkerGenPatternRule* InPatternRule);
	void HandleRulePositionChanged(UMarkerGenPatternRule* InPatternRule) const;
	void OnRuleGraphChanged(const FEdGraphEditAction& InAction) const;
	void OnRuleGraphPropertyChanged(const FPropertyChangedEvent& InEvent, const FString& InPropertyName) const;
	void OnRuleGraphUpdated() const;
	FText GetPatternEditorInvalidMessage() const;
	void RebuildPreviewScene() const;
	void Compile() const;
	
private:
	TSharedPtr<IDetailsView> PropertyEditor;
	TSharedPtr<SMGPatternEditorWidget> PatternEditor;
	TSharedPtr<SBox> PatternEditorHost;
	TSharedPtr<SWidget> PatternEditorInvalidMessage;
	TSharedPtr<SEditableListView<TObjectPtr<UMarkerGenLayer>>> PatternList;
	TWeakPtr<SThemePreviewViewport> PreviewViewportPtr;
	TSharedPtr<SBox> PatternRuleGraphEditorHost;
	TSharedPtr<SGraphEditor> PatternRuleGraphEditor;
	TSharedPtr<FMGPatternGraphHandler> PatternRuleGraphHandler;
	TWeakObjectPtr<UMarkerGenPatternRule> SelectedPatternRule;
	FDelegateHandle OnGraphChangedHandle;
	FDelegateHandle OnGraphPropertyChangedHandle;

	FWorkflowAllowedTabSet TabFactories;
	TWeakObjectPtr<UMarkerGenLayer> ActiveLayer;

	TSharedPtr<IMGAppModeEditorInterface> AppModeImpl;
};

