//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport3D/FlowDomainEdViewport3D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport3D/FlowDomainEdViewportClient3D.h"

#include "PreviewScene.h"
#include "Widgets/Docking/SDockTab.h"

DEFINE_LOG_CATEGORY_STATIC(LogDomainEdViewport, Log, All);
#define LOCTEXT_NAMESPACE "FlowDomainEdViewportBase"

//////////////////////////// SFlowDomainEdViewportBase ////////////////////////////
void SFlowDomainEdViewport3D::Construct(const FArguments& InArgs) {
    PreviewScene = MakeShareable(new FPreviewScene);    
    SEditorViewport::Construct(SEditorViewport::FArguments());

    BindCommands();
}

SFlowDomainEdViewport3D::~SFlowDomainEdViewport3D() {
    if (EditorViewportClient.IsValid()) {
        EditorViewportClient->Viewport = nullptr;
    }
}

void SFlowDomainEdViewport3D::AddReferencedObjects(FReferenceCollector& Collector) {
}

FString SFlowDomainEdViewport3D::GetReferencerName() const {
    static const FString NameString = TEXT("SFlowDomainEdViewport");
    return NameString;
}

UWorld* SFlowDomainEdViewport3D::GetWorld() const {
    return PreviewScene->GetWorld();
}

void SFlowDomainEdViewport3D::RegisterComponentForVisualization(const TSharedPtr<FComponentVisualizer>& InComponentVisualizer, TObjectPtr<UActorComponent> InComponent) const {
    if (EditorViewportClient.IsValid()) {
        EditorViewportClient->RegisterComponentForVisualization(InComponentVisualizer, InComponent);
    }
}

void SFlowDomainEdViewport3D::ClearComponentVisualizers() const {
    if (EditorViewportClient.IsValid()) {
        EditorViewportClient->ClearComponentVisualizers();
    }
}

FFDViewportActorMouseEvent& SFlowDomainEdViewport3D::GetActorSelectionChanged() {
    return EditorViewportClient->GetActorSelectionChanged();
}

FFDViewportActorMouseEvent& SFlowDomainEdViewport3D::GetActorDoubleClicked() {
    return EditorViewportClient->GetActorDoubleClicked();
}

TSharedRef<FEditorViewportClient> SFlowDomainEdViewport3D::MakeEditorViewportClient() {
    EditorViewportClient = MakeShareable( new FFlowDomainEdViewportClient3D(*PreviewScene, SharedThis(this)));
    EditorViewportClient->SetRealtime(true);
    EditorViewportClient->VisibilityDelegate.BindSP(this, &SFlowDomainEdViewport3D::IsVisible);
    return EditorViewportClient.ToSharedRef();
}

bool SFlowDomainEdViewport3D::IsVisible() const {
    return ViewportWidget.IsValid() && (!ParentTab.IsValid() || ParentTab.Pin()->IsForeground());
}


#undef LOCTEXT_NAMESPACE

