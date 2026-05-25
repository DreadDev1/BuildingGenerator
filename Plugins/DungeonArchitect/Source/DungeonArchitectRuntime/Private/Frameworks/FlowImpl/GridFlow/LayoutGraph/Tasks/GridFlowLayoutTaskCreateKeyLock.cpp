//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/Tasks/GridFlowLayoutTaskCreateKeyLock.h"


TSubclassOf<UFlowGraphItem> UGridFlowLayoutTaskCreateKeyLock::GetKeyItemClass() const {
	return UGridFlowGraphItem::StaticClass();
}

void UGridFlowLayoutTaskCreateKeyLock::ExtendKeyItem(UFlowGraphItem* InItem) const {
	if (UGridFlowGraphItem* GraphItem = Cast<UGridFlowGraphItem>(InItem)) {
		GraphItem->PlacementSettings = KeyPlacement;
	}
}

