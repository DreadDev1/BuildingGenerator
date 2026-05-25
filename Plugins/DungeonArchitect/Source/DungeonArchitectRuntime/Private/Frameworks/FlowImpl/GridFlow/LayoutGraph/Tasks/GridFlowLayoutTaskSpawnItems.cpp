//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/Tasks/GridFlowLayoutTaskSpawnItems.h"


TSubclassOf<UFlowGraphItem> UGridFlowLayoutTaskSpawnItems::GetItemClass() const {
	return UGridFlowGraphItem::StaticClass();
}

void UGridFlowLayoutTaskSpawnItems::ExtendItem(UFlowGraphItem* InItem) const {
	if (UGridFlowGraphItem* GraphItem = Cast<UGridFlowGraphItem>(InItem)) {
		GraphItem->PlacementSettings = PlacementSettings;
	}
}

