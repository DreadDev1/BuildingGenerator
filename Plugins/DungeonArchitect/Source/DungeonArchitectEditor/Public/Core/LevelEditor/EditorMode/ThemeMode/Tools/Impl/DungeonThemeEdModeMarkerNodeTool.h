//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBase.h"
#include "Core/Markers/DungeonMarkerVisualization.h"
#include "DungeonThemeEdModeMarkerNodeTool.generated.h"

class SDADraggableToolOverlayWidget;
class UEdGraphNode_DungeonThemeMarker;
class UMaterialInterface;
class FModeToolkit;
class IAssetViewport;
class SWidget;

UCLASS()
class UDungeonThemeEdModeMarkerNodeTool : public UDungeonThemeEdModeToolBase {
	GENERATED_BODY()
public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual bool IsActorSelectionAllowed(AActor* Actor, bool bInSelection) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) override;
	virtual void RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) override;
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes) override;
	virtual void OnAssetDragDrop(UObject* InAssetObject) override;

	static const FString ToolIdentifier;
	
protected:
	virtual UMaterialInterface* GetPostProcessMaterialTemplate() const override;
	
	float GetHighlightSaturation() const;
	void SetHighlightSaturation(float InValue);
	void SaveHighlightSaturationUserSetting() const;
	void ZoomInOnNearestMarker();
	
private:
	void SetMarkerNode(UEdGraphNode_DungeonThemeMarker* InMarkerNode);

private:
	TWeakObjectPtr<UEdGraphNode_DungeonThemeMarker> MarkerNode;
	FDungeonMarkerVisualizer MarkerVisualizer{};
	FDungeonMarkerVisualizationRules MarkerVisualizationRules{};
	TSharedPtr<SWidget> ToolShutdownViewportOverlayWidget;
	TSharedPtr<SDADraggableToolOverlayWidget> ToolOverlayWidget;

	float VisualHighlightSaturation = 1.0f;
	bool bNodeSelectionGuard = false;
	bool bMarkerInfoValid = false;
	FDungeonMarkerInstance MarkerInfo;
	
	friend class UDungeonThemeEdModeMarkerNodeToolBuilder;
};
