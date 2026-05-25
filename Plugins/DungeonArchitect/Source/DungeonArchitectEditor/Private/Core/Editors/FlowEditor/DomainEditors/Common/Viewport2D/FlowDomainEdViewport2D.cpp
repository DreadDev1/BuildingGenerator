//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewport2D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewportClient2D.h"

#include "Widgets/Docking/SDockTab.h"

void SFlowDomainEdViewport2D::Construct(const FArguments& InArgs, TSharedPtr<FFlowDomainEdViewportClient2D> InEditorViewportClient) {
	EditorViewportClient = InEditorViewportClient;
	EditorViewportClient->VisibilityDelegate.BindSP(this, &SFlowDomainEdViewport2D::IsVisible);

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SFlowDomainEdViewport2D::~SFlowDomainEdViewport2D() {
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	if (EditorViewportClient.IsValid()) {
		EditorViewportClient->Viewport = nullptr;
	}
}

TSharedRef<FEditorViewportClient> SFlowDomainEdViewport2D::MakeEditorViewportClient() {
	check(EditorViewportClient.IsValid());
	return EditorViewportClient.ToSharedRef();
}

EVisibility SFlowDomainEdViewport2D::OnGetViewportContentVisibility() const {
	return EVisibility::Visible;
}

bool SFlowDomainEdViewport2D::IsVisible() const {
	return ViewportWidget.IsValid() && (!ParentTab.IsValid() || ParentTab.Pin()->IsForeground());
}

