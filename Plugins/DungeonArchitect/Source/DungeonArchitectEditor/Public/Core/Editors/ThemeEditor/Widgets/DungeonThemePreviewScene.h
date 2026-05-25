//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "AdvancedPreviewScene.h"

class FDungeonThemePreviewScene : public FAdvancedPreviewScene {
public:
	FDungeonThemePreviewScene(ConstructionValues CVS, float InFloorOffset = 0.0f);

private:
	void ReleaseDefaultWorld();
};

