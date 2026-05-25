//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgeToolBase.h"

#include "BaseGizmos/GizmoInterfaces.h"
#include "GizmoEdModeInterface.h"
#include "InteractiveToolBuilder.h"
#include "DungeonForgeTransformTool.generated.h"

class UTransformProxy;
class UCombinedTransformGizmo;
class UDragAlignmentMechanic;

class IDungeonForgeTransformToolHandler;
class UDungeonForgeNodeSettings;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonForgeTransformToolBuilder : public UDungeonForgeToolBuilderBase
{
	GENERATED_BODY()
public:
	UDungeonForgeTransformToolBuilder();
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
	void SetNodeSettings(UDungeonForgeNodeSettings* InNodeSettings);

private:
	TWeakObjectPtr<UDungeonForgeNodeSettings> NodeSettings;
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonForgeTransformTool : public UDungeonForgeToolBase {
	GENERATED_BODY()
	
public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void OnTick(float DeltaTime) override;
	

	void SetTargetNodeSettings(UDungeonForgeNodeSettings* InNodeSettings);

protected:
	UPROPERTY()
	TWeakObjectPtr<UDungeonForgeNodeSettings> TargetNodeSettings;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> Gizmo;

	UPROPERTY()
	TObjectPtr<UDragAlignmentMechanic> DragAlignmentMechanic = nullptr;
	
	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy;

	FTransform CurrentTransform = FTransform::Identity;
	bool bIsDragging = false;
	FGizmoState CurrentGizmoState;

	void UpdateGizmoTransform();
	void OnTransformProxyChanged(UTransformProxy* Proxy, FTransform NewTransform);
	void OnBeginProxyDrag(UTransformProxy* Proxy);
	void OnEndProxyDrag(UTransformProxy* InProxy);
};

