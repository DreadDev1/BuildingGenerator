//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeFactory.h"

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"
#include "Frameworks/Forge/Graph/Slate/SDungeonForgeEditorGraphNode.h"

TSharedPtr<SGraphNode> FDungeonForgeEditorGraphNodeFactory::CreateNode(UEdGraphNode* InNode) const {
	if (UDungeonForgeEditorGraphNodeBase* GraphNode = Cast<UDungeonForgeEditorGraphNodeBase>(InNode)) {
		TSharedPtr<SGraphNode> VisualNode;
		
		const UDungeonForgeGraphNode* ForgeNode = GraphNode->GetForgeNode();
		SAssignNew(VisualNode, SDungeonForgeEditorGraphNode, GraphNode);
		
		VisualNode->SlatePrepass();

		return VisualNode.ToSharedRef();
	}

	return nullptr;
}

