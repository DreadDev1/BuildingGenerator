//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FFlowDomainEdViewportClient2D;

class SFlowDomainEdViewport2D : public SEditorViewport {
public:
	SLATE_BEGIN_ARGS(SFlowDomainEdViewport2D) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs, TSharedPtr<FFlowDomainEdViewportClient2D> InEditorViewportClient);
	virtual ~SFlowDomainEdViewport2D() override;
	
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual EVisibility OnGetViewportContentVisibility() const override;
	
	void SetParentTab(TSharedRef<SDockTab> InParentTab) { ParentTab = InParentTab; }
	TSharedPtr<FFlowDomainEdViewportClient2D> GetViewportClient2D() const { return EditorViewportClient; }

	/** Determines the visibility of the viewport. */
	virtual bool IsVisible() const override;

protected:
	TWeakPtr<SDockTab> ParentTab;
	TSharedPtr<FFlowDomainEdViewportClient2D> EditorViewportClient;
};

