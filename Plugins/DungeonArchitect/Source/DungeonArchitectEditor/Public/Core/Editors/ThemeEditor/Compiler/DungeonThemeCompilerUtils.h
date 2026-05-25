//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeBase.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"

struct FDungeonThemeMarkerEmitterNodeData;
class UEdGraphNode_DungeonThemeMarkerEmitter;
struct FDungeonThemeMarkerNodeData;
class UEdGraphNode_DungeonThemeMarker;
struct FDungeonThemeVisualNodeData;
class UDungeonThemeAsset;

namespace DA::ThemeGraph::Compiler {
	template <typename T>
	void GetChildNodes(UEdGraphNode_DungeonThemeBase* ParentNode, TArray<T*>& OutChildren) {
		for (UEdGraphPin* ChildPin : ParentNode->GetOutputPin()->LinkedTo) {
			if (ChildPin) {
				if (T* DesiredNode = Cast<T>(ChildPin->GetOwningNode())) {
					OutChildren.Add(DesiredNode);
				}
			}
		}
	}

	struct ExecutionSortComparer {
		bool operator()(const UEdGraphNode_DungeonThemeActorBase& A, const UEdGraphNode_DungeonThemeActorBase& B) const {
			return A.ExecutionOrder < B.ExecutionOrder;
		}
	};

	template <typename T>
	void CloneUObjectArray(UObject* Outer, const TArray<TObjectPtr<T>>& SourceList, TArray<TObjectPtr<T>>& DestList) {
		DestList.Reset();
		for (T* Source : SourceList) {
			if (!Source) continue;
			T* Clone = NewObject<T>(Outer, Source->GetClass(), NAME_None, RF_NoFlags, Source);
			DestList.Add(Clone);
		}
	}


	template<typename TNode>
	void CreateNodeMap(UEdGraph* InGraph, TMap<FGuid, TNode*>& OutResult) {
		OutResult.Reset();
		if (InGraph) {
			TArray<TNode*> Nodes;
			InGraph->GetNodesOfClass<TNode>(Nodes);
			for (TNode* Node : Nodes) {
				TNode*& NodeRef = OutResult.FindOrAdd(Node->NodeGuid);
				NodeRef = Node;
			} 
		}
	}
}

class FDungeonThemeCompilerUtils {
public:
	static void CreateNodeData(UDungeonThemeAsset* InOwner, const UEdGraphNode_DungeonThemeActorBase* InActorNode, FDungeonThemeVisualNodeData& OutNodeData);
	static void CreateNodeData(const UEdGraphNode_DungeonThemeMarker* InMarkerNode, FDungeonThemeMarkerNodeData& OutNodeData);
	static void CreateNodeData(const UEdGraphNode_DungeonThemeMarkerEmitter* InMarkerEmitterNode, FDungeonThemeMarkerEmitterNodeData& OutNodeData);
};

