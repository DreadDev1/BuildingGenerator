//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Tilemap/FlowDomainEdTilemap.h"

class FGridFlowDomainEdTilemap : public FFlowDomainEdTilemap {
private:
	virtual IFlowDomainPtr CreateDomain() const override;
};

