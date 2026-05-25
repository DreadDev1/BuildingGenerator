//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/Widgets/SThemePreviewViewport.h"

#include "PreviewScene.h"

class FDungeonArchitectThemeEditor;

/** Viewport Client for the preview viewport */
class DUNGEONARCHITECTEDITOR_API FDungeonEditorViewportClient : public FEditorViewportClient,
                                                                public TSharedFromThis<FDungeonEditorViewportClient> {
public:
    FDungeonEditorViewportClient(TWeakPtr<SThemePreviewViewport> InDungeonEditorViewport,
                                 FPreviewScene& InPreviewScene, UDungeonThemeAsset* InProp);

    // FEditorViewportClient interface
    virtual void Tick(float DeltaSeconds) override;
    // End of FEditorViewportClient interface
	
private:
    TWeakPtr<SThemePreviewViewport> InDungeonEditorViewport;
    UDungeonThemeAsset* InProp;
};

