//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DragDrop/Modes/DAGInventoryDragMode_DragDrop.h"

#include "Inventory/Components/DAGInventoryComponent.h"
#include "Inventory/UI/DAGInventorySlotViewWidget.h"
#include "Inventory/UI/DAGInventorySlotWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragDropData.h"

#include "Blueprint/WidgetBlueprintLibrary.h"

FReply FDAG_InventoryDragDropMode_DragDrop::HandleSlotMouseButtonDown(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) {
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, InEvent.SlotWidget, EKeys::LeftMouseButton).NativeReply;
	}
	return FReply::Unhandled();
}

void FDAG_InventoryDragDropMode_DragDrop::HandleSlotDragDetected(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) {
	// Implement drag detected logic for Drag and Drop
	if (UDAG_InventorySlotWidget* SlotWidget = InEvent.SlotWidget) {
		UDAG_InventoryDragDropData* DragDropOperation = NewObject<UDAG_InventoryDragDropData>();
		DragDropOperation->DefaultDragVisual = SlotWidget->SlotView;
		DragDropOperation->SlotIndex = SlotWidget->SlotIndex;
		DragDropOperation->InventoryComponent = SlotWidget->GetInventoryComponent();
		OutOperation = DragDropOperation;
	}
}

bool FDAG_InventoryDragDropMode_DragDrop::HandleSlotDrop(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	if (UDAG_InventorySlotWidget* SlotWidget = InEvent.SlotWidget) {
		// Implement drop logic for Drag and Drop
		UDAG_InventoryDragDropData* DragDropOperation = Cast<UDAG_InventoryDragDropData>(InOperation);
		UDAG_InventoryComponent* InventoryComponent = SlotWidget->GetInventoryComponent();

		if (DragDropOperation && DragDropOperation->InventoryComponent && InventoryComponent) {
			if (InventoryComponent == DragDropOperation->InventoryComponent) {
				// Part of the same inventory
				InventoryComponent->SwapItems(DragDropOperation->SlotIndex, SlotWidget->SlotIndex);
			} else {
				// Coming from another inventory
				// TODO: Implement cross-inventory swap
			}
			return true;
		}
	}
	return false;
}

