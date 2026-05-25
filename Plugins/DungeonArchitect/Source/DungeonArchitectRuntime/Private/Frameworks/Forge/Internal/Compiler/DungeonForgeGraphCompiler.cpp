//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Internal/Compiler/DungeonForgeGraphCompiler.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphCompilationData.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphEdge.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

FDungeonForgeGraphCompiler::FDungeonForgeGraphCompiler(bool bInIsCooking)
	: bIsCooking(bInIsCooking)
{
	
}

void FDungeonForgeGraphCompiler::Compile(UDungeonForgeGraph* InGraph, TArray<FDungeonForgeGraphTask>& OutCompiledTasks) {
#if !WITH_EDITOR
	// Don't need to compile if the tasks are already cooked.
	const UDungeonForgeGraphCompilationData* CookedData = InGraph ? InGraph->GetCookedCompilationData() : nullptr;
	if (CookedData) {
		OutCompiledTasks = CookedData->Tasks;
		for (FDungeonForgeGraphTask& CompiledTask : OutCompiledTasks) {
			CompiledTask.LoadCookedData();
		}
		return;
	}
#endif

	OutCompiledTasks = CompileGraph(InGraph);
}

TArray<FDungeonForgeGraphTask> FDungeonForgeGraphCompiler::CompileGraph(UDungeonForgeGraph* InGraph) {
	TArray<FDungeonForgeGraphTask> CompiledTasks;
	TMap<const UDungeonForgeGraphNode*, FDungeonForgeTaskId> IdMapping;
	TArray<const UDungeonForgeGraphNode*> NodeQueue;
	TSet<const UDungeonForgeGraphNode*> TouchedNodes;
	
	// Prime the node queue with all nodes that have no inbound edges
	FDungeonForgeTaskId NextId = {};
	for (const UDungeonForgeGraphNode* Node : InGraph->GetNodes()) {
		const FDungeonForgeTaskId NodeId = NextId++;
		if(!Node->HasInboundEdges()) {
			NodeQueue.Add(Node);
			TouchedNodes.Add(Node);
		}
		
		IdMapping.Add(Node, NodeId);
	}
	
	while (NodeQueue.Num() > 0) {
		const UDungeonForgeGraphNode* Node = NodeQueue.Pop();
		FDungeonForgeTaskId NodeId = IdMapping.FindChecked(Node);
		FDungeonForgeGraphTask& Task = CompiledTasks.Emplace_GetRef();
		Task.Node = Node;
		Task.NodeId = NodeId;

		// Populate the task inputs
		for (const UDungeonForgeGraphPin* InputPin : Node->InputPins) {
			for (const UDungeonForgeGraphEdge* InboundEdge : InputPin->Edges) {
				if (!InboundEdge->IsValid()) {
					UE_LOG(LogDungeonForge, Warning, TEXT("Unbound inbound edge"));
					continue;
				}

				if (FDungeonForgeTaskId* InboundId = IdMapping.Find(InboundEdge->InputPin->Node)) {
					Task.Inputs.Emplace(*InboundId, InboundEdge->InputPin->Properties, InboundEdge->OutputPin->Properties);
				}
				else {
					UE_LOG(LogDungeonForge, Error, TEXT("Inconsistent node linkage on node '%s'"), *Node->GetFName().ToString());
					return TArray<FDungeonForgeGraphTask>();
				}
			}
		}

		// Push the next nodes to the queue
		for (const UDungeonForgeGraphPin* OutPin : Node->OutputPins) {
			for (const UDungeonForgeGraphEdge* OutboundEdge : OutPin->Edges) {
				if (!OutboundEdge->IsValid()) {
					UE_LOG(LogDungeonForge, Warning, TEXT("Unbound outbound edge"));
					continue;
				}

				const UDungeonForgeGraphNode* OutboundNode = OutboundEdge->OutputPin->Node;
				check(OutboundNode);

				if (TouchedNodes.Contains(OutboundNode)) {
					continue;
				}

				bool bAllPrerequisitesMet = true;

				for (const UDungeonForgeGraphPin* OutboundNodeInputPin : OutboundNode->InputPins) {
					for (const UDungeonForgeGraphEdge* OutboundNodeInboundEdge : OutboundNodeInputPin->Edges) {
						if (OutboundNodeInboundEdge->IsValid()) {
							bAllPrerequisitesMet &= IdMapping.Contains(OutboundNodeInboundEdge->InputPin->Node);
						}
					}
				}

				if (bAllPrerequisitesMet) {
					NodeQueue.Add(OutboundNode);
					TouchedNodes.Add(OutboundNode);
				}
			}
		}
	}

	return CompiledTasks;
}

