//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvas.h"

#include "SceneView.h"

class UFont;

class FFlowViewportCanvasRenderer {
public:
	FFlowViewportCanvasRenderer(FSceneView& InView, const FIntPoint& InViewSize, FCanvas& InCanvas, float InZoom);
	void DrawText(const FString& InText, float X, float Y, float InScale = 1.0f) const;;
	void DrawRect(const FVector2D& InLocation, const FVector2D& InSize, const FLinearColor& InFillColor, const FLinearColor& InBorderColor) const;;
	void DrawLine(const FVector2D& InStart, const FVector2D& InEnd, const FLinearColor& InColor, float InThickness = 0.0f) const;
	void Clear(const FLinearColor& InClearColor) const;
	void GetScreenWorldBounds(FVector& OutClipStart, FVector& OutClipEnd) const;
	
private:
	FORCEINLINE FVector2D L(float X, float Y) const {
		FVector4 BaseLocation = View.WorldToScreen(FVector(X, -Y, 0));
		BaseLocation /= BaseLocation.W;
		return FVector2D(BaseLocation.X, -BaseLocation.Y) * HalfViewSize + HalfViewSize;
	};

	static FCanvasUVTri MakeTri(const FVector2D& ScreenA, const FVector2D& ScreenB, const FVector2D& ScreenC, const FLinearColor& InColor);;

private:
	FSceneView& View;
	FCanvas& Canvas;
	FVector2D HalfViewSize;
	float TextScale;
	UFont* TextFont; 
};

