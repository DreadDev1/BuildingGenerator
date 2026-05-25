//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/UI/Viewport/DungeonCanvasViewportClient.h"

#include "Frameworks/Canvas/Cameras/DungeonCanvasCamera.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"
#include "Frameworks/Canvas/UI/Viewport/SDungeonCanvasViewport.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Slate/SceneViewport.h"
#include "TextureResource.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SScrollBar.h"

/* FDungeonCanvasEditorViewportClient structs
 *****************************************************************************/

FDungeonCanvasViewportClient::FDungeonCanvasViewportClient(TWeakPtr<SDungeonCanvasViewport> InCanvasViewport)
	: CanvasViewportPtr(InCanvasViewport)
	, CheckerboardTexture(nullptr)
{
	check(CanvasViewportPtr.IsValid());

	LoadCheckerboardTextureColors();

	CanvasLayout = NewObject<UCanvas>(GetTransientPackage(), TEXT("DungeonCanvasViewportObject"));
	CanvasRenderResources = NewObject<UDungeonCanvasThemeRenderResources>(GetTransientPackage(), TEXT("CanvasRenderResources"));
}


FDungeonCanvasViewportClient::~FDungeonCanvasViewportClient( )
{
	DestroyCheckerboardTexture();
}


/* FViewportClient interface
 *****************************************************************************/
UDungeonCanvasMaterialTheme* FDungeonCanvasViewportClient::GetDungeonCanvasMaterialTheme() const {
	TSharedPtr<SDungeonCanvasViewport> DungeonCanvasViewport = CanvasViewportPtr.Pin();
	if (DungeonCanvasViewport.IsValid()) {
		return DungeonCanvasViewport->GetDungeonCanvasMaterialTheme();
	}
	return nullptr;
}

UDungeonCanvasComponent* FDungeonCanvasViewportClient::GetDungeonCanvasInstance() const {
	TSharedPtr<SDungeonCanvasViewport> DungeonCanvasViewport = CanvasViewportPtr.Pin();
	if (DungeonCanvasViewport.IsValid()) {
		return DungeonCanvasViewport->GetDungeonCanvasInstance();
	}
	return nullptr; 
}

FDungeonCanvasDrawSettings FDungeonCanvasViewportClient::GetDrawSettings() const {
	TSharedPtr<SDungeonCanvasViewport> DungeonCanvasViewport = CanvasViewportPtr.Pin();
	if (DungeonCanvasViewport.IsValid()) {
		return DungeonCanvasViewport->GetDrawSettings();
	}
	return {};
}

void FDungeonCanvasViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas) {
	
	FDungeonCanvasDrawSettings DrawSettings = GetDrawSettings();
	
	FVector2D Ratio = FVector2D(GetViewportHorizontalScrollBarRatio(), GetViewportVerticalScrollBarRatio());
	FVector2D ViewportSize = FVector2D(CanvasViewportPtr.Pin()->GetViewport()->GetSizeXY());
	FVector2D ScrollBarPos = GetViewportScrollBarPositions();
	int32 BorderSize = DrawSettings.TextureBorderEnabled ? 1 : 0;
	float YOffset = static_cast<float>((Ratio.Y > 1.0) ? ((ViewportSize.Y - (ViewportSize.Y / Ratio.Y)) * 0.5) : 0);
	int32 YPos = (int32)FMath::Clamp(FMath::RoundToInt(YOffset - ScrollBarPos.Y + BorderSize), TNumericLimits<int32>::Min(), TNumericLimits<int32>::Max());
	float XOffset = static_cast<float>((Ratio.X > 1.0) ? ((ViewportSize.X - (ViewportSize.X / Ratio.X)) * 0.5) : 0);
	int32 XPos = (int32)FMath::Clamp(FMath::RoundToInt(XOffset - ScrollBarPos.X + BorderSize), TNumericLimits<int32>::Min(), TNumericLimits<int32>::Max());
	
	UpdateScrollBars();

	Canvas->Clear( DrawSettings.BackgroundColor );
	
	// Figure out the size we need
	int32 Width, Height, Depth, ArraySize;
	CanvasViewportPtr.Pin()->CalculateTextureDimensions(Width, Height, Depth, ArraySize, false);

	// Draw the background checkerboard pattern in the same size/position as the render texture so it will show up anywhere
	// the texture has transparency
	if (Viewport && CheckerboardTexture)
	{
		const int32 CheckerboardSizeX = FMath::Max<int32>(1, CheckerboardTexture->GetSizeX());
		const int32 CheckerboardSizeY = FMath::Max<int32>(1, CheckerboardTexture->GetSizeY());
		Canvas->DrawTile( XPos, YPos, Width, Height, 0.0f, 0.0f, (float)Width / CheckerboardSizeX, (float)Height / CheckerboardSizeY, FLinearColor::White, CheckerboardTexture->GetResource());
	}
	
	if (CanvasLayout) {
		CanvasLayout->Init(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, nullptr, Canvas);
		if (UDungeonCanvasComponent* CanvasInstance = GetDungeonCanvasInstance(); IsValid(CanvasInstance)) {
			if (UDungeonCanvasMaterialTheme* CanvasTheme = GetDungeonCanvasMaterialTheme()) {
				FDungeonCanvasViewportTransform ViewTransform = CanvasInstance->GetFullDungeonViewTransform();
				const FVector CanvasSize = FVector(Width, Height, 0);
				const FVector CanvasBaseLocation = FVector(XPos, YPos, 0) + CanvasSize * 0.5f;
				const FTransform LocalToCanvas(FRotator::ZeroRotator, CanvasBaseLocation, CanvasSize);
				ViewTransform.SetLocalToCanvas(LocalToCanvas);

				if (!CanvasRenderResources->ThemeMaterialInstance || CanvasRenderResources->ThemeMaterialInstance->Parent != CanvasTheme->CompiledThemeMaterial) {
					CanvasRenderResources->ThemeMaterialInstance = UMaterialInstanceDynamic::Create(CanvasTheme->CompiledThemeMaterial, CanvasRenderResources);
					if (UMaterialInterface* FogOfWarMaterialTemplate = CanvasTheme->FogOfWarMaterialTemplate.LoadSynchronous()) {
						CanvasRenderResources->FogOfWarMaterialInstance = UMaterialInstanceDynamic::Create(FogOfWarMaterialTemplate, CanvasRenderResources);
					}
				}

				TSharedPtr<SDungeonCanvasViewport> CanvasViewport = CanvasViewportPtr.Pin();
				if (CanvasViewport.IsValid()) {
					// Draw the canvas
					FDungeonCanvasDrawContext DrawContext;
					DrawContext.CanvasLayout = CanvasLayout;
					DrawContext.ThemeRenderResources = CanvasRenderResources;
					DrawContext.ViewTransform = ViewTransform;
					DrawContext.DrawSettings = CanvasViewport->GetDrawSettings();
					DrawContext.DrawSettings.bFogOfWarEnabled = false;
					DrawContext.Theme = CanvasTheme;
			
					CanvasInstance->Draw(DrawContext);
				}
			}
		}
	}

	// Draw a white border around the texture to show its extents
	if (DrawSettings.TextureBorderEnabled)
	{
		float ScaledBorderSize = ((float)BorderSize - 1) * 0.5f;
		FCanvasBoxItem BoxItem(FVector2D((float)XPos - ScaledBorderSize, (float)YPos - ScaledBorderSize), FVector2D(Width + BorderSize, Height + BorderSize));
		BoxItem.LineThickness = (float)BorderSize;
		BoxItem.SetColor( DrawSettings.TextureBorderColor );
		Canvas->DrawItem( BoxItem );
	}
}

UWorld* FDungeonCanvasViewportClient::GetWorld() const {
	if (const UDungeonCanvasComponent* DungeonCanvasInstance = GetDungeonCanvasInstance()) {
		return DungeonCanvasInstance->GetWorld();
	}
	return nullptr;
}

void FDungeonCanvasViewportClient::LoadCheckerboardTextureColors()
{
	DestroyCheckerboardTexture();

	TSharedPtr<SDungeonCanvasViewport> DungeonCanvasViewport = CanvasViewportPtr.Pin();
	FDungeonCanvasDrawSettings DungeonCanvasDrawSettings = DungeonCanvasViewport.IsValid()
			? DungeonCanvasViewport->GetDrawSettings()
			: FDungeonCanvasDrawSettings{};
	
	if (DungeonCanvasDrawSettings.bDrawTransparentBackgroundCheckerboardTexture) {
		constexpr FColor CheckerColorOne = FColor(128, 128, 128);
		constexpr FColor CheckerColorTwo = FColor(64, 64, 64);
		constexpr int32 CheckerSize = 16;
		CheckerboardTexture = FImageUtils::CreateCheckerboardTexture(CheckerColorOne, CheckerColorTwo, CheckerSize);
	}
}

bool FDungeonCanvasViewportClient::InputKey(const FInputKeyEventArgs& InEventArgs)
{
	if (InEventArgs.Event == IE_Pressed) {
		if (InEventArgs.Key == EKeys::MouseScrollUp) {
			CanvasViewportPtr.Pin()->ZoomIn();
			return true;
		}
		else if (InEventArgs.Key == EKeys::MouseScrollDown) {
			CanvasViewportPtr.Pin()->ZoomOut();
			return true;
		}
	}
	return false;
}

