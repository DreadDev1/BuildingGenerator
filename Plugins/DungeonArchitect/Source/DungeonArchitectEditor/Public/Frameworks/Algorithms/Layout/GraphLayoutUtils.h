//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Common/Utils/DungeonGraphUtils.h"

#include "EdGraph/EdGraph.h"

namespace GraphLayoutUtils {
    template <typename TLayoutNode>
    struct TLayoutGraphGraph {
        TArray<TSharedPtr<TLayoutNode>> Nodes;
    };

    template <typename TLayoutNode>
    class TLayoutGraphGraphQuery {
    public:
        TLayoutGraphGraphQuery(const TLayoutGraphGraph<TLayoutNode>& InGraph) {
            for (const TSharedPtr<TLayoutNode>& Node : InGraph.Nodes) {
                if (Node.IsValid()) {
                    NodeLookup.FindOrAdd(Node->NodeId, Node);
                }
            }
        }

        TSharedPtr<TLayoutNode> GetNode(const FGuid& InNodeId) {
            return NodeLookup.FindRef(InNodeId).Pin();
        }
        
    private:
        TMap<FGuid, TWeakPtr<TLayoutNode>> NodeLookup;
    };

    template <typename TLayoutNode>
    TSharedPtr<TLayoutNode> FindRootNode(TLayoutGraphGraph<TLayoutNode>& InGraph) {
        TSharedPtr<TLayoutNode> RootNode;
        if (InGraph.Nodes.Num() > 0) {
            TSharedPtr<TLayoutNode> Parent = InGraph.Nodes[0];
            TSet<TSharedPtr<TLayoutNode>> Visited;
            while (Parent.IsValid() || Visited.Contains(Parent)) {
                Visited.Add(Parent);
                RootNode = Parent;
                Parent = (RootNode->IncomingNodes.Num() > 0)
                    ? RootNode->IncomingNodes[0]
                    : nullptr;
            }
        }

        return RootNode;
    }

    

    template <typename TLayoutNode>
    TLayoutGraphGraph<TLayoutNode> GenerateLayoutGraph(UEdGraph* InGraph, UEdGraphNode* InRootNode = nullptr) {
        // Build the node graph in memory
        TMap<TWeakObjectPtr<UEdGraphNode>, TSharedPtr<TLayoutNode>> NodeMap;
        {
            TArray<UEdGraphNode*> AllNodes = InRootNode
                ? FDungeonGraphUtils::GetAllNodesInHierarchy(InRootNode)
                : InGraph->Nodes;
            
            for (UEdGraphNode* Node : AllNodes) {
                if (Node) {
                    TSharedPtr<TLayoutNode> NodePtr = MakeShareable(new TLayoutNode);
                    NodePtr->NodeId = Node->NodeGuid;
                    NodeMap.Add(Node, NodePtr);
                }
            }
        }

        // Link the nodes
        for (auto& Entry : NodeMap) {
            TWeakObjectPtr<UEdGraphNode> EdNode = Entry.Key;
            TSharedPtr<TLayoutNode> Node = Entry.Value;
            if (UEdGraphPin* OutputPin = FDungeonGraphUtils::GetFirstOutputPin(EdNode.Get())) {
                for (UEdGraphPin* OutgoingPin : OutputPin->LinkedTo) {
                    UEdGraphNode* OutgoingEdNode = OutgoingPin->GetOwningNode();
                    if (OutgoingEdNode && NodeMap.Contains(OutgoingEdNode)) {
                        TSharedPtr<TLayoutNode> OutgoingNode = NodeMap[OutgoingEdNode];
                        Node->OutgoingNodes.Add(OutgoingNode);
                        OutgoingNode->IncomingNodes.Add(Node);
                    }
                }
            }
        }

        TLayoutGraphGraph<TLayoutNode> Result;
        NodeMap.GenerateValueArray(Result.Nodes);
        
        return Result;
    }
};

