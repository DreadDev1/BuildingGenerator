//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FMinimalViewInfo;
class FSceneView;
class UDungeonThemeEdMode;
struct FViewCameraState;

class FDungeonThemeEdModeUtils {
public:
	static void GatherViewportInfo(const FSceneView* InSceneView, const FViewCameraState& InCameraState,
			FMinimalViewInfo& OutViewInfo, FIntPoint& OutViewSize);

	static UDungeonThemeEdMode* GetThemeEdMode();
};

