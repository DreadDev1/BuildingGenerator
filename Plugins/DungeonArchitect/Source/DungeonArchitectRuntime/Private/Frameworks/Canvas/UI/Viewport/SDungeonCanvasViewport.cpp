//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/UI/Viewport/SDungeonCanvasViewport.h"

#include "Frameworks/Canvas/Cameras/DungeonCanvasCamera.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"
#include "Frameworks/Canvas/UI/Viewport/DungeonCanvasViewportClient.h"

#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "SDungeonCanvasViewport"

const double FDungeonCanvasViewportConstants::MaxZoom = 16.0;
const double FDungeonCanvasViewportConstants::MinZoom = 1.0/64;
const int FDungeonCanvasViewportConstants::ZoomFactorLogSteps = 8;
const double FDungeonCanvasViewportConstants::ZoomFactor = FMath::Pow(2.0,1.0/ZoomFactorLogSteps);

/* SDungeonCanvasEditorViewport interface
 *****************************************************************************/

void SDungeonCanvasViewport::AddReferencedObjects( FReferenceCollector& Collector )
{
	ViewportClient->AddReferencedObjects(Collector);
}

namespace DungeonCanvasEditorViewport {
	static void CaptureThumbnail() {
		
	}
}

void SDungeonCanvasViewport::Construct(const FArguments& InArgs) {
	DrawSettings = InArgs._DrawSettings.Get();
	Camera = InArgs._Camera.Get();
	bIsRenderingEnabled = true;
	ZoomMode = EDungeonCanvasViewportZoomMode::Fit;
	Zoom = 1.0f;

	bool bRenderDirectlyToWindow = false;
	if (UDungeonCanvasComponent* DungeonCanvas = GetDungeonCanvasInstance()) {
		UWorld* World = DungeonCanvas->GetWorld();
		if (World && World->IsGameWorld()) {
			bRenderDirectlyToWindow = true;
		}
	}
		
	
	ViewportWidget = SNew(SViewport)
		.EnableGammaCorrection(true)
		.EnableBlending(true)
		.PreMultipliedAlpha(true)
		.RenderDirectlyToWindow(false)	// TODO: parameterize me
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
		.ShowEffectWhenDisabled(false)
		.EnableBlending(true);
	
	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SOverlay)
			// viewport canvas
			+ SOverlay::Slot()
			.Padding(5.0f, 0.0f)
			[
				ViewportWidget.ToSharedRef()
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			[
				// vertical scroll bar
				SAssignNew(CanvasViewportVerticalScrollBar, SScrollBar)
				.Visibility(this, &SDungeonCanvasViewport::HandleVerticalScrollBarVisibility)
				.OnUserScrolled(this, &SDungeonCanvasViewport::HandleVerticalScrollBarScrolled)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			// horizontal scrollbar
			SAssignNew(CanvasViewportHorizontalScrollBar, SScrollBar)
				.Orientation( Orient_Horizontal )
				.Visibility(this, &SDungeonCanvasViewport::HandleHorizontalScrollBarVisibility)
				.OnUserScrolled(this, &SDungeonCanvasViewport::HandleHorizontalScrollBarScrolled)
		]

	];
	
	ViewportClient = MakeShareable(new FDungeonCanvasViewportClient(SharedThis(this)));
	Viewport = MakeShareable(new FSceneViewport(ViewportClient.Get(), ViewportWidget));

	// The viewport widget needs an interface so it knows what should render
	ViewportWidget->SetViewportInterface( Viewport.ToSharedRef() );
}

void SDungeonCanvasViewport::ModifyCheckerboardTextureColors( )
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->LoadCheckerboardTextureColors();
	}
}

void SDungeonCanvasViewport::EnableRendering()
{
	bIsRenderingEnabled = true;
}

void SDungeonCanvasViewport::DisableRendering()
{
	bIsRenderingEnabled = false;
}

TSharedPtr<FSceneViewport> SDungeonCanvasViewport::GetViewport( ) const
{
	return Viewport;
}

TSharedPtr<SViewport> SDungeonCanvasViewport::GetViewportWidget( ) const
{
	return ViewportWidget;
}

TSharedPtr<SScrollBar> SDungeonCanvasViewport::GetVerticalScrollBar( ) const
{
	return CanvasViewportVerticalScrollBar;
}

