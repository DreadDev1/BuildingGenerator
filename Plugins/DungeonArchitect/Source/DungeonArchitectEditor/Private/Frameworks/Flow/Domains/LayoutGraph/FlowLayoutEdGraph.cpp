//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraph.h"

#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraphSchema.h"

UFlowLayoutEdGraph::UFlowLayoutEdGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
	Schema = UFlowLayoutEdGraphSchema::StaticClass();
}


