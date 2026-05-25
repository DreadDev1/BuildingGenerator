//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/Renderers/FlowViewportCanvasRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "GlobalRenderResources.h"

FFlowViewportCanvasRenderer::FFlowViewportCanvasRenderer(FSceneView& InView, const FIntPoint& InViewSize, FCanvas& InCanvas, float InZoom): View(InView), Canvas(InCanvas) {
	HalfViewSize = FVector2D(InViewSize.X, InViewSize.Y) * 0.5f;
		
	constexpr float DefaultZoom = 10000;
	const float CurrentZoom = InZoom;
	TextScale = DefaultZoom / CurrentZoom;
	TextFont = GEngine->GetSmallFont();

	FTransform LocalTransform = FTransform(FVector(0, 0, -100000));
	Canvas.PushRelativeTransform(LocalTransform.ToMatrixWithScale());
}

void FFlowViewportCanvasRenderer::DrawText(const FString& InText, float X, float Y, float InScale) const {
	int TextHeight, TextWidth;
	TextFont->GetStringHeightAndWidth(InText, TextHeight, TextWidth);
	const float OffsetY = TextHeight * InScale;
	FCanvasTextItem TextItem(L(0, 0 + OffsetY), FText::FromString(InText), TextFont, FLinearColor::White);
	TextItem.EnableShadow(FLinearColor::Black);
	TextItem.Scale = FVector2D(TextScale * InScale);
	Canvas.DrawItem(TextItem);
}

void FFlowViewportCanvasRenderer::DrawRect(const FVector2D& InLocation, const FVector2D& InSize, const FLinearColor& InFillColor,
	const FLinearColor& InBorderColor) const {
	const FVector2D Start = L(InLocation.X, InLocation.Y);
	const FVector2D End = L(InLocation.X + InSize.X, InLocation.Y + InSize.Y);

	const FVector2D V00 = Start;
	const FVector2D V10 = FVector2D(End.X, Start.Y);
	const FVector2D V11 = End;
	const FVector2D V01 = FVector2D(Start.X, End.Y);

	FCanvasUVTri Tri0 = MakeTri(V00, V10, V11, InFillColor);
	FCanvasUVTri Tri1 = MakeTri(V00, V11, V01, InFillColor);

	FCanvasTriangleItem TriItem({ Tri0, Tri1 }, GWhiteTexture);
	TriItem.BlendMode = SE_BLEND_Translucent;
	Canvas.DrawItem(TriItem);
		
	// Draw the border
	constexpr float Thickness = 1.0f;
	FCanvasBoxItem BoxItem(Start, End - Start);
	BoxItem.LineThickness = Thickness;
	BoxItem.BlendMode = SE_BLEND_Translucent;
	BoxItem.SetColor(InBorderColor);
	Canvas.DrawItem(BoxItem);
}

void FFlowViewportCanvasRenderer::DrawLine(const FVector2D& InStart, const FVector2D& InEnd, const FLinearColor& InColor, float InThickness) const {
	const FVector2D Start = L(InStart.X, InStart.Y);
	const FVector2D End = L(InEnd.X, InEnd.Y);
		
	FCanvasLineItem Line(Start, End);
	Line.LineThickness = InThickness;
	Line.SetColor(InColor);
	Canvas.DrawItem(Line);
}

void FFlowViewportCanvasRenderer::Clear(const FLinearColor& InClearColor) const {
	Canvas.Clear(InClearColor);
}

void FFlowViewportCanvasRenderer::GetScreenWorldBounds(FVector& OutClipStart, FVector& OutClipEnd) const {
	static const FVector4 MinScreen(-1, -1, 0, 1);
	static const FVector4 MaxScreen(1, 1, 0, 1);
	const FVector4 ClipStart4 = View.ScreenToWorld(MinScreen);
	const FVector4 ClipEnd4 = View.ScreenToWorld(MaxScreen);
	
	OutClipStart = ClipStart4 / ClipStart4.W;
	OutClipEnd = ClipEnd4 / ClipEnd4.W;

	OutClipStart.Y = -OutClipStart.Y;
	OutClipEnd.Y = -OutClipEnd.Y;
}

FCanvasUVTri FFlowViewportCanvasRenderer::MakeTri(const FVector2D& ScreenA, const FVector2D& ScreenB, const FVector2D& ScreenC, const FLinearColor& InColor) {
	FCanvasUVTri Tri;
	Tri.V0_Pos = ScreenA;
	Tri.V1_Pos = ScreenB;
	Tri.V2_Pos = ScreenC;

	Tri.V0_UV = Tri.V1_UV = Tri.V2_UV = FVector2D::ZeroVector;
	Tri.V0_Color = Tri.V1_Color = Tri.V2_Color = InColor;
	return Tri;
}