TSharedPtr<SScrollBar> SDungeonCanvasViewport::GetHorizontalScrollBar( ) const
{
	return CanvasViewportHorizontalScrollBar;
}


/* SWidget overrides
 *****************************************************************************/

void SDungeonCanvasViewport::SetDungeonCanvasInstance(UDungeonCanvasComponent* InInstance) {
	DungeonCanvasInstancePtr = InInstance;
}

void SDungeonCanvasViewport::SetDungeonCanvasTheme(UDungeonCanvasMaterialTheme* InTheme) {
	DungeonCanvasThemePtr = InTheme;
}

void SDungeonCanvasViewport::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (bIsRenderingEnabled)
	{
		Viewport->Invalidate();
	}
}


void SDungeonCanvasViewport::ZoomIn() {
	const double CurrentZoom = CalculateDisplayedZoomLevel();
	SetCustomZoomLevel(CurrentZoom * FDungeonCanvasViewportConstants::ZoomFactor);
}

void SDungeonCanvasViewport::ZoomOut() {
	const double CurrentZoom = CalculateDisplayedZoomLevel();
	SetCustomZoomLevel(CurrentZoom / FDungeonCanvasViewportConstants::ZoomFactor);
}

EDungeonCanvasViewportZoomMode SDungeonCanvasViewport::GetZoomMode() const {
	return ZoomMode;
}

float SDungeonCanvasViewport::GetCustomZoomLevel() const {
	return Zoom;
}

void SDungeonCanvasViewport::SetCustomZoomLevel(float ZoomValue) {
	// snap to discrete steps so that if we are nearly at 1.0 or 2.0, we hit them exactly:
	//ZoomValue = FMath::GridSnap(ZoomValue, MinZoom/4.0);

	double LogZoom = log2(ZoomValue);
	// the mouse wheel zoom is quantized on ZoomFactorLogSteps
	//	but that's too chunky for the drag slider, give it more steps, but on the same quantization grid
	double QuantizationSteps = FDungeonCanvasViewportConstants::ZoomFactorLogSteps*2.0;
	double LogZoomQuantized = (1.0/QuantizationSteps) * (double)FMath::RoundToInt( QuantizationSteps * LogZoom );
	ZoomValue = pow(2.0,LogZoomQuantized);

	ZoomValue = FMath::Clamp(ZoomValue, FDungeonCanvasViewportConstants::MinZoom, FDungeonCanvasViewportConstants::MaxZoom);
	
	// set member variable "Zoom"
	Zoom = ZoomValue;

	// For now we also want to be in custom mode whenever this is changed
	SetZoomMode(EDungeonCanvasViewportZoomMode::Custom);
}

void SDungeonCanvasViewport::SetZoomMode(const EDungeonCanvasViewportZoomMode InZoomMode) {
	// Update our own zoom mode
	ZoomMode = InZoomMode;
}

void SDungeonCanvasViewport::GetBaseTextureSize(int& OutWidth, int& OutHeight) const {
	FIntPoint Size = GetViewport()->GetSizeXY();
	OutWidth = Size.X;
	OutHeight = Size.Y;
}

double SDungeonCanvasViewport::CalculateDisplayedZoomLevel() const {
	// Avoid calculating dimensions if we're custom anyway
	if (GetZoomMode() == EDungeonCanvasViewportZoomMode::Custom) {
		return Zoom;
	}
	int32 PreviewTexWidth{}, PreviewTexHeight{};
	GetBaseTextureSize(PreviewTexWidth, PreviewTexHeight);
	
	int32 DisplayWidth, DisplayHeight, DisplayDepth, DisplayArraySize;
	CalculateTextureDimensions(DisplayWidth, DisplayHeight, DisplayDepth, DisplayArraySize, false);
	if (PreviewTexHeight != 0) {
		return (double)DisplayHeight / PreviewTexHeight;
	}
	else if (PreviewTexWidth != 0) {
		return (double)DisplayWidth / PreviewTexWidth;
	}
	else {
		return 0;
	}
}

