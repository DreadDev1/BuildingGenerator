//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SGraphPanel.h"

class UEdGraphNode_DungeonThemeActorBase;
class UEdGraphNode_DungeonThemeMarker;

class FDAThemeGraphLayoutManager {
public:
	static bool LayoutMarkerNodeTree(const UEdGraphNode_DungeonThemeMarker* InMarkerNode, const TFunction<bool(const UEdGraphNode*, FSlateRect&)>& GetBoundsForNode);
};

class FDANodeBoundsCache {
public:
	template<typename TNode>
	void CacheBounds(const TArray<TNode*>& InNodes, const TSharedPtr<SGraphEditor>& InGraphEditor) {
		if (!InGraphEditor.IsValid()) {
			return;
		}
	
		for (TNode* Node : InNodes) {
			if (Node) {
				FSlateRect& RectRef = NodeBounds.FindOrAdd(Node->NodeGuid);
				if (!InGraphEditor->GetBoundsForNode(Node, RectRef, 0)) {
					if (SGraphPanel* GraphPanel = InGraphEditor->GetGraphPanel()) {
						TSharedPtr<SGraphNode> GraphNode = GraphPanel->GetNodeWidgetFromGuid(Node->NodeGuid);
						if (GraphNode.IsValid()) {
							const FGeometry& NodeGeometry = GraphNode->GetCachedGeometry();
							RectRef = NodeGeometry.GetRenderBoundingRect();
						}
						else {
							RectRef = FSlateRect(0, 0, 150, 150);
						}
					}
				}
			}
		}
	}
	
	bool GetBounds(const UEdGraphNode* InNode, FSlateRect& OutRect);
	
private:
	TMap<FGuid, FSlateRect> NodeBounds;
};