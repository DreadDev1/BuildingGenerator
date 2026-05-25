//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonEdMode/DungeonEdModeRenderer.h"

class USnapMapEdModeHandler;

class DUNGEONARCHITECTEDITOR_API FSnapMapEdModeRenderer : public FDungeonEdModeRenderer {
public:
    FSnapMapEdModeRenderer(USnapMapEdModeHandler* pModeHandler);
    virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI,
                        TSharedPtr<IDungeonEdTool> ActiveTool) override;
    virtual void Destroy() override;

private:
    USnapMapEdModeHandler* ModeHandler;
};

