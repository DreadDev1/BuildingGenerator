//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/Tasks/GridFlowLayoutTaskCreatePath.h"

#include "Frameworks/FlowImpl/GridFlow/Common/GridFlowItem.h"

TSubclassOf<UFlowGraphItem> UGridFlowLayoutTaskCreatePath::GetTeleporterItemClass() const {
	return UGridFlowGraphItem::StaticClass();
}

void UGridFlowLayoutTaskCreatePath::ExtendTeleporterItem(UFlowGraphItem* InItem) const {
	if (UGridFlowGraphItem* GraphItem = Cast<UGridFlowGraphItem>(InItem)) {
		GraphItem->PlacementSettings = TeleporterPlacement;
	}
}

