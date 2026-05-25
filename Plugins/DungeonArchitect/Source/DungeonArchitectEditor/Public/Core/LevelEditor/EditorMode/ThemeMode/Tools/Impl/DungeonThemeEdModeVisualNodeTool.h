//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBase.h"
#include "Core/Markers/DungeonMarkerVisualization.h"

#include "Camera/CameraTypes.h"
#include "DungeonThemeEdModeVisualNodeTool.generated.h"

class SDADraggableToolOverlayWidget;
class UDungeonEditorViewportCapture;

UCLASS()
class UDungeonThemeEdModeVisualNodeTool
	: public UDungeonThemeEdModeToolBase
{
	GENERATED_BODY()
public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) override;
	virtual void RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) override;
	virtual void OnSelectedActorChanged() override;
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes) override;
	virtual bool UsesTransformWidget() override { return true; }
	virtual void OnAssetDragDrop(UObject* InAssetObject) override;
	virtual bool ProcessDeleteSelectedActor() override;
	void RefreshSelectedActor();
	virtual void OnDungeonBuildComplete() override;
	virtual void OnNodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent) override;

	static const FString ToolIdentifier;

protected:
	void UpdateThemeNodeTransform() const;
	float GetHighlightSaturation() const;
	void SetHighlightSaturation(float InValue);
	void SaveHighlightSaturationUserSetting() const;
	void CaptureOffscreenHighlightActors(const UWorld* InTargetWorld, const FMinimalViewInfo& ViewInfo, const FIntPoint& ViewSize) const;
	void SetActorBeingEdited(AActor* InActor);

	virtual UMaterialInterface* GetPostProcessMaterialTemplate() const override;
	
	UFUNCTION()
	void HandleActorsMoved(TArray<AActor*>& InActors) const;
	
protected:
	UPROPERTY()
	TObjectPtr<UDungeonEditorViewportCapture> ViewportCapture;
	
	TSharedPtr<SWidget> ToolShutdownViewportOverlayWidget;
	TSharedPtr<SDADraggableToolOverlayWidget> ToolOverlayWidget;
	
	TWeakObjectPtr<AActor> ActorBeingEdited;

	float VisualHighlightSaturation = 1.0f;
	
	FDungeonMarkerVisualizationRules MarkerVisualizationRules{};
	FDungeonMarkerVisualizer MarkerVisualizer{};

	friend class UDungeonThemeEdModeVisualNodeToolBuilder;
};

