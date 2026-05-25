//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "Layout/Geometry.h"
#include "DAGInventoryDragMode.generated.h"

class UDragDropOperation;
class UDAG_InventoryDragDropManager;
class UDAG_InventoryComponent;
class UDAG_PlayerInventoryControllerComponent;
class UDAG_InventorySlotWidget;
class UWidget;

struct FDAG_InventorySlotDragEvent {
	TObjectPtr<UDAG_InventorySlotWidget> SlotWidget;
	TWeakObjectPtr<UDAG_PlayerInventoryControllerComponent> InventoryController;
	static FDAG_InventorySlotDragEvent Create(const TObjectPtr<UDAG_InventorySlotWidget>& InSlotWidget);
};

class IDAG_InventoryDragMode {
public:
	virtual ~IDAG_InventoryDragMode() = default;
	
	virtual FReply HandleSlotMouseButtonDown(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual FReply HandleSlotMouseButtonUp(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual FReply HandleSlotMouseButtonDoubleClick(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual void HandleSlotMouseEnter(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {};
	virtual void HandleSlotMouseLeave(const FDAG_InventorySlotDragEvent& InEvent, const FPointerEvent& InMouseEvent) {};
	virtual void HandleSlotDragDetected(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) {}
	virtual bool HandleSlotDrop(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) { return false; }
	virtual void HandleSlotDragCancelled(const FDAG_InventorySlotDragEvent& InEvent, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {}
	virtual void HandleSlotDragEnter(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {}
	virtual void HandleSlotDragLeave(const FDAG_InventorySlotDragEvent& InEvent, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) {}
	virtual bool HandleSlotDragOver(const FDAG_InventorySlotDragEvent& InEvent, const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) { return false; }

	virtual FReply HandleBackgroundMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	virtual FReply HandleBackgroundMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	virtual FReply HandleBackgroundMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	virtual FReply HandleBackgroundMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	virtual void HandleBackgroundMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) {}
	virtual void HandleBackgroundMouseLeave(const FPointerEvent& MouseEvent) {}
	
	virtual void Tick(float DeltaTime) {}
	virtual void HandleWindowClosed() {}
	
	void SetDragDropManager(UDAG_InventoryDragDropManager* InManager);

protected:
	TWeakObjectPtr<UDAG_InventoryDragDropManager> DragDropManager;
};

UENUM(BlueprintType)
enum class EDAG_InventoryDragMode : uint8
{
	/** Drag the resources and drop on to another slot, like in factorio */
	DragAndDrop,

	/** Pick the resources and place on multiple slots selectively, like in minecraft */
	PickAndDrop,

	/** Does not support drag drop */
	Ignored
};

class FDAG_InventoryDragModeFactory {
public:
	static TUniquePtr<IDAG_InventoryDragMode> Create(UDAG_InventoryDragDropManager* InDragDropManager, EDAG_InventoryDragMode InMode);
};

