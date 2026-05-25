//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Inventory/UI/DragDrop/DAGInventoryDragDropManager.h"

#include "Inventory/DAGInventory.h"
#include "Inventory/UI/DAGInventoryGridWidget.h"
#include "Inventory/UI/DAGInventorySlotWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogDragDropManager, Log, All);

void UDAG_InventoryEventHandler::OnItemDroppedInWorld_Implementation(UObject* WorldContextObject, UDAG_ItemDefinition* ItemDefinition, int32 Count) {
	
}

void UDAG_InventoryDragDropManager::SynchronizeProperties() {
	Super::SynchronizeProperties();

	DragMode = FDAG_InventoryDragModeFactory::Create(this, DragModeType);

	// Recreate the cursor overlay if necessary
	const bool bRecreateCursor = !CursorWidget || CursorWidget->GetClass() != CursorWidgetClass;
	if (bRecreateCursor) {
		RebuildCursorWidget();
	}

	MyOverlay->SetOnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonDown));
	MyOverlay->SetOnMouseButtonUp(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonUp));
	MyOverlay->SetOnMouseMove(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseMove));
	MyOverlay->SetOnMouseDoubleClick(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseDoubleClick));
	MyOverlay->SetOnMouseEnter(BIND_UOBJECT_DELEGATE(FNoReplyPointerEventHandler, HandleMouseEnter));
	MyOverlay->SetOnMouseLeave(BIND_UOBJECT_DELEGATE(FSimpleNoReplyPointerEventHandler, HandleMouseLeave));
	SetVisibility(ESlateVisibility::Visible);
}

void UDAG_InventoryDragDropManager::SetCursorVisibility(ESlateVisibility InVisibility) const {
	if (CursorWidget && CursorWidget->GetVisibility() != InVisibility) {
		CursorWidget->SetVisibility(InVisibility);
	}
}

void UDAG_InventoryDragDropManager::SetCursorVisuals(const FDAG_InventorySlot& InCursorSlot) const {
	if (CursorWidget) {
		CursorWidget->UpdateView(InCursorSlot.ItemDefinition, InCursorSlot.StackCount);
	}
}

void UDAG_InventoryDragDropManager::NotifyScreenClosed() {
	if (DragMode.IsValid()) {
		DragMode->HandleWindowClosed();
	}
}

void UDAG_InventoryDragDropManager::OnInventorySlotMouseEnter(UDAG_InventorySlotWidget* InSlot) {
	ActiveHoveredSlot = InSlot;
}

void UDAG_InventoryDragDropManager::OnInventorySlotMouseLeave(const UDAG_InventorySlotWidget* InSlot) {
	if (ActiveHoveredSlot == InSlot) {
		ActiveHoveredSlot = nullptr;
	}
}

void UDAG_InventoryDragDropManager::OnInventoryGridMouseEnter(UDAG_InventoryGridWidget* InGrid) {
	ActiveHoveredInventoryGrid = InGrid;
}

void UDAG_InventoryDragDropManager::OnInventoryGridMouseLeave(const UDAG_InventoryGridWidget* InGrid) {
	if (ActiveHoveredInventoryGrid == InGrid) {
		ActiveHoveredInventoryGrid = nullptr;
	}
}

bool UDAG_InventoryDragDropManager::IsCursorOutsideInventoryArea() const {
	return !ActiveHoveredInventoryGrid.IsValid();
}

void UDAG_InventoryDragDropManager::RebuildCursorWidget() {
	// Destroy the existing widget
	if (CursorWidget) {
		CursorWidget->RemoveFromParent();
		RemoveChild(CursorWidget);
		CursorWidget = nullptr;
	}
	
	if (CursorWidgetClass) {
		CursorWidget = CreateWidget<UDAG_InventorySlotViewWidget>(this, CursorWidgetClass);
		CursorWidgetSlot = AddChildToOverlay(CursorWidget);
		CursorWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

FReply UDAG_InventoryDragDropManager::HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const {
	if (DragMode.IsValid() && IsCursorOutsideInventoryArea()) {
		return DragMode->HandleBackgroundMouseButtonDown(Geometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply UDAG_InventoryDragDropManager::HandleMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const {
	if (DragMode.IsValid() && IsCursorOutsideInventoryArea()) {
		return DragMode->HandleBackgroundMouseButtonUp(Geometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply UDAG_InventoryDragDropManager::HandleMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const {
	if (DragMode.IsValid()) {
		return DragMode->HandleBackgroundMouseMove(Geometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply UDAG_InventoryDragDropManager::HandleMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const {
	if (DragMode.IsValid()) {
		return DragMode->HandleBackgroundMouseDoubleClick(Geometry, MouseEvent);
	}
	return FReply::Unhandled();
}

void UDAG_InventoryDragDropManager::HandleMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const {
	if (DragMode.IsValid()) {
		DragMode->HandleBackgroundMouseEnter(Geometry, MouseEvent);
	}
}

void UDAG_InventoryDragDropManager::HandleMouseLeave(const FPointerEvent& MouseEvent) const {
	if (DragMode.IsValid()) {
		DragMode->HandleBackgroundMouseLeave(MouseEvent);
	}
}

TSharedRef<SWidget> UDAG_InventoryDragDropManager::RebuildWidget() {
	RebuildCursorWidget();

	return Super::RebuildWidget();
}

void UDAG_InventoryDragDropManager::Tick(float DeltaTime) {
	if (DragMode.IsValid()) {
		DragMode->Tick(DeltaTime);
	}

	// Update the cursor's resource drop icon visibility
	if (CursorWidget && CursorWidget->ResourceDropIcon) {
		CursorWidget->ResourceDropIcon->SetVisibility(
			IsCursorOutsideInventoryArea()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Hidden);
	}
	
	UpdateCursorPosition();
}

bool UDAG_InventoryDragDropManager::IsTickable() const {
	return IsVisible();
}

void UDAG_InventoryDragDropManager::UpdateCursorPosition() const {
	if (CursorWidgetSlot) {
		if (APlayerController* PlayerController = GetOwningPlayer()) {
			const FVector2D CursorPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(PlayerController);
			const FVector2D CursorWidgetSize = CursorWidget->GetDesiredSize();
			const FVector2D TargetPosition = CursorPosition - CursorWidgetSize * 0.5;
			CursorWidgetSlot->SetPadding(FMargin(TargetPosition.X, TargetPosition.Y, 0, 0));
		}
	}
}


UWorld* UDAG_InventoryDragDropManager::GetTickableGameObjectWorld() const {
	return GetWorld();
}

TStatId UDAG_InventoryDragDropManager::GetStatId() const {
	RETURN_QUICK_DECLARE_CYCLE_STAT(FDAG_InventoryDragDropManager, STATGROUP_Tickables);
}

