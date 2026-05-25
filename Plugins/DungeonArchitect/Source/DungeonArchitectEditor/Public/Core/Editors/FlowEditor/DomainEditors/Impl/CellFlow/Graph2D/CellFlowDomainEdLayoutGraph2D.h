//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdLayoutGraph2D.h"

class FCellFlowDomainEdLayoutGraph2D : public FFlowDomainEdLayoutGraph2D {
public:
	FCellFlowDomainEdLayoutGraph2D();
	
private:
	virtual IFlowDomainPtr CreateDomain() const override;
	virtual TSharedPtr<FFlowDomainEdViewportClient2D> CreateViewportClient(const FDomainEdInitSettings& InSettings) const override;

};

