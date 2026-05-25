//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Graph2D/CellFlowDomainEdLayoutGraph2D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Graph2D/CellFlowDomainEdViewportClient2D.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraphDomain.h"

#include "AdvancedPreviewScene.h"

#define LOCTEXT_NAMESPACE "CellFlowDomainEdLayoutGraph2D"

FCellFlowDomainEdLayoutGraph2D::FCellFlowDomainEdLayoutGraph2D() {
	
}

IFlowDomainPtr FCellFlowDomainEdLayoutGraph2D::CreateDomain() const {
	return MakeShareable(new FPreviewFlowDomain(TEXT("CellPreview2D"), LOCTEXT("CellPreviewDisplayName", "Cell Preview")));
}

TSharedPtr<FFlowDomainEdViewportClient2D> FCellFlowDomainEdLayoutGraph2D::CreateViewportClient(const FDomainEdInitSettings& InSettings) const {
	TSharedPtr<FFlowDomainEdViewportClient2D> EditorViewportClient = MakeShareable(new FCellFlowDomainEdViewportClient2D(InSettings.EditorModeManager.Get(), PreviewScene.Get()));
	EditorViewportClient->bSetListenerPosition = false;
	EditorViewportClient->SetViewportType(LVT_OrthoXY);
	EditorViewportClient->SetRealtime(true); // TODO: Check if real-time is needed

	return EditorViewportClient;
}

#undef LOCTEXT_NAMESPACE

