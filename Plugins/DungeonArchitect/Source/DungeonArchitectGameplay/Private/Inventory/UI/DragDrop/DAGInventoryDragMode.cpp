//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

#include "Inventory/Components/DAGPlayerInventoryControllerComponent.h"
#include "Inventory/UI/DAGInventorySlotWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragDropManager.h"
#include "Inventory/UI/DragDrop/Modes/DAGInventoryDragMode_DragDrop.h"
#include "Inventory/UI/DragDrop/Modes/DAGInventoryDragMode_PickDrop.h"

class FDAG_InventoryDragDropMode_NoOp : public IDAG_InventoryDragMode {
public:
};

FDAG_InventorySlotDragEvent FDAG_InventorySlotDragEvent::Create(const TObjectPtr<UDAG_InventorySlotWidget>& InSlotWidget) {
	FDAG_InventorySlotDragEvent Event;
	Event.SlotWidget = InSlotWidget;
	if (InSlotWidget) {
		if (const APlayerController* PlayerController = InSlotWidget->GetOwningPlayer()) {
			Event.InventoryController = PlayerController->FindComponentByClass<UDAG_PlayerInventoryControllerComponent>();
		}
	}
	return Event;
}

FReply IDAG_InventoryDragMode::HandleSlotMouseButtonDown(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) { return FReply::Unhandled(); }
FReply IDAG_InventoryDragMode::HandleSlotMouseButtonUp(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) { return FReply::Unhandled(); }
FReply IDAG_InventoryDragMode::HandleSlotMouseButtonDoubleClick(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) { return FReply::Unhandled(); }
FReply IDAG_InventoryDragMode::HandleBackgroundMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
FReply IDAG_InventoryDragMode::HandleBackgroundMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
FReply IDAG_InventoryDragMode::HandleBackgroundMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
FReply IDAG_InventoryDragMode::HandleBackgroundMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }

void IDAG_InventoryDragMode::SetDragDropManager(UDAG_InventoryDragDropManager* InManager) {
	DragDropManager = InManager;
}

TUniquePtr<IDAG_InventoryDragMode> FDAG_InventoryDragModeFactory::Create(UDAG_InventoryDragDropManager* InDragDropManager, EDAG_InventoryDragMode InMode) {
	TUniquePtr<IDAG_InventoryDragMode> Manager;
	switch(InMode) {
	case EDAG_InventoryDragMode::DragAndDrop:
		Manager = MakeUnique<FDAG_InventoryDragDropMode_DragDrop>();
		break;

	case EDAG_InventoryDragMode::PickAndDrop:
		Manager = MakeUnique<FDAG_InventoryDragDropMode_PickDrop>();
		break;

	case EDAG_InventoryDragMode::Ignored:
	default:
		Manager = MakeUnique<FDAG_InventoryDragDropMode_NoOp>();
		break;
	}
	Manager->SetDragDropManager(InDragDropManager);
	return Manager;
}

