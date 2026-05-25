//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvas.h"

#include "UObject/GCObject.h"
#include "ViewportClient.h"

class UDungeonCanvasMaterialTheme;
class UDungeonCanvasThemeRenderResources;
class SDungeonCanvasViewport;
class FDungeonCanvasMaterialThemeEditor;

class DUNGEONARCHITECTRUNTIME_API FDungeonCanvasViewportClient 
	: public FViewportClient
	, public FGCObject
{
public:
	/** Constructor */
	FDungeonCanvasViewportClient(TWeakPtr<SDungeonCanvasViewport> InCanvasViewport);
	~FDungeonCanvasViewportClient();

	/** FViewportClient interface */
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	virtual bool InputKey(const FInputKeyEventArgs& InEventArgs) override;
	virtual bool InputAxis(const FInputKeyEventArgs& Args) override;
	virtual bool InputGesture(FViewport* Viewport, const FInputDeviceId DeviceId, EGestureEvent GestureType, const FVector2D& GestureDelta, bool bIsDirectionInvertedFromDevice, const uint64 Timestamp) override;
	virtual UWorld* GetWorld() const override;
	virtual EMouseCursor::Type GetCursor(FViewport* Viewport, int32 X, int32 Y) override;

	virtual UDungeonCanvasMaterialTheme* GetDungeonCanvasMaterialTheme() const;
	virtual UDungeonCanvasComponent* GetDungeonCanvasInstance() const;
	FDungeonCanvasDrawSettings GetDrawSettings() const;

	
	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FDungeonCanvasViewportClient");
	}

	/** Returns a string representation of the currently displayed textures resolution */
	FText GetDisplayedResolution() const;

	/** Returns the ratio of the size of the canvas texture to the size of the viewport */
	float GetViewportVerticalScrollBarRatio() const;
	float GetViewportHorizontalScrollBarRatio() const;
	void LoadCheckerboardTextureColors();
	
private:
	/** Updates the states of the scrollbars */
	void UpdateScrollBars();

	/** Returns the positions of the scrollbars relative to the canvas textures */
	FVector2D GetViewportScrollBarPositions() const;

	/** Destroy the checkerboard texture if one exists */
	void DestroyCheckerboardTexture();

	/** TRUE if right clicking and dragging for panning a texture 2D */
	bool ShouldUseMousePanning(FViewport* Viewport) const;

private:
	/** Pointer back to the canvas viewport control that owns us */
	TWeakPtr<SDungeonCanvasViewport> CanvasViewportPtr;

	/** Checkerboard texture */
	TObjectPtr<UTexture2D> CheckerboardTexture;

	TObjectPtr<UCanvas> CanvasLayout;
	TObjectPtr<UDungeonCanvasThemeRenderResources> CanvasRenderResources;
};