void SDungeonCanvasViewport::CalculateTextureDimensions(int32& OutWidth, int32& OutHeight, int32& OutDepth, int32& OutArraySize, bool bInIncludeBorderSize) const {
	int32 CanvasTexWidth{}, CanvasTexHeight{};
	GetBaseTextureSize(CanvasTexWidth, CanvasTexHeight);
	
	OutDepth = 0;
	OutArraySize = 0;
	const int32 BorderSize = DrawSettings.TextureBorderEnabled ? 1 : 0;

	if (!CanvasTexWidth || !CanvasTexHeight)
	{
		OutWidth = 0;
		OutHeight = 0;
		OutDepth = 0;
		OutArraySize = 0;
		return;
	}

	// Fit is the same as fill, but doesn't scale up past 100%
	const EDungeonCanvasViewportZoomMode CurrentZoomMode = GetZoomMode();
	if (CurrentZoomMode == EDungeonCanvasViewportZoomMode::Fit || CurrentZoomMode == EDungeonCanvasViewportZoomMode::Fill)
	{
		const int32 MaxWidth = FMath::Max(GetViewport()->GetSizeXY().X - 2 * BorderSize, 0);
		const int32 MaxHeight = FMath::Max(GetViewport()->GetSizeXY().Y - 2 * BorderSize, 0);

		if (MaxWidth * CanvasTexHeight < MaxHeight * CanvasTexWidth)
		{
			OutWidth = MaxWidth;
			OutHeight = FMath::DivideAndRoundNearest(OutWidth * CanvasTexHeight, CanvasTexWidth);
		}
		else
		{
			OutHeight = MaxHeight;
			OutWidth = FMath::DivideAndRoundNearest(OutHeight * CanvasTexWidth, CanvasTexHeight);
		}

		// If fit, then we only want to scale down
		// So if our natural dimensions are smaller than the viewport, we can just use those
		if (CurrentZoomMode == EDungeonCanvasViewportZoomMode::Fit && (CanvasTexWidth < OutWidth || CanvasTexHeight < OutHeight))
		{
			OutWidth = CanvasTexWidth;
			OutHeight = CanvasTexHeight;
		}
	}
	else
	{
		OutWidth = static_cast<int32>(CanvasTexWidth * Zoom);
		OutHeight = static_cast<int32>(CanvasTexHeight * Zoom);
	}

	if (bInIncludeBorderSize)
	{
		OutWidth += 2 * BorderSize;
		OutHeight += 2 * BorderSize;
	}
}



/* SDungeonCanvasEditorViewport implementation
 *****************************************************************************/

FText SDungeonCanvasViewport::GetDisplayedResolution( ) const
{
	return ViewportClient->GetDisplayedResolution();
}


/* SDungeonCanvasEditorViewport event handlers
 *****************************************************************************/

void SDungeonCanvasViewport::HandleHorizontalScrollBarScrolled( float InScrollOffsetFraction )
{
	const float Ratio = ViewportClient->GetViewportHorizontalScrollBarRatio();
	const float MaxOffset = (Ratio < 1.0f) ? 1.0f - Ratio : 0.0f;
	InScrollOffsetFraction = FMath::Clamp(InScrollOffsetFraction, 0.0f, MaxOffset);

	CanvasViewportHorizontalScrollBar->SetState(InScrollOffsetFraction, Ratio);
}

EVisibility SDungeonCanvasViewport::HandleHorizontalScrollBarVisibility( ) const
{
	if (ViewportClient->GetViewportHorizontalScrollBarRatio() < 1.0f)
	{
		return EVisibility::Visible;
	}
	
	return EVisibility::Collapsed;
}

void SDungeonCanvasViewport::HandleVerticalScrollBarScrolled( float InScrollOffsetFraction )
{
	const float Ratio = ViewportClient->GetViewportVerticalScrollBarRatio();
	const float MaxOffset = (Ratio < 1.0f) ? 1.0f - Ratio : 0.0f;
	InScrollOffsetFraction = FMath::Clamp(InScrollOffsetFraction, 0.0f, MaxOffset);

	CanvasViewportVerticalScrollBar->SetState(InScrollOffsetFraction, Ratio);
}

EVisibility SDungeonCanvasViewport::HandleVerticalScrollBarVisibility( ) const
{
	if (ViewportClient->GetViewportVerticalScrollBarRatio() < 1.0f)
	{
		return EVisibility::Visible;
	}
	
	return EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE

