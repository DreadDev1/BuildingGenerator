//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeSelection.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "GraphEditAction.h"
#include "InteractiveTool.h"
#include "DungeonThemeEdModeToolBase.generated.h"

class IAssetViewport;
class FModeToolkit;
class UTransformProxy;
class UCombinedTransformGizmo;
class UDungeonThemeEdMode;
class UEdGraphNode_DungeonThemeBase;
class UDungeonScenePostProcess;

UCLASS()
class UDungeonThemeEdModeToolBase
	: public UInteractiveTool
	, public IClickBehaviorTarget
{
	GENERATED_BODY()
public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	
	virtual void SetWorld(UWorld* InWorld);
	virtual bool IsActorSelectionAllowed(AActor* Actor, bool bInSelection);
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes) {}
	virtual void OnSelectedActorChanged() {}
	virtual void OnDungeonBuildComplete();

	virtual void AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) {}
	virtual void RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) {}
	virtual void OnTick(float DeltaTime) override;

	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void OnActorDoubleClicked(AActor* InActor) {}
	virtual void OnThemeNodeDoubleClicked(UEdGraphNode_DungeonThemeBase* InThemeNode) {}
	virtual void OnAssetDragDrop(UObject* InAssetObject) {}
	virtual void OnNodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent) {}

	virtual bool UsesTransformWidget() { return false; }
	virtual bool ProcessDeleteSelectedActor();

	void SetEdMode(const TWeakObjectPtr<UDungeonThemeEdMode>& InEdMode);
	void HandleNewActorsDropped(const TArray<AActor*> InDroppedActors);
	void HandleSelectedNodesChanged(const TSet<UObject*>& SelectedNodes);

protected:
	void SetupGizmo();
	void DestroyGizmo();
	void SetGizmoTarget(USceneComponent* InSceneComponent);
	void SetGizmoVisibility(bool bInVisible) const;
	FDungeonThemeEdModeActorSelection GetActorSelection() const;

	
protected:
	virtual UMaterialInterface* GetPostProcessMaterialTemplate() const { return nullptr; }
	
protected:
	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo;
	
	UPROPERTY()
	TObjectPtr<UDungeonScenePostProcess> PostProcessor;
	
protected:
	TWeakObjectPtr<UWorld> TargetWorld;
	TWeakObjectPtr<UDungeonThemeEdMode> EdMode;
};

