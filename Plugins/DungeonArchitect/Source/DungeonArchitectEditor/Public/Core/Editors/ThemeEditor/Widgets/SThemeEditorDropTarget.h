//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SAssetDropTarget.h"

/** The list view mode of the asset view */
class DUNGEONARCHITECTEDITOR_API SThemeEditorDropTarget : public SAssetDropTarget {
public:
	typedef SAssetDropTarget Super;
    
	SLATE_BEGIN_ARGS(SThemeEditorDropTarget) : _bSupportsMultiDrop(true)
	{ }
		/* Content to display for the in the drop target */
		SLATE_DEFAULT_SLOT( FArguments, Content )
		/** Called when a valid asset is dropped */
		SLATE_EVENT( FOnAssetsDropped, OnAssetsDropped )
		/** Called to check if an asset is acceptable for dropping */
		SLATE_EVENT( FAreAssetsAcceptableForDrop, OnAreAssetsAcceptableForDrop )
		/** Called to check if an asset is acceptable for dropping if you also plan on returning a reason text */
		SLATE_EVENT( FAreAssetsAcceptableForDropWithReason, OnAreAssetsAcceptableForDropWithReason )
		/** Sets if this drop target can support multiple assets dropped, or only supports a single asset dropped at a time. False by default for legacy behavior. */
		SLATE_ARGUMENT( bool, bSupportsMultiDrop )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs );
	
    virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

    FVector2D GetPanelCoordDropPosition() const { return PanelCoordDropPosition; }

private:
    FVector2D PanelCoordDropPosition;
};

