//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/GraphEditor/FlowDomainEdLayoutGraph.h"

class FGridFlowDomainEdLayoutGraph : public FFlowDomainEdLayoutGraph {
private:
	virtual UFlowLayoutEdGraph* CreateGraph() const override;
	virtual UFlowLayoutEdGraphNode* CreateGraphNode() const override;
	virtual IFlowDomainPtr CreateDomain() const override;
};

