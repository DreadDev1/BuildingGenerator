//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DragDrop/Modes/DAGInventoryDragMode_PickDrop.h"

#include "Inventory/Components/DAGInventoryComponent.h"
#include "Inventory/Components/DAGPlayerInventoryControllerComponent.h"
#include "Inventory/UI/DAGInventorySlotWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragDropManager.h"
#include "Item/DAGItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogPickDrop, Log, All);

FReply FDAG_InventoryDragDropMode_PickDrop::HandleSlotMouseButtonDown(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	// Mouse down is handled only if the cursor is empty. While placing items, we do that on the mouse up event
	FDAG_InventorySlot CachedCursorSlot;
	if (GetCachedCursorSlot(CachedCursorSlot)) {
		if (CachedCursorSlot.IsEmpty()) {
			LastPickupSlotIndex = InEvent.SlotWidget->SlotIndex;
			LastPickupInventory = InEvent.SlotWidget->GetInventoryComponent();
			if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) {
				// Pickup the full stack
				return PickupAllItemsToCursor(InEvent.SlotWidget);
			}
			else if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton)) {
				// Pick up half the stack
				return PickupHalfItemsToCursor(InEvent.SlotWidget);
			}
		}
	}
	return FReply::Unhandled();
}

FReply FDAG_InventoryDragDropMode_PickDrop::HandleSlotMouseButtonUp(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (bPickedUpOnLastMouseDown) {
		bPickedUpOnLastMouseDown = false;
		// we picked an item up in the last mouse down. check if we released the mouse on the same slot (i.e. clicked on a item).
		// If so, we don't want to place it right back
		if (LastPickupSlotIndex == InEvent.SlotWidget->SlotIndex && LastPickupInventory == InEvent.SlotWidget->GetInventoryComponent()) {
			// Treat this as a click and don't place items back
			return FReply::Unhandled();
		}
	}
	// Mouse up is handled only if the cursor is not empty. We place items on mouse up event
	FDAG_InventorySlot CachedCursorSlot;
	if (GetCachedCursorSlot(CachedCursorSlot)) {
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
			return DropCursorItems(InEvent.SlotWidget, CachedCursorSlot.StackCount, true);
		}
		else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) {
			// Attempt to drop one item, do not swap if there's an item of another type
			return DropCursorItems(InEvent.SlotWidget, 1, false);
		}
	}
	return FReply::Unhandled();
}

FReply FDAG_InventoryDragDropMode_PickDrop::HandleSlotMouseButtonDoubleClick(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) {
		if (UDAG_InventorySlotWidget* SlotWidget = InEvent.SlotWidget) {
			if (UDAG_InventoryComponent* Inventory = SlotWidget->GetInventoryComponent()) {
				bPickedUpOnLastMouseDown = true;

				FDAG_InventorySlot CachedCursorSlot;
				if (GetCachedCursorSlot(CachedCursorSlot)) {
					FDAG_InventorySlot ExistingSlot;
					// Pick up max stack of items from the entire inventory, starting with the given slot
					if (Inventory->GetItemAt(SlotWidget->SlotIndex, ExistingSlot)) {
						if (CachedCursorSlot.IsEmpty() && ExistingSlot.IsEmpty()) {
							return FReply::Unhandled();
						}
						if (!CachedCursorSlot.IsEmpty() && !ExistingSlot.IsEmpty() && CachedCursorSlot.ItemDefinition != ExistingSlot.ItemDefinition) {
							// cursor and target slot contains different items
							return FReply::Unhandled();
						}

						UDAG_ItemDefinition* ItemDef = CachedCursorSlot.ItemDefinition ? CachedCursorSlot.ItemDefinition : ExistingSlot.ItemDefinition;
						if (!ItemDef) {
							return FReply::Unhandled();
						}
					}

					if (UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = GetPlayerInventoryController(SlotWidget)) {
						PlayerInventoryController->MoveInventoryToCursorFullStack(Inventory, SlotWidget->SlotIndex);
						bPickedUpOnLastMouseDown = true;
						return FReply::Handled();
					}
				}
			}
		}
	}
	return FReply::Unhandled();
}

void FDAG_InventoryDragDropMode_PickDrop::HandleSlotDragDetected(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) {
	
}

