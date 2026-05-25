//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DAGInventoryGridWidget.h"

#include "Inventory/Components/DAGInventoryComponent.h"
#include "Inventory/UI/DAGInventorySlotViewWidget.h"
#include "Inventory/UI/DAGInventorySlotWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragDropManager.h"

#include "Blueprint/WidgetTree.h"
#include "Components/WrapBox.h"

void UDAG_InventoryGridWidget::InitializeGrid(UDAG_InventoryComponent* Inventory) {
	InventoryComponent = Inventory;
	if (!InventoryComponent) {
		return;
	}
	
	if (!SlotWidgetClass) {
		SlotWidgetClass = UDAG_InventorySlotWidget::StaticClass();
	}
	
	if (InventoryGrid) {
		// Listen to slot changes so we can update the UI	
		InventoryComponent->Inventory.OnSlotChanged.RemoveAll(this);
		InventoryComponent->Inventory.OnSlotChanged.AddDynamic(this, &UDAG_InventoryGridWidget::OnInventorySlotChanged);
		
		InventoryGrid->ClearChildren();
		const int32 NumSlots = InventoryComponent->GetNumSlots();
		for (int SlotIdx = 0; SlotIdx < NumSlots; SlotIdx++) {
			UDAG_InventorySlotWidget* SlotWidget = CreateWidget<UDAG_InventorySlotWidget>(this, SlotWidgetClass);
			SlotWidget->SlotIndex = SlotIdx;
			SlotWidget->ParentGridWidget = this;
			FDAG_InventorySlot SlotItem;
			if (InventoryComponent->GetItemAt(SlotIdx, SlotItem)) {
				if (SlotWidget->SlotView) {
					SlotWidget->SlotView->UpdateView(SlotItem.ItemDefinition, SlotItem.StackCount);
				}
			}
			if (SlotWidget) {
				InventoryGrid->AddChild(SlotWidget);
			}
		}
	}

	// Find the drag drop manager in the parent hierarchy
	if (bEnableDragSupport) {
		UWidget* SearchWidget = this;
		while (SearchWidget) {
			if (UDAG_InventoryDragDropManager* Manager = Cast<UDAG_InventoryDragDropManager>(SearchWidget)) {
				DragDropManager = Manager;
				break;
			}
			SearchWidget = SearchWidget->GetParent();
		}
	}
}

IDAG_InventoryDragMode* UDAG_InventoryGridWidget::GetDragMode() const {
	return DragDropManager ? DragDropManager->GetDragMode() : nullptr;
}

FReply UDAG_InventoryGridWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	return FReply::Handled();
}

void UDAG_InventoryGridWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (DragDropManager) {
		DragDropManager->OnInventoryGridMouseEnter(this);
	}
}

void UDAG_InventoryGridWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent) {
	Super::NativeOnMouseLeave(InMouseEvent);

	if (DragDropManager) {
		DragDropManager->OnInventoryGridMouseLeave(this);
	}
}

void UDAG_InventoryGridWidget::OnInventorySlotChanged(int32 InSlotIndex, const FDAG_InventorySlot& InSlot) {
	if (InventoryGrid && InventoryGrid->GetChildrenCount() > InSlotIndex) {
		UDAG_InventorySlotWidget* SlotWidget = Cast<UDAG_InventorySlotWidget>(InventoryGrid->GetChildAt(InSlotIndex));
		if (SlotWidget && SlotWidget->SlotView) {
			SlotWidget->SlotView->UpdateView(InSlot.ItemDefinition, InSlot.StackCount);
		}
	}
}

