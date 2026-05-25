//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphLayoutManager.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"

bool FDAThemeGraphLayoutManager::LayoutMarkerNodeTree(const UEdGraphNode_DungeonThemeMarker* InMarkerNode, const TFunction<bool(const UEdGraphNode*, FSlateRect&)>& GetBoundsForNode) {
	 if (!InMarkerNode) {
		 return false;
	 }

	TArray<UEdGraphNode_DungeonThemeActorBase*> ActorNodes;
	if (UEdGraphPin* MarkerOutPin = InMarkerNode->GetOutputPin()) {
		for (UEdGraphPin* LinkedPin : MarkerOutPin->LinkedTo) {
			if (UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(LinkedPin->GetOwningNode())) {
				ActorNodes.Add(ActorNode);
			}
		} 
	}

	if (ActorNodes.Num() > 0) {
		ActorNodes.Sort([](const UEdGraphNode_DungeonThemeActorBase& A, const UEdGraphNode_DungeonThemeActorBase& B) {
			return A.ExecutionOrder < B.ExecutionOrder;
		});

		FSlateRect MarkerNodeRect;
		if (!GetBoundsForNode(InMarkerNode, MarkerNodeRect)) {
			return false;
		}

		check(ActorNodes.Num() >= 1);
		
		FSlateRect ActorNodeRect;
		if (!GetBoundsForNode(ActorNodes[0], ActorNodeRect)) {
			return false;
		}
		
		float NodeWidth = ActorNodeRect.GetSize().X;
		float TotalWidth = NodeWidth;
		constexpr float NodeSpacing = 20;
		
		TArray<float> NodeOffsets;
		NodeOffsets.Add(0);
		
		for (int i = 1; i < ActorNodes.Num(); i++) {
			NodeOffsets.Add(TotalWidth + NodeSpacing);
			
			if (!GetBoundsForNode(ActorNodes[i], ActorNodeRect)) {
				return false;
			}
			NodeWidth = ActorNodeRect.GetSize().X;
			
			TotalWidth += NodeSpacing + NodeWidth;
		}
		constexpr float NodeOffsetY = 100;

		float LaneOffset = MarkerNodeRect.GetCenter().X - TotalWidth * 0.5f;
		float NodeY = InMarkerNode->NodePosY + NodeOffsetY;

		for (int i = 0; i < ActorNodes.Num(); i++) {
			UEdGraphNode_DungeonThemeActorBase* ActorNode = ActorNodes[i];
			ActorNode->NodePosX = LaneOffset + NodeOffsets[i];
			ActorNode->NodePosY = NodeY;
			ActorNode->Modify();
			ActorNode->ExecutionOrder = i + 1;
		}
	}
	return true;
}


///////////////////////////////// FDANodeBoundsCache /////////////////////////////////
bool FDANodeBoundsCache::GetBounds(const UEdGraphNode* InNode, FSlateRect& OutRect) {
	if (InNode) {
		if (FSlateRect* RectPtr = NodeBounds.Find(InNode->NodeGuid)) {
			OutRect = *RectPtr;
			return true;
		}
	}
	
	return false;
}

