//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "BaseTools/SingleClickTool.h"
#include "InteractiveToolBuilder.h"
#include "InteractiveToolChange.h"
#include "InteractiveToolQueryInterfaces.h"
#include "DungeonEdTool.generated.h"

class UCombinedTransformGizmo;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonEdTool : public UInteractiveTool {
	GENERATED_BODY()
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonEdToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

public:
	UPROPERTY()
	TSubclassOf<UDungeonEdToolBase> ToolClass;
};


UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonEdToolBase : public USingleClickTool, public IHoverBehaviorTarget, public IInteractiveToolCameraFocusAPI {
	GENERATED_BODY()

public:
	virtual void SetWorld(UWorld* World);

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	// USingleClickTool
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;

	// IHoverBehaviorTarget interface
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

	// IInteractiveToolCameraFocusAPI implementation
	virtual bool SupportsWorldSpaceFocusBox() override;
	virtual FBox GetWorldSpaceFocusBox() override;
	virtual bool SupportsWorldSpaceFocusPoint() override;
	virtual bool GetWorldSpaceFocusPoint(const FRay& WorldRay, FVector& PointOut) override;

	
protected:
	enum class EState
	{
		PlacingPrimitive,
		AdjustingSettings
	};

	EState CurrentState = EState::PlacingPrimitive;
	void SetState(EState NewState);
	
	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> Gizmo = nullptr;

	TWeakObjectPtr<UWorld> TargetWorld{};

	// Used to make the initial placement of the mesh undoable
	class FStateChange : public FToolCommandChange
	{
	public:
		FStateChange(const FTransform& MeshTransformIn)
			: MeshTransform(MeshTransformIn)
		{
		}

		virtual void Apply(UObject* Object) override;
		virtual void Revert(UObject* Object) override;
		virtual FString ToString() const override
		{
			return TEXT("UDungeonEdToolBase::FStateChange");
		}

	protected:
		FTransform MeshTransform;
	};
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonGridPaintEdTool : public UDungeonEdToolBase {
	GENERATED_BODY()
	
public:
	
	
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonSnapStitchEdTool : public UDungeonEdToolBase {
	GENERATED_BODY()
	
public:
	
	
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonSnapConnectionEdTool : public UDungeonEdToolBase {
	GENERATED_BODY()
	
public:
	
	
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonFlowNodeEdTool : public UDungeonEdToolBase {
	GENERATED_BODY()
	
public:
	
	
};

