//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport3D/FlowDomainEdViewportClient3D.h"

#include "EngineUtils.h"
#include "PreviewScene.h"
#include "SceneView.h"

//////////////////////////// SFlowDomainEdViewportClient ////////////////////////////

FFlowDomainEdViewportClient3D::FFlowDomainEdViewportClient3D(FPreviewScene& InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewport)
    : FEditorViewportClient(nullptr, &InPreviewScene, InEditorViewport)
{
    static const float ViewportGridSize = 2048.0f;
    static const int32 ViewportCellSize = 16;
    
    // Setup defaults for the common draw helper.
    DrawHelper.bDrawPivot = false;
    DrawHelper.bDrawWorldBox = false;
    DrawHelper.bDrawKillZ = false;
    DrawHelper.bDrawGrid = true;
    DrawHelper.GridColorAxis = FColor(160, 160, 160);
    DrawHelper.GridColorMajor = FColor(144, 144, 144);
    DrawHelper.GridColorMinor = FColor(128, 128, 128);
    DrawHelper.PerspectiveGridSize = ViewportGridSize;
    DrawHelper.NumCells = FMath::FloorToInt(DrawHelper.PerspectiveGridSize / (ViewportCellSize * 2.0f));

    FEditorViewportClient::SetViewMode(VMI_Lit);

    EngineShowFlags.SetSnap(false);
    EngineShowFlags.CompositeEditorPrimitives = true;
    OverrideNearClipPlane(1.0f);
    bUsingOrbitCamera = true;


    const FVector CamLocation(200, 200, 200);

    // Set the initial camera position
    const FRotator OrbitRotation(-45, 100, 0);
    SetCameraSetup(
        FVector::ZeroVector,
        OrbitRotation,
        FVector(0.0f, 100, 0.0f),
        FVector::ZeroVector,
        CamLocation,
        FRotator(0, 0, 0)
    );
}

void FFlowDomainEdViewportClient3D::Tick(float DeltaSeconds) {
    FEditorViewportClient::Tick(DeltaSeconds);

    // Tick the preview scene world.
    if (!GIntraFrameDebuggingGameThread) {
        PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
    }
}
void FFlowDomainEdViewportClient3D::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) {
    FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);

    AActor* SelectedActor = nullptr;
    if(HitProxy && HitProxy->IsA(HActor::StaticGetType()))
    {
        HActor* ActorHit = static_cast<HActor*>(HitProxy);
        SelectedActor = ActorHit ? ActorHit->Actor : nullptr;
    }
    ActorSelectionChanged.ExecuteIfBound(SelectedActor);

    if (Event == IE_DoubleClick && SelectedActor) {
        ActorDoubleClicked.ExecuteIfBound(SelectedActor);
    }
}

void FFlowDomainEdViewportClient3D::SetupViewForRendering(FSceneViewFamily& ViewFamily, FSceneView& View) {
    FEditorViewportClient::SetupViewForRendering(ViewFamily, View);

    View.AntiAliasingMethod = AAM_FXAA; 
}

void FFlowDomainEdViewportClient3D::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) {
    FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
    
    // Draw the registered component visualizers
    ComponentsToVisualize.RemoveAll([](const FComponentVisualizeInfo& Entry) { return !Entry.ComponentPtr.IsValid(); });
    for (FComponentVisualizeInfo& VisualizerInfo : ComponentsToVisualize) {
        if (VisualizerInfo.Visualizer.IsValid() && VisualizerInfo.ComponentPtr.IsValid()) {
            VisualizerInfo.Visualizer->DrawVisualizationHUD(VisualizerInfo.ComponentPtr.Get(), &InViewport, &View, &Canvas);
        }
    }
}

void FFlowDomainEdViewportClient3D::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) {
    FEditorViewportClient::Draw(View, PDI);

    // Draw the registered component visualizers
    ComponentsToVisualize.RemoveAll([](const FComponentVisualizeInfo& Entry) { return !Entry.ComponentPtr.IsValid(); });
    for (FComponentVisualizeInfo& VisualizerInfo : ComponentsToVisualize) {
        if (VisualizerInfo.Visualizer.IsValid() && VisualizerInfo.ComponentPtr.IsValid()) {
            VisualizerInfo.Visualizer->DrawVisualization(VisualizerInfo.ComponentPtr.Get(), View, PDI);
        }
    }
}

void FFlowDomainEdViewportClient3D::RegisterComponentForVisualization(const TSharedPtr<FComponentVisualizer>& InComponentVisualizer, TObjectPtr<UActorComponent> InComponent) {
    ComponentsToVisualize.Add({ InComponentVisualizer, InComponent });
}

void FFlowDomainEdViewportClient3D::ClearComponentVisualizers() {
    ComponentsToVisualize.Reset();
}

