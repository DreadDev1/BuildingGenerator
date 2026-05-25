//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DAGInventorySlotWidget.h"

#include "Inventory/UI/DAGInventoryGridWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragDropManager.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

FReply UDAG_InventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotMouseButtonDown(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InMouseEvent);
	}
	else { 
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
}

FReply UDAG_InventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotMouseButtonUp(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InMouseEvent);
	}
	else {
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}
}

FReply UDAG_InventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotMouseButtonDoubleClick(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InMouseEvent);
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UDAG_InventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) {
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		DragMode->HandleSlotDragDetected(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InMouseEvent, OutOperation);
	}

	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
}

bool UDAG_InventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	bIsDraggedOver = false;
	
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotDrop(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InDragDropEvent, InOperation);
	}
	else {
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}
}

void UDAG_InventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotMouseEnter(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InMouseEvent);
	}
	
	if (UDAG_InventoryDragDropManager* DragDropManager = GetDragDropManager()) {
		DragDropManager->OnInventorySlotMouseEnter(this);
	}
}

void UDAG_InventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent) {
	Super::NativeOnMouseLeave(InMouseEvent);
	bIsDraggedOver = false;

	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotMouseLeave(FDAG_InventorySlotDragEvent::Create(this), InMouseEvent);
	}
	
	if (UDAG_InventoryDragDropManager* DragDropManager = GetDragDropManager()) {
		DragDropManager->OnInventorySlotMouseLeave(this);
	}
}

void UDAG_InventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	bIsDraggedOver = false;
	
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		DragMode->HandleSlotDragCancelled(FDAG_InventorySlotDragEvent::Create(this), InDragDropEvent, InOperation);
	}
}

void UDAG_InventorySlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	bIsDraggedOver = true;
	
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		DragMode->HandleSlotDragEnter(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InDragDropEvent, InOperation);
	}
}

void UDAG_InventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bIsDraggedOver = false;

	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		DragMode->HandleSlotDragLeave(FDAG_InventorySlotDragEvent::Create(this), InDragDropEvent, InOperation);
	}
}

bool UDAG_InventorySlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	bIsDraggedOver = true;
	
	if (IDAG_InventoryDragMode* DragMode = GetDragMode()) {
		return DragMode->HandleSlotDragOver(FDAG_InventorySlotDragEvent::Create(this), InGeometry, InDragDropEvent, InOperation);
	}
	
	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

UDAG_InventoryComponent* UDAG_InventorySlotWidget::GetInventoryComponent() const {
	return ParentGridWidget ? ParentGridWidget->InventoryComponent : nullptr;
}

UDAG_InventoryDragDropManager* UDAG_InventorySlotWidget::GetDragDropManager() const {
	return ParentGridWidget ? ParentGridWidget->GetDragDropManager() : nullptr;
}

IDAG_InventoryDragMode* UDAG_InventorySlotWidget::GetDragMode() const {
	return ParentGridWidget ? ParentGridWidget->GetDragMode() : nullptr;
}

