//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgeTransformTool.h"

#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorMode.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgeTransformToolHandler.h"

#include "BaseGizmos/CombinedTransformGizmo.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "EditorWorldExtension.h"
#include "InteractiveToolManager.h"
#include "Mechanics/DragAlignmentMechanic.h"

///////////////////////////// UDungeonForgeTransformToolBuilder /////////////////////////////
UDungeonForgeTransformToolBuilder::UDungeonForgeTransformToolBuilder() {
	ToolClass = UDungeonForgeTransformTool::StaticClass();
}

void UDungeonForgeTransformToolBuilder::SetNodeSettings(UDungeonForgeNodeSettings* InNodeSettings) {
	NodeSettings = InNodeSettings;
}

UInteractiveTool* UDungeonForgeTransformToolBuilder::BuildTool(const FToolBuilderState& SceneState) const {
	UInteractiveTool* Tool = Super::BuildTool(SceneState);
	if (UDungeonForgeTransformTool* TransformTool = Cast<UDungeonForgeTransformTool>(Tool)) {
		TransformTool->SetTargetNodeSettings(NodeSettings.Get());
	}
	return Tool;
}

///////////////////////////// UDungeonForgeTransformTool /////////////////////////////
void UDungeonForgeTransformTool::Setup() {
	Super::Setup();

	TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->OnTransformChanged.AddUObject(this, &UDungeonForgeTransformTool::OnTransformProxyChanged);
	TransformProxy->OnBeginTransformEdit.AddUObject(this, &UDungeonForgeTransformTool::OnBeginProxyDrag);
	TransformProxy->OnEndTransformEdit.AddUObject(this, &UDungeonForgeTransformTool::OnEndProxyDrag);

	if (!IDungeonForgeTransformToolHandler::Execute_GetTransformToolTransform(TargetNodeSettings.Get(), CurrentTransform))
	{
		UE_LOG(LogDungeonForgeEdMode, Warning, TEXT("Could not get initial transform for Tool from %s. Using Identity."), *TargetNodeSettings->GetDefaultNodeName().ToString());
		CurrentTransform = FTransform::Identity;
	}
	
	CurrentTransform.NormalizeRotation();
	TransformProxy->SetTransform(CurrentTransform);

	UInteractiveGizmoManager* GizmoManager = GetToolManager()->GetPairedGizmoManager();
	ETransformGizmoSubElements Elements = ETransformGizmoSubElements::TranslateAllAxes | ETransformGizmoSubElements::TranslateAllPlanes;
	if (TargetNodeSettings.IsValid()) {
		if (IDungeonForgeTransformToolHandler::Execute_SupportsTransformToolRotation(TargetNodeSettings.Get())) {
			Elements |= ETransformGizmoSubElements::RotateAllAxes;
		}
		
		if (IDungeonForgeTransformToolHandler::Execute_SupportsTransformToolScale(TargetNodeSettings.Get())) {
			Elements |= ETransformGizmoSubElements::ScaleAllAxes | ETransformGizmoSubElements::ScaleAllPlanes | ETransformGizmoSubElements::ScaleUniform;
		}
	}
	
	
	Gizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GetToolManager(), Elements, this);
	Gizmo->SetActiveTarget(TransformProxy, GetToolManager());

	DragAlignmentMechanic = NewObject<UDragAlignmentMechanic>(this);
	DragAlignmentMechanic->Setup(this);
	DragAlignmentMechanic->AddToGizmo(Gizmo);
	
	IDungeonForgeTransformToolHandler::Execute_OnTransformToolActivated(TargetNodeSettings.Get());
}

void UDungeonForgeTransformTool::Shutdown(EToolShutdownType ShutdownType) {
	if (TargetNodeSettings.IsValid()) {
		IDungeonForgeTransformToolHandler::Execute_OnTransformToolDeactivated(TargetNodeSettings.Get());
	}

	if (DragAlignmentMechanic) {
		DragAlignmentMechanic->Shutdown();
		DragAlignmentMechanic = nullptr;
	}
	
	if (Gizmo) {
		UInteractiveGizmoManager* GizmoManager = GetToolManager()->GetPairedGizmoManager();
		GizmoManager->DestroyGizmo(Gizmo);
		Gizmo = nullptr;
	}
	
	if (TransformProxy) {
		TransformProxy->OnTransformChanged.RemoveAll(this);
		TransformProxy->OnBeginTransformEdit.RemoveAll(this);
		TransformProxy->OnEndTransformEdit.RemoveAll(this);
		TransformProxy = nullptr;
	}

	TargetNodeSettings = nullptr;

	Super::Shutdown(ShutdownType);
}

void UDungeonForgeTransformTool::Render(IToolsContextRenderAPI* RenderAPI) {
	Super::Render(RenderAPI);
	
	if (DragAlignmentMechanic) {
		DragAlignmentMechanic->Render(RenderAPI);
	}
}

void UDungeonForgeTransformTool::OnTick(float DeltaTime) {
	Super::OnTick(DeltaTime);

}

void UDungeonForgeTransformTool::SetTargetNodeSettings(UDungeonForgeNodeSettings* InNodeSettings) {
	TargetNodeSettings = InNodeSettings;
}

void UDungeonForgeTransformTool::UpdateGizmoTransform() {
	if (TargetNodeSettings.IsValid() && TransformProxy && Gizmo && !bIsDragging) {
		if (IDungeonForgeTransformToolHandler::Execute_GetTransformToolTransform(TargetNodeSettings.Get(), CurrentTransform)) {
			CurrentTransform.NormalizeRotation();
			if (!TransformProxy->GetTransform().Equals(CurrentTransform, 0.001f)) {
				TransformProxy->SetTransform(CurrentTransform);
			}
		}
	}
}

void UDungeonForgeTransformTool::OnTransformProxyChanged(UTransformProxy* Proxy, FTransform NewTransform) {
	if (TargetNodeSettings.IsValid()) {
        CurrentTransform = NewTransform;
    	IDungeonForgeTransformToolHandler::Execute_SetTransformToolTransform(TargetNodeSettings.Get(), NewTransform, /*bInteractionEnded=*/false);
    }
}

void UDungeonForgeTransformTool::OnBeginProxyDrag(UTransformProxy* Proxy) {
    bIsDragging = true;
}

void UDungeonForgeTransformTool::OnEndProxyDrag(UTransformProxy* InProxy) {
    bIsDragging = false;
	if (TargetNodeSettings.IsValid()) {
		IDungeonForgeTransformToolHandler::Execute_SetTransformToolTransform(TargetNodeSettings.Get(), InProxy->GetTransform(), /*bInteractionEnded=*/true);

		if (UDungeonForgeEditorMode* ForgeEdMode = GetForgeEditorMode()) {
			ForgeEdMode->BuildDungeon();
		}
	}
}

