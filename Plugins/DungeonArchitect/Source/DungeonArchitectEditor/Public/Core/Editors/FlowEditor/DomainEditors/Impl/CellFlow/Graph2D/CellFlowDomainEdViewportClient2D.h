//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewportClient2D.h"

class FCellFlowDomainEdViewportClient2D : public FFlowDomainEdViewportClient2D {
public:
	FCellFlowDomainEdViewportClient2D(FEditorModeTools* InModeTools, FPreviewScene* InPreviewScene = nullptr, const TWeakPtr<SEditorViewport>& InEditorViewportWidget = nullptr)
		: FFlowDomainEdViewportClient2D(InModeTools, InPreviewScene, InEditorViewportWidget) {}

	virtual void DrawImpl(const FFlowViewportCanvasRenderer& InRenderer) override;
};

