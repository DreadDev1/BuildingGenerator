//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

#include "Blueprint/UserWidget.h"
#include "DAGInventorySlotWidget.generated.h"

class IDAG_InventoryDragMode;
class UDAG_InventoryGridWidget;
class UDAG_InventorySlotViewWidget;
class UDAG_InventoryComponent;
class UDAG_InventoryDragDropManager;
enum class EDAInventorySlotViewCursorState : uint8;

UCLASS()
class DUNGEONARCHITECTGAMEPLAY_API UDAG_InventorySlotWidget : public UUserWidget {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), Category="Inventory")
	TObjectPtr<UDAG_InventorySlotViewWidget> SlotView;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	TObjectPtr<class UDAG_InventoryGridWidget> ParentGridWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	bool bIsDraggedOver = false;
	
public:
	UDAG_InventoryComponent* GetInventoryComponent() const;
	UDAG_InventoryDragDropManager* GetDragDropManager() const;
	IDAG_InventoryDragMode* GetDragMode() const;
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

};

