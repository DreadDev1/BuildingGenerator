//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Graph/DungeonThemeRuntimeGraph.h"

#include "Frameworks/ThemeEngine/Graph/DungeonThemeCompiledGraph.h"

FDungeonThemeRuntimeGraph::FDungeonThemeRuntimeGraph(const FDungeonThemeCompiledGraph& InCompiledGraph) {
	// Create nodes
	for (const FDungeonThemeMarkerNodeData& MarkerData : InCompiledGraph.MarkerNodes) {
		FDungeonThemeRuntimeMarkerNode* NewNode = new FDungeonThemeRuntimeMarkerNode(MarkerData);
		AllNodes.Add(NewNode->NodeGuid, NewNode);
	}
        
	for (const FDungeonThemeMarkerEmitterNodeData& EmitterData : InCompiledGraph.MarkerEmitterNodes) {
		FDungeonThemeRuntimeMarkerEmitterNode* NewNode = new FDungeonThemeRuntimeMarkerEmitterNode(EmitterData);
		AllNodes.Add(NewNode->NodeGuid, NewNode);
	}

	for (const FDungeonThemeVisualNodeData& VisualData : InCompiledGraph.VisualNodes) {
		FDungeonThemeRuntimeVisualNode* NewNode = new FDungeonThemeRuntimeVisualNode(VisualData);
		AllNodes.Add(NewNode->NodeGuid, NewNode);
	}
        
	// Set up links
	for (TPair<FGuid, FDungeonThemeRuntimeNode*>& Pair : AllNodes) {
		FDungeonThemeRuntimeNode* Node = Pair.Value;
            
		switch (Node->NodeType) {
		case EDungeonThemeNodeType::MarkerEmitter: {
			FDungeonThemeRuntimeMarkerEmitterNode* EmitterNode = static_cast<FDungeonThemeRuntimeMarkerEmitterNode*>(Node);
			EmitterNode->TargetMarker = static_cast<FDungeonThemeRuntimeMarkerNode*>(AllNodes[EmitterNode->NodeData.MarkerNodeGuid]);
                        
			for (const FGuid& ParentGuid : EmitterNode->NodeData.ParentNodes) {
				if (FDungeonThemeRuntimeNode* ParentNode = AllNodes.FindRef(ParentGuid)) {
					ParentNode->Children.Add(EmitterNode);
				}
			}
			break;
		}
		case EDungeonThemeNodeType::Visual: {
			FDungeonThemeRuntimeVisualNode* VisualNode = static_cast<FDungeonThemeRuntimeVisualNode*>(Node);
                    
			for (const FGuid& ParentGuid : VisualNode->NodeData.ParentNodes) {
				if (FDungeonThemeRuntimeNode* ParentNode = AllNodes.FindRef(ParentGuid)) {
					ParentNode->Children.Add(VisualNode);
				}
			}
			break;
		}
		default:
			break;
		}
	}

	// Initialize marker lookup
	for (const auto& Pair : AllNodes) {
		FDungeonThemeRuntimeNode* Node = Pair.Value;
		if (Node->NodeType == EDungeonThemeNodeType::Marker) {
			if (FDungeonThemeRuntimeMarkerNode* MarkerNode = static_cast<FDungeonThemeRuntimeMarkerNode*>(Node)) {
				NameToMarkerNodeMap.Add(MarkerNode->NodeData.MarkerName, MarkerNode);
			}
		}
	}

	// Sort the children based on the execution order
	for (auto& Entry : AllNodes) {
		if (FDungeonThemeRuntimeNode* Node = Entry.Value) {
			Node->Children.RemoveAll([&](const FDungeonThemeRuntimeNode* InNode) { return !InNode; });
			if (Node->Children.Num() > 1) {
				Node->Children.Sort([this](const FDungeonThemeRuntimeNode& A, const FDungeonThemeRuntimeNode& B) {
					return A.GetExecutionOrder() < B.GetExecutionOrder();
				});
			}
		}
	}
}

FDungeonThemeRuntimeGraph::~FDungeonThemeRuntimeGraph() {
	for (TPair<FGuid, FDungeonThemeRuntimeNode*>& Pair : AllNodes) {
		delete Pair.Value;
	}
}

void FDungeonThemeRuntimeGraph::TraverseVisualNodes(const FString& InMarkerName, const TFunction<void(const FDungeonThemeVisualNodeData&)>& Visit) const {
	TraverseVisualNodes(GetMarkerNodeByName(InMarkerName), Visit);
}

void FDungeonThemeRuntimeGraph::TraverseVisualNodes(const FDungeonThemeRuntimeMarkerNode* InNode, const TFunction<void(const FDungeonThemeVisualNodeData&)>& Visit) {
	TraverseVisualNodesImpl(InNode, {}, Visit);
}

bool FDungeonThemeRuntimeGraph::HasMarkerTree(const FString& InMarkerName) const {
	if (FDungeonThemeRuntimeMarkerNode* const* MarkerNodePtr = NameToMarkerNodeMap.Find(InMarkerName)) {
		const FDungeonThemeRuntimeMarkerNode* MarkerNode = *MarkerNodePtr;
		return MarkerNode->Children.Num() > 0;
	}
	return false;
}

void FDungeonThemeRuntimeGraph::TraverseVisualNodesImpl(const FDungeonThemeRuntimeNode* InNode, const TSet<FGuid>& InVisited, const TFunction<void(const FDungeonThemeVisualNodeData&)>& VisitNode) {
	if (!InNode) {
		return;
	}
	
	if (InVisited.Contains(InNode->NodeGuid)) {
		return;
	}
	
	TSet<FGuid> Visited = InVisited;
	Visited.Add(InNode->NodeGuid);

	if (InNode->NodeType == EDungeonThemeNodeType::Visual) {
		if (const FDungeonThemeRuntimeVisualNode* VisualNode = static_cast<const FDungeonThemeRuntimeVisualNode*>(InNode)) {
			VisitNode(VisualNode->NodeData);
		}
	}
	else if (InNode->NodeType == EDungeonThemeNodeType::MarkerEmitter) {
		if (const FDungeonThemeRuntimeMarkerEmitterNode* MarkerEmitterNode = static_cast<const FDungeonThemeRuntimeMarkerEmitterNode*>(InNode)) {
			TraverseVisualNodesImpl(MarkerEmitterNode->TargetMarker, Visited, VisitNode);
		} 
	}
	
	// Visit the child nodes
	for (FDungeonThemeRuntimeNode* Child : InNode->Children) {
		if (Child && !Visited.Contains(Child->NodeGuid)) {
			TraverseVisualNodesImpl(Child, Visited, VisitNode);
		}
	}
}

TArray<FDungeonThemeRuntimeMarkerNode*> FDungeonThemeRuntimeGraph::GetAllMarkerNodes() const {
	TArray<FDungeonThemeRuntimeMarkerNode*> Result;
	for (const auto& Pair : AllNodes) {
		FDungeonThemeRuntimeNode* Node = Pair.Value;
		if (Node->NodeType == EDungeonThemeNodeType::Marker) {
			Result.Add(static_cast<FDungeonThemeRuntimeMarkerNode*>(Node));
		}
	}
	return Result;
}

FDungeonThemeRuntimeMarkerNode* FDungeonThemeRuntimeGraph::GetMarkerNodeByName(const FString& InName) const {
	return NameToMarkerNodeMap.FindRef(InName);
}

FDungeonThemeRuntimeNode* FDungeonThemeRuntimeGraph::GetNode(const FGuid& InNodeGuid) const {
	return AllNodes.FindRef(InNodeGuid);
}

