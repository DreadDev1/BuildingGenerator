//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvas.h"

#include "Widgets/SCompoundWidget.h"
#include "SDungeonCanvasViewport.generated.h"

class UDungeonCanvasMaterialTheme;
class FSceneViewport;
class SScrollBar;
class SViewport;
class UDungeonCanvasCamera;
class FDungeonCanvasViewportClient;

struct FDungeonCanvasViewportConstants {
	// MaxZoom and MinZoom should both be powers of two
	static const double MaxZoom;
	static const double MinZoom;
	
	// ZoomFactor is multiplicative such that an integer number of steps will give a power of two zoom (50% or 200%)
	static const int ZoomFactorLogSteps;
	static const double ZoomFactor;
};


UENUM()
enum class EDungeonCanvasViewportZoomMode : uint8
{
	Custom    UMETA(DisplayName = "Specific Zoom Level"), // First so that any new modes added don't change serialized value
	Fit       UMETA(DisplayName = "Scale Down to Fit"),
	Fill      UMETA(DisplayName = "Scale to Fill"),
};

/**
 * Implements the dungeon canvas editor's view port.
 */
class DUNGEONARCHITECTRUNTIME_API SDungeonCanvasViewport
	: public SCompoundWidget
{
public:	
	SLATE_BEGIN_ARGS(SDungeonCanvasViewport) { }
		SLATE_ATTRIBUTE(FDungeonCanvasDrawSettings, DrawSettings)
		SLATE_ATTRIBUTE(UDungeonCanvasCamera*, Camera)
	SLATE_END_ARGS()

	/**
	 */
	void AddReferencedObjects( FReferenceCollector& Collector );

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 */
	void Construct(const FArguments& InArgs);
	
	/**
	 * Modifies the checkerboard texture's data.
	 */
	void ModifyCheckerboardTextureColors( );


	/** Enable viewport rendering */
	void EnableRendering();

	/** Disable viewport rendering */
	void DisableRendering();

	
	TSharedPtr<SViewport> GetViewportWidget( ) const;
	TSharedPtr<SScrollBar> GetVerticalScrollBar( ) const;
	TSharedPtr<SScrollBar> GetHorizontalScrollBar( ) const;
	TSharedPtr<FSceneViewport> GetViewport( ) const;
	TSharedPtr<FDungeonCanvasViewportClient> GetViewportClient() const { return ViewportClient; }
	FDungeonCanvasDrawSettings GetDrawSettings() const { return DrawSettings; }
	UDungeonCanvasCamera* GetCamera( ) const { return Camera.Get(); }
	void SetDungeonCanvasInstance(UDungeonCanvasComponent* InInstance);
	void SetDungeonCanvasTheme(UDungeonCanvasMaterialTheme* InTheme);

	virtual UDungeonCanvasMaterialTheme* GetDungeonCanvasMaterialTheme() const { return DungeonCanvasThemePtr.Get(); }
	virtual UDungeonCanvasComponent* GetDungeonCanvasInstance() const { return DungeonCanvasInstancePtr.Get(); }
	
public:

	// SWidget overrides
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	
	virtual void ZoomIn();
	virtual void ZoomOut();
	EDungeonCanvasViewportZoomMode GetZoomMode() const;
	virtual float GetCustomZoomLevel() const;
	virtual void SetCustomZoomLevel(float InZoomLevel);
	void SetZoomMode(const EDungeonCanvasViewportZoomMode ZoomMode);
	virtual double CalculateDisplayedZoomLevel() const;
	virtual void CalculateTextureDimensions(int32& OutWidth, int32& OutHeight, int32& OutDepth, int32& OutArraySize, bool bInIncludeBorderSize) const;
	virtual void GetBaseTextureSize( int& OutWidth, int& OutHeight ) const;
	
protected:

	/**
	 * Gets the displayed textures resolution as a string.
	 *
	 * @return Texture resolution string.
	 */
	FText GetDisplayedResolution() const;

protected:

	// Callback for the horizontal scroll bar.
	void HandleHorizontalScrollBarScrolled( float InScrollOffsetFraction );

	// Callback for getting the visibility of the horizontal scroll bar.
	EVisibility HandleHorizontalScrollBarVisibility( ) const;

	// Callback for the vertical scroll bar.
	void HandleVerticalScrollBarScrolled( float InScrollOffsetFraction );

	// Callback for getting the visibility of the horizontal scroll bar.
	EVisibility HandleVerticalScrollBarVisibility( ) const;

	// Level viewport client.
	TSharedPtr<class FDungeonCanvasViewportClient> ViewportClient;

	// Slate viewport for rendering and IO.
	TSharedPtr<FSceneViewport> Viewport;

	// Viewport widget.
	TSharedPtr<SViewport> ViewportWidget;

	// Vertical scrollbar.
	TSharedPtr<SScrollBar> CanvasViewportVerticalScrollBar;

	// Horizontal scrollbar.
	TSharedPtr<SScrollBar> CanvasViewportHorizontalScrollBar;

	FDungeonCanvasDrawSettings DrawSettings;
	TWeakObjectPtr<UDungeonCanvasComponent> DungeonCanvasInstancePtr;
	TWeakObjectPtr<UDungeonCanvasMaterialTheme> DungeonCanvasThemePtr;
	TWeakObjectPtr<UDungeonCanvasCamera> Camera;
	
	/** This toolkit's current zoom mode **/
	EDungeonCanvasViewportZoomMode ZoomMode;
	
	/** The texture's zoom factor. */
	double Zoom;
	
	// Is rendering currently enabled? (disabled when reimporting a texture)
	bool bIsRenderingEnabled = true;
};

