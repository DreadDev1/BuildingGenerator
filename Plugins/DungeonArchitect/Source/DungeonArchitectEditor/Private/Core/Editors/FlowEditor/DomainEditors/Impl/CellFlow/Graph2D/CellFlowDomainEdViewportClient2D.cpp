//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Graph2D/CellFlowDomainEdViewportClient2D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/Renderers/FlowViewportCanvasRenderer.h"

#define LOCTEXT_NAMESPACE "CellFlowDomainEdViewportClient2D"

#undef LOCTEXT_NAMESPACE
void FCellFlowDomainEdViewportClient2D::DrawImpl(const FFlowViewportCanvasRenderer& InRenderer) {
	FFlowDomainEdViewportClient2D::DrawImpl(InRenderer);
	
	InRenderer.DrawText("Hello World 3 ggg", 0, 0);
	InRenderer.DrawRect(FVector2D(-100, -200), FVector2D(200, 100), FLinearColor(0, 1, 0, 0.25f), FLinearColor(0, 1, 0, 0.5f));
	InRenderer.DrawLine(FVector2D(-100, -100), FVector2D(200, 200), FLinearColor(0, 1, 1), 2);
	InRenderer.DrawLine(FVector2D(100, -100), FVector2D(-200, 100), FLinearColor(0, 1, 1), 1);
}

