//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeUtils.h"

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"

#include "Camera/CameraTypes.h"
#include "EditorModeManager.h"
#include "SceneView.h"
#include "ToolContextInterfaces.h"

void FDungeonThemeEdModeUtils::GatherViewportInfo(const FSceneView* InSceneView,
                                                  const FViewCameraState& InCameraState, FMinimalViewInfo& OutViewInfo, FIntPoint& OutViewSize)
{
	if (InSceneView) {
		FMinimalViewInfo ViewInfo;
		ViewInfo.Location = InCameraState.Position;
		ViewInfo.Rotation = InCameraState.Orientation.Rotator();
		ViewInfo.FOV = InCameraState.HorizontalFOVDegrees;
		ViewInfo.AspectRatio = InCameraState.AspectRatio;
		
		if (InSceneView->ViewMatrices.IsPerspectiveProjection()) {
			ViewInfo.ProjectionMode = ECameraProjectionMode::Perspective;
		}
		else {
			ViewInfo.ProjectionMode = ECameraProjectionMode::Orthographic;
			ViewInfo.OrthoWidth = InSceneView->UnscaledViewRect.Width();
		}

		constexpr float ViewRectScaleMultiplier = 0.5f;
		FIntPoint ViewSize = FIntPoint(
			InSceneView->UnscaledViewRect.Width(),
			InSceneView->UnscaledViewRect.Height()) / 2;

		OutViewSize = ViewSize;
		OutViewInfo = ViewInfo;
	}
	else {
		OutViewSize = {};
		OutViewInfo = {};
	}
}


UDungeonThemeEdMode* FDungeonThemeEdModeUtils::GetThemeEdMode() {
	if (GEditor && GEditor->GetEditorWorldContext().World()) {
		return Cast<UDungeonThemeEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDungeonThemeEdMode::EM_DungeonTheme));
	}
	return nullptr;
}

