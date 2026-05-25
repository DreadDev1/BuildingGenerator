//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

class FDAG_InventoryDragDropMode_DragDrop : public IDAG_InventoryDragMode {
public:
	virtual FReply HandleSlotMouseButtonDown(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void HandleSlotDragDetected(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool HandleSlotDrop(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};

