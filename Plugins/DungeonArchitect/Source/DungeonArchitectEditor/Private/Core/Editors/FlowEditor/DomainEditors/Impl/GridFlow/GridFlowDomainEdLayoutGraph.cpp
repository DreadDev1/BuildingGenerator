//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/GridFlow/GridFlowDomainEdLayoutGraph.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Impl/GridFlow/GridFlowLayoutEdGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Impl/GridFlow/GridFlowLayoutEdGraphNode.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraphDomain.h"

//////////////////////////////////// FGridFlowDomainEdAbstractGraph ////////////////////////////////////

IFlowDomainPtr FGridFlowDomainEdLayoutGraph::CreateDomain() const {
	return MakeShareable(new FGridFlowAbstractGraphDomain);
}

UFlowLayoutEdGraph* FGridFlowDomainEdLayoutGraph::CreateGraph() const {
	return NewObject<UGridFlowLayoutEdGraph>();
}

UFlowLayoutEdGraphNode* FGridFlowDomainEdLayoutGraph::CreateGraphNode() const {
	FGraphNodeCreator<UGridFlowLayoutEdGraphNode> EdNodeCreator(*AbstractGraph);
	UGridFlowLayoutEdGraphNode* EdNode = EdNodeCreator.CreateNode();
	EdNodeCreator.Finalize();

	return EdNode;
}



