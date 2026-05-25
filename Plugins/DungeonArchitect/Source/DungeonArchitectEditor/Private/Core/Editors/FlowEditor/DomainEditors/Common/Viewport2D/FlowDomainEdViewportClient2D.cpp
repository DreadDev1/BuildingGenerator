//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewportClient2D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/Renderers/FlowViewportCanvasRenderer.h"

#include "BaseBehaviors/ClickDragBehavior.h"
#include "BaseBehaviors/MouseWheelBehavior.h"
#include "EditorModeManager.h"
#include "Tools/EdModeInteractiveToolsContext.h"

FFlowDomainEdViewportClient2D::FFlowDomainEdViewportClient2D(FEditorModeTools* InModeTools, FPreviewScene* InPreviewScene,
                                                             const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
	: FEditorViewportClient(InModeTools, InPreviewScene, InEditorViewportWidget)
{
	ShowWidget(false);
	
	// Don't draw the little XYZ drawing in the corner.
	bDrawAxes = false;

	SetViewLocation(FVector(0, 0, 1));
	
	// We want our near clip plane to be quite close so that we can zoom in further.
	OverrideNearClipPlane(KINDA_SMALL_NUMBER);

	// Set up viewport manipulation behaviors:
	

	BehaviorSet = NewObject<UInputBehaviorSet>();

	// We'll have the priority of our viewport manipulation behaviors be lower (i.e. higher
	// numerically) than both the gizmo default and the tool default.
	static constexpr int DEFAULT_VIEWPORT_BEHAVIOR_PRIORITY = 150;

	ScrollBehaviorTarget = MakeUnique<FFlowDomainEd2DScrollBehaviorTargets>(this);
	UClickDragInputBehavior* ScrollBehavior = NewObject<UClickDragInputBehavior>();
	ScrollBehavior->Initialize(ScrollBehaviorTarget.Get());
	ScrollBehavior->SetDefaultPriority(DEFAULT_VIEWPORT_BEHAVIOR_PRIORITY);
	ScrollBehavior->SetUseRightMouseButton();
	BehaviorSet->Add(ScrollBehavior);

	ZoomBehaviorTarget = MakeUnique<FFlowDomainEd2DMouseWheelZoomBehaviorTargets>(this);
	UMouseWheelInputBehavior* ZoomBehavior = NewObject<UMouseWheelInputBehavior>();
	ZoomBehavior->Initialize(ZoomBehaviorTarget.Get());
	ZoomBehavior->SetDefaultPriority(DEFAULT_VIEWPORT_BEHAVIOR_PRIORITY);	
	BehaviorSet->Add(ZoomBehavior);

	ModeTools->GetInteractiveToolsContext()->InputRouter->RegisterSource(this);
}


const UInputBehaviorSet* FFlowDomainEdViewportClient2D::GetInputBehaviors() const
{
	return BehaviorSet;
}

// Collects UObjects that we don't want the garbage collecter to throw away under us
void FFlowDomainEdViewportClient2D::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEditorViewportClient::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(BehaviorSet);
}


bool FFlowDomainEdViewportClient2D::InputKey(const FInputKeyEventArgs& EventArgs) {
	//return FEditorViewportClient::InputKey(InViewport, ControllerId, Key, Event, AmountDepressed, Gamepad);
	
	// We'll support disabling input like our base class, even if it does not end up being used.
	if (bDisableInput)
	{
		return true;
	}

	// Our viewport manipulation is placed in the input router that ModeTools manages
	return ModeTools->InputKey(this, Viewport, EventArgs.Key, EventArgs.Event);
}

namespace FlowDomain2DLib {
	void DrawGrid(const FFlowViewportCanvasRenderer& InRenderer, const FVector& ClipStart, const FVector& ClipEnd, float GridSize, const FLinearColor& Color) {
		const int sx = FMath::FloorToInt(ClipStart.X / GridSize);
		const int ex = FMath::CeilToInt(ClipEnd.X / GridSize);

		int sy = FMath::FloorToInt(ClipStart.Y / GridSize);
		int ey = FMath::CeilToInt(ClipEnd.Y / GridSize);

		for (int x = sx; x <= ex; x++) {
			if (x == 0) continue;
			InRenderer.DrawLine(FVector2D(x, sy) * GridSize, FVector2D(x, ey) * GridSize, Color);
		}

		for (int y = sy; y <= ey; y++) {
			if (y == 0) continue;
			InRenderer.DrawLine(FVector2D(sx, y) * GridSize, FVector2D(ex, y) * GridSize, Color);
		}
	}
	
	void DrawAxis(const FFlowViewportCanvasRenderer& InRenderer, const FVector& ClipStart, const FVector& ClipEnd, float GridSize) {
		const int sx = FMath::FloorToInt(ClipStart.X / GridSize);
		const int ex = FMath::CeilToInt(ClipEnd.X / GridSize);

		int sy = FMath::FloorToInt(ClipStart.Y / GridSize);
		int ey = FMath::CeilToInt(ClipEnd.Y / GridSize);

		const FLinearColor ColorAxisX = FColor(64, 32, 32);
		const FLinearColor ColorAxisY = FColor(32, 64, 32);

		// X-axis
		if (sy <= 0 && ey >= 0) {
			InRenderer.DrawLine(FVector2D(sx, 0) * GridSize, FVector2D(ex, 0) * GridSize, ColorAxisX);
		}

		// Y-axis
		if (sx <= 0 && ex >= 0) {
			InRenderer.DrawLine(FVector2D(0, sy) * GridSize, FVector2D(0, ey) * GridSize, ColorAxisY);
		}
	}
}

// Note that this function gets called from the super class Draw(FViewport*, FCanvas*) overload to draw the scene.
// We don't override that top-level function so that it can do whatever view calculations it needs to do.
void FFlowDomainEdViewportClient2D::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) {
	FEditorViewportClient::Draw(View, PDI);
}

void FFlowDomainEdViewportClient2D::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) {
	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
	
	const FFlowViewportCanvasRenderer Renderer(View, InViewport.GetSizeXY(), Canvas, GetOrthoZoom());
	DrawGrid(Renderer);
	DrawImpl(Renderer);
}

void FFlowDomainEdViewportClient2D::DrawImpl(const FFlowViewportCanvasRenderer& InRenderer) { }
void FFlowDomainEdViewportClient2D::DrawGrid(const FFlowViewportCanvasRenderer& InRenderer) const {
	FVector ClipStart, ClipEnd;
	InRenderer.GetScreenWorldBounds(ClipStart, ClipEnd);
	//InRenderer.Clear(FColor(80, 80, 80));

	constexpr uint8 GridLineColor = 32;
	const FLinearColor ColorGrid = FColor(GridLineColor, GridLineColor, GridLineColor);

	float GridSize = 100;
	const float OrthoZoom = GetOrthoZoom();
	if (OrthoZoom > 600000) {
		GridSize = 10000;
	}
	else if (OrthoZoom > 60000) {
		GridSize = 1000;
	}

	if (OrthoZoom < 6000000) {
		FlowDomain2DLib::DrawGrid(InRenderer, ClipStart, ClipEnd, GridSize, ColorGrid);
	}
	FlowDomain2DLib::DrawAxis(InRenderer, ClipStart, ClipEnd, 100);
}

bool FFlowDomainEdViewportClient2D::ShouldOrbitCamera() const
{
	return false; // The 2D viewport should never orbit.
}

bool FFlowDomainEdViewportClient2D::CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const {
	return false;
}

UE::Widget::EWidgetMode FFlowDomainEdViewportClient2D::GetWidgetMode() const {
	return UE::Widget::EWidgetMode::WM_None;
}