bool FDungeonCanvasViewportClient::InputAxis(const FInputKeyEventArgs& Args) {
	if (Args.Key == EKeys::MouseX || Args.Key == EKeys::MouseY)
	{
		if (ShouldUseMousePanning(Args.Viewport))
		{
			TSharedPtr<SDungeonCanvasViewport> EditorViewport = CanvasViewportPtr.Pin();

			int32 Width, Height, Depth, ArraySize;
			CanvasViewportPtr.Pin()->CalculateTextureDimensions(Width, Height, Depth, ArraySize, true);

			if (Args.Key == EKeys::MouseY)
			{
				float VDistFromBottom = EditorViewport->GetVerticalScrollBar()->DistanceFromBottom();
				float VRatio = GetViewportVerticalScrollBarRatio();
				float localDelta = (Args.AmountDepressed / static_cast<float>(Height));
				EditorViewport->GetVerticalScrollBar()->SetState(FMath::Clamp((1.f - VDistFromBottom - VRatio) + localDelta, 0.0f, 1.0f - VRatio), VRatio);
			}
			else
			{
				float HDistFromBottom = EditorViewport->GetHorizontalScrollBar()->DistanceFromBottom();
				float HRatio = GetViewportHorizontalScrollBarRatio();
				float localDelta = (Args.AmountDepressed / static_cast<float>(Width)) * -1.f; // delta needs to be inversed
				EditorViewport->GetHorizontalScrollBar()->SetState(FMath::Clamp((1.f - HDistFromBottom - HRatio) + localDelta, 0.0f, 1.0f - HRatio), HRatio);
			}
		}
		return true;
	}

	return false;
}

bool FDungeonCanvasViewportClient::ShouldUseMousePanning(FViewport* Viewport) const
{
	if (Viewport->KeyState(EKeys::RightMouseButton))
	{
		TSharedPtr<SDungeonCanvasViewport> EditorViewport = CanvasViewportPtr.Pin();
		return EditorViewport.IsValid() && EditorViewport->GetVerticalScrollBar().IsValid() && EditorViewport->GetHorizontalScrollBar().IsValid();
	}

	return false;
}

EMouseCursor::Type FDungeonCanvasViewportClient::GetCursor(FViewport* Viewport, int32 X, int32 Y)
{
	return ShouldUseMousePanning(Viewport) ? EMouseCursor::GrabHandClosed : EMouseCursor::Default;
}

bool FDungeonCanvasViewportClient::InputGesture(FViewport* Viewport, const FInputDeviceId DeviceId, EGestureEvent GestureType, const FVector2D& GestureDelta, bool bIsDirectionInvertedFromDevice, const uint64 Timestamp) {
	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);

	if (GestureType == EGestureEvent::Scroll && !LeftMouseButtonDown && !RightMouseButtonDown)
	{
		double CurrentZoom = CanvasViewportPtr.Pin()->GetCustomZoomLevel();
		CanvasViewportPtr.Pin()->SetCustomZoomLevel(CurrentZoom + GestureDelta.Y * 0.01);
		return true;
	}

	return false;
}

void FDungeonCanvasViewportClient::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(CheckerboardTexture);
	Collector.AddReferencedObject(CanvasLayout);
	Collector.AddReferencedObject(CanvasRenderResources);
}

FText FDungeonCanvasViewportClient::GetDisplayedResolution() const
{
	// Zero is the default size 
	int32 Height, Width, Depth, ArraySize;
	CanvasViewportPtr.Pin()->CalculateTextureDimensions(Width, Height, Depth, ArraySize, false);

	FNumberFormattingOptions Options;
	Options.UseGrouping = false;

	if (Depth > 0)
	{
		return FText::Format(NSLOCTEXT("TextureEditor", "DisplayedResolutionThreeDimension", "Displayed: {0}x{1}x{2}"), FText::AsNumber(Width, &Options), FText::AsNumber(Height, &Options), FText::AsNumber(Depth, &Options));
	}
	else
	{
		return FText::Format(NSLOCTEXT("TextureEditor", "DisplayedResolutionTwoDimension", "Displayed: {0}x{1}"), FText::AsNumber(Width, &Options), FText::AsNumber(Height, &Options));
	}
}


float FDungeonCanvasViewportClient::GetViewportVerticalScrollBarRatio() const
{
	int32 Height = 1;
	int32 Width = 1;
	float WidgetHeight = 1.0f;
	if (CanvasViewportPtr.Pin()->GetVerticalScrollBar().IsValid())
	{
		int32 Depth, ArraySize;
		CanvasViewportPtr.Pin()->CalculateTextureDimensions(Width, Height, Depth, ArraySize, true);

		WidgetHeight = (float)(CanvasViewportPtr.Pin()->GetViewport()->GetSizeXY().Y);
	}

	return WidgetHeight / (float)Height;
}


