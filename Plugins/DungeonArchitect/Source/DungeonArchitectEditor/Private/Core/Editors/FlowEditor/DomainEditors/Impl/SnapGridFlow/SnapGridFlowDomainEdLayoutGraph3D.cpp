//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/SnapGridFlow/SnapGridFlowDomainEdLayoutGraph3D.h"

#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraphDomain.h"

IFlowDomainPtr FSnapGridFlowDomainEdLayoutGraph3D::CreateDomain() const {
	return MakeShareable(new FSnapGridFlowAbstractGraphDomain);
}

