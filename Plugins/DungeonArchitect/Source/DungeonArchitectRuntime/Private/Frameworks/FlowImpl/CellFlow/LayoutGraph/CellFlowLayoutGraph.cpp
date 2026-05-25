//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"


void UCellFlowLayoutGraph::CloneFromStateObject(const UObject* SourceObject) {
	const UCellFlowLayoutGraph* SourceGraph = Cast<UCellFlowLayoutGraph>(SourceObject);
	if (!SourceGraph) return;
	CopyStateFrom(SourceGraph);
}

