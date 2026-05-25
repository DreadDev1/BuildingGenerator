//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport3D/FlowDomainEdLayoutGraph3D.h"

class FSnapGridFlowDomainEdLayoutGraph3D : public FFlowDomainEdLayoutGraph3D {
private:
	virtual IFlowDomainPtr CreateDomain() const override;
};