float FDungeonCanvasViewportClient::GetViewportHorizontalScrollBarRatio() const
{
	int32 Width = 1;
	int32 Height = 1;
	float WidgetWidth = 1.0f;
	if (CanvasViewportPtr.Pin()->GetHorizontalScrollBar().IsValid())
	{
		int32 Depth, ArraySize;
		CanvasViewportPtr.Pin()->CalculateTextureDimensions(Width, Height, Depth, ArraySize, true);

		WidgetWidth = (float)(CanvasViewportPtr.Pin()->GetViewport()->GetSizeXY().X);
	}

	return WidgetWidth / (float)Width;
}


void FDungeonCanvasViewportClient::UpdateScrollBars()
{
	TSharedPtr<SDungeonCanvasViewport> Viewport = CanvasViewportPtr.Pin();

	if (!Viewport.IsValid() || !Viewport->GetVerticalScrollBar().IsValid() || !Viewport->GetHorizontalScrollBar().IsValid())
	{
		return;
	}

	float VRatio = GetViewportVerticalScrollBarRatio();
	float HRatio = GetViewportHorizontalScrollBarRatio();
	float VDistFromTop = Viewport->GetVerticalScrollBar()->DistanceFromTop();
	float VDistFromBottom = Viewport->GetVerticalScrollBar()->DistanceFromBottom();
	float HDistFromTop = Viewport->GetHorizontalScrollBar()->DistanceFromTop();
	float HDistFromBottom = Viewport->GetHorizontalScrollBar()->DistanceFromBottom();

	if (VRatio < 1.0f)
	{
		if (VDistFromBottom < 1.0f)
		{
			Viewport->GetVerticalScrollBar()->SetState(FMath::Clamp((1.0f + VDistFromTop - VDistFromBottom - VRatio) * 0.5f, 0.0f, 1.0f - VRatio), VRatio);
		}
		else
		{
			Viewport->GetVerticalScrollBar()->SetState(0.0f, VRatio);
		}
	}

	if (HRatio < 1.0f)
	{
		if (HDistFromBottom < 1.0f)
		{
			Viewport->GetHorizontalScrollBar()->SetState(FMath::Clamp((1.0f + HDistFromTop - HDistFromBottom - HRatio) * 0.5f, 0.0f, 1.0f - HRatio), HRatio);
		}
		else
		{
			Viewport->GetHorizontalScrollBar()->SetState(0.0f, HRatio);
		}
	}
}


FVector2D FDungeonCanvasViewportClient::GetViewportScrollBarPositions() const
{
	FVector2D Positions = FVector2D::ZeroVector;
	if (CanvasViewportPtr.Pin()->GetVerticalScrollBar().IsValid() && CanvasViewportPtr.Pin()->GetHorizontalScrollBar().IsValid())
	{
		int32 Width, Height, Depth, ArraySize;
		float VRatio = GetViewportVerticalScrollBarRatio();
		float HRatio = GetViewportHorizontalScrollBarRatio();
		float VDistFromTop = CanvasViewportPtr.Pin()->GetVerticalScrollBar()->DistanceFromTop();
		float VDistFromBottom = CanvasViewportPtr.Pin()->GetVerticalScrollBar()->DistanceFromBottom();
		float HDistFromTop = CanvasViewportPtr.Pin()->GetHorizontalScrollBar()->DistanceFromTop();
		float HDistFromBottom = CanvasViewportPtr.Pin()->GetHorizontalScrollBar()->DistanceFromBottom();

		CanvasViewportPtr.Pin()->CalculateTextureDimensions(Width, Height, Depth, ArraySize, true);

		if ((CanvasViewportPtr.Pin()->GetVerticalScrollBar()->GetVisibility() == EVisibility::Visible) && VDistFromBottom < 1.0f)
		{
			Positions.Y = FMath::Clamp((1.0f + VDistFromTop - VDistFromBottom - VRatio) * 0.5f, 0.0f, 1.0f - VRatio) * (float)Height;
		}
		else
		{
			Positions.Y = 0.0f;
		}

		if ((CanvasViewportPtr.Pin()->GetHorizontalScrollBar()->GetVisibility() == EVisibility::Visible) && HDistFromBottom < 1.0f)
		{
			Positions.X = FMath::Clamp((1.0f + HDistFromTop - HDistFromBottom - HRatio) * 0.5f, 0.0f, 1.0f - HRatio) * (float)Width;
		}
		else
		{
			Positions.X = 0.0f;
		}
	}

	return Positions;
}

void FDungeonCanvasViewportClient::DestroyCheckerboardTexture()
{
	if (CheckerboardTexture)
	{
		if (CheckerboardTexture->GetResource())
		{
			CheckerboardTexture->ReleaseResource();
		}
		CheckerboardTexture->MarkAsGarbage();
		CheckerboardTexture = NULL;
	}
}