bool FDAG_InventoryDragDropMode_PickDrop::HandleSlotDrop(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {
	return true;
}

void FDAG_InventoryDragDropMode_PickDrop::HandleSlotMouseEnter(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	UpdateSlotDragState(InEvent.SlotWidget);
}

void FDAG_InventoryDragDropMode_PickDrop::HandleSlotMouseLeave(const FDAG_InventorySlotDragEvent& InEvent, const FPointerEvent& InMouseEvent) {
	if (InEvent.SlotWidget) {
		InEvent.SlotWidget->bIsDraggedOver = false;
	}
}

FReply FDAG_InventoryDragDropMode_PickDrop::HandleBackgroundMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) {
	if (DragDropManager.IsValid()) {
		if (APlayerController* PlayerController = DragDropManager->GetOwningPlayer()) {
			if (UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = PlayerController->FindComponentByClass<UDAG_PlayerInventoryControllerComponent>()) {
				PlayerInventoryController->DropCursorItemsToWorld();
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

void FDAG_InventoryDragDropMode_PickDrop::HandleBackgroundMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) {
	UE_LOG(LogPickDrop, Log, TEXT("Background enter"));
}

void FDAG_InventoryDragDropMode_PickDrop::HandleBackgroundMouseLeave(const FPointerEvent& MouseEvent) {
	UE_LOG(LogPickDrop, Log, TEXT("Background leave"));
}

void FDAG_InventoryDragDropMode_PickDrop::HandleWindowClosed() {
	if (DragDropManager.IsValid()) {
		if (APlayerController* PlayerController = DragDropManager->GetOwningPlayer()) {
			if (UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = PlayerController->FindComponentByClass<UDAG_PlayerInventoryControllerComponent>()) {
				PlayerInventoryController->ReturnAllCursorItemsToOwningInventory();
			}
		}
	}
}

void FDAG_InventoryDragDropMode_PickDrop::Tick(float DeltaTime) {
	UpdateCursorWidget();
}

FReply FDAG_InventoryDragDropMode_PickDrop::DropCursorItems(UDAG_InventorySlotWidget* SlotWidget, int Count, bool bAllowSwapping) {
	if (!SlotWidget) {
		// Nothing to drop
		return FReply::Unhandled();
	}

	UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = GetPlayerInventoryController(SlotWidget);
	if (!PlayerInventoryController) {
		UE_LOG(LogPickDrop, Log, TEXT("Player Inventory Controller component is not attached to the player controller"));
		return FReply::Unhandled();
	}

	if (UDAG_InventoryComponent* Inventory = SlotWidget->GetInventoryComponent()) {
		PlayerInventoryController->MoveCursorToInventorySlot(Inventory, SlotWidget->SlotIndex, Count, bAllowSwapping);
		UpdateCursorWidget();
		UpdateSlotDragState(SlotWidget);
		return FReply::Handled();
	}
		
	return FReply::Unhandled();
}

FReply FDAG_InventoryDragDropMode_PickDrop::PickupAllItemsToCursor(UDAG_InventorySlotWidget* SlotWidget) {
	if (SlotWidget) {
		if (UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = GetPlayerInventoryController(SlotWidget)) {
			PlayerInventoryController->MoveInventorySlotToCursor(SlotWidget->GetInventoryComponent(), SlotWidget->SlotIndex);
			
			UpdateCursorWidget();
			UpdateSlotDragState(SlotWidget);
			bPickedUpOnLastMouseDown = true;
			
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

FReply FDAG_InventoryDragDropMode_PickDrop::PickupHalfItemsToCursor(UDAG_InventorySlotWidget* SlotWidget) {
	if (SlotWidget) {
		if (UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = GetPlayerInventoryController(SlotWidget)) {
			PlayerInventoryController->MoveInventoryHalfSlotToCursor(SlotWidget->GetInventoryComponent(), SlotWidget->SlotIndex);
			
			UpdateCursorWidget();
			UpdateSlotDragState(SlotWidget);
			bPickedUpOnLastMouseDown = true;
			
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

void FDAG_InventoryDragDropMode_PickDrop::UpdateSlotDragState(UDAG_InventorySlotWidget* HoveredWidget) const {
	if (HoveredWidget) {
		FDAG_InventorySlot CachedCursorSlot;
		if (GetCachedCursorSlot(CachedCursorSlot)) {
			HoveredWidget->bIsDraggedOver = !CachedCursorSlot.IsEmpty();
		}
	}
}

void FDAG_InventoryDragDropMode_PickDrop::UpdateCursorWidget() const {
	if (DragDropManager.IsValid()) {
		
		FDAG_InventorySlot CachedCursorSlot;
		if (GetCachedCursorSlot(CachedCursorSlot)) {
			DragDropManager->SetCursorVisuals(CachedCursorSlot);
			DragDropManager->SetCursorVisibility(
				CachedCursorSlot.IsEmpty()
				? ESlateVisibility::Hidden
				: ESlateVisibility::HitTestInvisible);
		}
	}
}

UDAG_PlayerInventoryControllerComponent* FDAG_InventoryDragDropMode_PickDrop::GetPlayerInventoryController(const UWidget* InWidget) {
	if (APlayerController* PlayerController = InWidget->GetOwningPlayer()) {
		return PlayerController->FindComponentByClass<UDAG_PlayerInventoryControllerComponent>();
	}
	return nullptr;
}

bool FDAG_InventoryDragDropMode_PickDrop::GetCachedCursorSlot(FDAG_InventorySlot& OutCursorSlot) const {
	if (APlayerController* PlayerController = DragDropManager->GetOwningPlayer()) {
		if (UDAG_PlayerInventoryControllerComponent* PlayerInventoryController = PlayerController->FindComponentByClass<UDAG_PlayerInventoryControllerComponent>()) {
			OutCursorSlot = PlayerInventoryController->CursorSlot;
			return true;
		}
	}
	return false;
}

