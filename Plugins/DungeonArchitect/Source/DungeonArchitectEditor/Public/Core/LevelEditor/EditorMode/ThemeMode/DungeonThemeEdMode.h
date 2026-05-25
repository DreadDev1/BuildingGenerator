//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeSelection.h"
#include "Core/Utils/DeferredExecutor.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"
#include "DungeonThemeEdMode.generated.h"

class ADungeon;
class UEdGraphNode;
class UDungeonThemeAsset;
class FThemeGraphEditorImpl;
class FDungeonThemeEdModeToolkit;
class UDungeonEditorViewportCapture;
class UDungeonThemeEdModeToolBase;
class UEdGraphNode_DungeonThemeActorBase;
class UEdGraphNode_DungeonThemeMarker;
class UDungeonThemeEdModeMarkerNodeToolBuilder;
struct FDungeonBuildSettings;


UCLASS()
class UDungeonThemeEdMode : public UBaseLegacyWidgetEdMode
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_DungeonTheme;

	static FString GridToolName;
	static FString TextureToolName;

	UDungeonThemeEdMode();
	virtual ~UDungeonThemeEdMode() override;
	virtual void ModeTick(float DeltaTime) override;

	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;
	void SetThemeGraphState(ADungeon* InDungeon, UDungeonThemeAsset* InThemeAsset);
	TSharedPtr<FThemeGraphEditorImpl> GetThemeGraphEditor() const { return ThemeGraphEditorImpl; }
	virtual bool Select(AActor* InActor, bool bInSelected) override;
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override;

	virtual void OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	 
	/** UEdMode interface */
	virtual void Initialize() override;
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void CreateToolkit() override;
	
	virtual EEditAction::Type GetActionEditCut() override { return EEditAction::Process; }
	virtual EEditAction::Type GetActionEditDelete() override { return EEditAction::Process; }
	virtual EEditAction::Type GetActionEditDuplicate() override { return EEditAction::Process; }
	virtual bool ProcessEditCut() override;
	virtual bool ProcessEditDelete() override;
	virtual bool ProcessEditDuplicate() override;
	virtual void ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, bool bOffsetLocations) override;
	
	ADungeon* GetActiveDungeon() const;
	UDungeonThemeAsset* GetThemeAsset() const;
	void RequestFocusOnThemeNode(UEdGraphNode* InNode) const;
	void RequestDeleteThemeNode(UEdGraphNode_DungeonThemeActorBase* VisualNode, bool bRecompile = true);
	void PerformGraphLayout() const;
	void PerformGraphLayoutNextFrame();
	void OnPreSaveWorld(UWorld* World, FObjectPreSaveContext ObjectPreSaveContext) const;

	UDungeonThemeEdModeToolBase* GetActiveTool() const;
	void RequestActivateSelectTool();
	void RequestActivateVisualNodeEdTool(UEdGraphNode_DungeonThemeActorBase* InThemeNode);
	void RequestActivateMarkerNodeEdTool(UEdGraphNode_DungeonThemeMarker* InMarkerNode);
	
	void BuildActiveDungeon(const FDungeonBuildSettings& InBuildSettings);
	bool CompileThemeGraph(TArray<FDungeonThemeGraphBuildError>& OutCompileErrors, bool bAutoSaveAsset = false) const;
	const FDungeonThemeEdModeActorSelection& GetActorSelection() const { return ActorSelection; }
	FDungeonThemeEdModeActorSelection& GetMutableActorSelection() { return ActorSelection; }

	FDADeferredExecutor& GetDeferredExecutor() { return DeferredExecutor; }

	void ShowObjectDetails(const TArray<UObject*>& InObjects, bool bForceRefresh = false) const;
	
protected:
	virtual TSharedRef<FLegacyEdModeWidgetHelper> CreateWidgetHelper() override;
	
private:
	void NotifyGraphChange(const FEdGraphEditAction& Action);
	void NodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent);
	void SelectedNodesChanged(const TSet<UObject*>& SelectedNodes);
	void NodeDoubleClicked(class UEdGraphNode* Node) const;
	void ExecuteNextFrame(const TFunction<void(UDungeonThemeEdMode*)>& InCallback);
	void OnNewActorsDropped(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& DroppedActors);
	void ActivateSelectTool() const;
	void ActivateVisualNodeEdTool(UEdGraphNode_DungeonThemeActorBase* InThemeNode);
	void ActivateMarkerTool(UEdGraphNode_DungeonThemeMarker* InMarkerNode) const;

	UFUNCTION()
	void OnDungeonBuildComplete(ADungeon* Dungeon, bool bSuccess);

	UFUNCTION()
	void OnDungeonPropertyChanged(ADungeon* InDungeon, const FName& InPropertyName);
	
private:
	TSharedPtr<FDungeonThemeEdModeToolkit> ToolkitImpl;
	TSharedPtr<FThemeGraphEditorImpl> ThemeGraphEditorImpl;

	TWeakObjectPtr<ADungeon> ActiveDungeon;
	TWeakObjectPtr<UDungeonThemeAsset> ActiveThemeAsset;
	FDungeonThemeEdModeActorSelection ActorSelection{};
	FDADeferredExecutor DeferredExecutor{};
	
	UPROPERTY()
	TObjectPtr<UDungeonThemeEdModeMarkerNodeToolBuilder> MarkerNodeToolBuilder;
};


class FDungeonThemeEdModeWidgetHelper : public FLegacyEdModeWidgetHelper {
public:
	virtual bool UsesTransformWidget() const override;
	

private:
	UDungeonThemeEdModeToolBase* GetActiveTool() const;
};

