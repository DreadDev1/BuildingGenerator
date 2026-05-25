//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/GridFlow/GridFlowDomainEdTilemap.h"

#include "Frameworks/FlowImpl/GridFlow/Tilemap/GridFlowTilemapDomain.h"

IFlowDomainPtr FGridFlowDomainEdTilemap::CreateDomain() const {
	return MakeShareable(new FGridFlowTilemapDomain);
}

