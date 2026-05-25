//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/DAGInventory.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

class FDAG_InventoryDragDropMode_PickDrop : public IDAG_InventoryDragMode {
public:
	virtual FReply HandleSlotMouseButtonDown(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply HandleSlotMouseButtonUp(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply HandleSlotMouseButtonDoubleClick(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	virtual void HandleSlotDragDetected(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool HandleSlotDrop(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void HandleSlotMouseEnter(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void HandleSlotMouseLeave(const FDAG_InventorySlotDragEvent& InEvent, const FPointerEvent& InMouseEvent) override;

	virtual FReply HandleBackgroundMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
	virtual void HandleBackgroundMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
	virtual void HandleBackgroundMouseLeave(const FPointerEvent& MouseEvent) override;
	
	virtual void HandleWindowClosed() override;
	virtual void Tick(float DeltaTime) override;

	
private:
	FReply DropCursorItems(UDAG_InventorySlotWidget* SlotWidget, int Count, bool bAllowSwapping);
	FReply PickupAllItemsToCursor(UDAG_InventorySlotWidget* SlotWidget);
	FReply PickupHalfItemsToCursor(UDAG_InventorySlotWidget* SlotWidget);
	void UpdateSlotDragState(UDAG_InventorySlotWidget* HoveredWidget) const;
	
	void UpdateCursorWidget() const;
	static UDAG_PlayerInventoryControllerComponent* GetPlayerInventoryController(const UWidget* InWidget);

	bool GetCachedCursorSlot(FDAG_InventorySlot& OutCursorSlot) const;

private:
	bool bPickedUpOnLastMouseDown{};
	int32 LastPickupSlotIndex = INDEX_NONE;
	TWeakObjectPtr<UDAG_InventoryComponent> LastPickupInventory;
};

