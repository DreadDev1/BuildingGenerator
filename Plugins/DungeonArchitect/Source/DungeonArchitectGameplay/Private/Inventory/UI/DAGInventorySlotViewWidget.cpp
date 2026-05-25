//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DAGInventorySlotViewWidget.h"

#include "Item/DAGItemDefinition.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UDAG_InventorySlotViewWidget::UpdateView(UDAG_ItemDefinition* ItemDefinition, int32 Count) {
	bool bIsEmpty = !ItemDefinition || Count == 0;
	
	if (ItemImage) {
		if (!bIsEmpty) {
			ItemImage->SetBrush(ItemDefinition->ItemIcon);
		}
		else {
			FSlateBrush EmptyBrush;
			EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
			ItemImage->SetBrush(EmptyBrush);
		}
	}
	if (ItemCountText) {
		if (!bIsEmpty) {
			ItemCountText->SetText(FText::AsNumber(Count));
			ItemCountText->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			ItemCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

