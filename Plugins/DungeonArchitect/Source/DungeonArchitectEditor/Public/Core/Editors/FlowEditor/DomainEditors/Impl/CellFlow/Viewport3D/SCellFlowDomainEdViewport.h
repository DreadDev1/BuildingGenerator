//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport3D/FlowDomainEdViewport3D.h"

#include "Framework/MultiBox/MultiBoxExtender.h"
#include "SViewportToolBar.h"

class DUNGEONARCHITECTEDITOR_API SCellFlowDomainEdViewport : public SFlowDomainEdViewport3D {
public:
	SLATE_BEGIN_ARGS(SCellFlowDomainEdViewport) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	virtual TSharedPtr<SWidget> BuildViewportToolbar() override;

private:
	EVisibility GetToolbarVisibility() const;
	
};

class DUNGEONARCHITECTEDITOR_API SCellFlowDomainEdViewportToolbar : public SViewportToolBar {
public:
	SLATE_BEGIN_ARGS(SCellFlowDomainEdViewportToolbar) {}
		SLATE_ARGUMENT(TSharedPtr<class SCellFlowDomainEdViewport>, Viewport)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> GeneratePropertiesMenu() const;
	TSharedRef<SWidget> GenerateCameraMenu() const;
	TSharedPtr<FExtender> GetViewMenuExtender();

private:
	/** The viewport that we are in */
	TWeakPtr<SCellFlowDomainEdViewport> Viewport;

};

