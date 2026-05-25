//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/Tools/DungeonEdTool.h"

#include "Engine/World.h"
#include "InteractiveToolManager.h"

bool UDungeonEdToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const {
	return ToolClass != nullptr;
}

UInteractiveTool* UDungeonEdToolBuilder::BuildTool(const FToolBuilderState& SceneState) const {
	if (ToolClass) {
		if (UDungeonEdToolBase* NewTool = NewObject<UDungeonEdToolBase>(SceneState.ToolManager, ToolClass)) {
			NewTool->SetWorld(SceneState.World);
			return NewTool;
		}
	}
	return nullptr;
}

void UDungeonEdToolBase::SetWorld(UWorld* World) {
	this->TargetWorld = World;
}

void UDungeonEdToolBase::Setup() {
	Super::Setup();
	
}

void UDungeonEdToolBase::Shutdown(EToolShutdownType ShutdownType) {
	Super::Shutdown(ShutdownType);
	
}

void UDungeonEdToolBase::Render(IToolsContextRenderAPI* RenderAPI) {
	Super::Render(RenderAPI);
	
}

bool UDungeonEdToolBase::CanAccept() const {
	return Super::CanAccept();
}

void UDungeonEdToolBase::OnPropertyModified(UObject* PropertySet, FProperty* Property) {
	Super::OnPropertyModified(PropertySet, Property);
} 

void UDungeonEdToolBase::OnClicked(const FInputDeviceRay& ClickPos) {
	Super::OnClicked(ClickPos);
}

FInputRayHit UDungeonEdToolBase::IsHitByClick(const FInputDeviceRay& ClickPos) {
	return Super::IsHitByClick(ClickPos);
}

FInputRayHit UDungeonEdToolBase::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) {
	const FInputRayHit Result(0);
	return Result;
}

void UDungeonEdToolBase::OnBeginHover(const FInputDeviceRay& DevicePos) {
}

bool UDungeonEdToolBase::OnUpdateHover(const FInputDeviceRay& DevicePos) {
	return false;
}

void UDungeonEdToolBase::OnEndHover() {
}

bool UDungeonEdToolBase::SupportsWorldSpaceFocusBox() {
	return IInteractiveToolCameraFocusAPI::SupportsWorldSpaceFocusBox();
}

FBox UDungeonEdToolBase::GetWorldSpaceFocusBox() {
	return IInteractiveToolCameraFocusAPI::GetWorldSpaceFocusBox();
}

bool UDungeonEdToolBase::SupportsWorldSpaceFocusPoint() {
	return IInteractiveToolCameraFocusAPI::SupportsWorldSpaceFocusPoint();
}

bool UDungeonEdToolBase::GetWorldSpaceFocusPoint(const FRay& WorldRay, FVector& PointOut) {
	return IInteractiveToolCameraFocusAPI::GetWorldSpaceFocusPoint(WorldRay, PointOut);
}

void UDungeonEdToolBase::SetState(EState NewState) {
	CurrentState = NewState;
}

