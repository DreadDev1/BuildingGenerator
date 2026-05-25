//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

#include "Blueprint/UserWidget.h"
#include "DAGInventoryGridWidget.generated.h"

class IDAG_InventoryDragMode;
class UWrapBox;
class UDAG_InventorySlotWidget;
class UDAG_InventoryComponent;
class UDAG_InventoryDragDropManager;

UCLASS()
class DUNGEONARCHITECTGAMEPLAY_API UDAG_InventoryGridWidget : public UUserWidget {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeGrid(UDAG_InventoryComponent* Inventory);

	IDAG_InventoryDragMode* GetDragMode() const;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	FORCEINLINE UDAG_InventoryDragDropManager* GetDragDropManager() const { return DragDropManager; }
	
private:
	UFUNCTION()
	void OnInventorySlotChanged(int32 InSlotIndex, const FDAG_InventorySlot& InSlot);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TObjectPtr<UWrapBox> InventoryGrid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TSubclassOf<UDAG_InventorySlotWidget> SlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bEnableDragSupport = true;
	
	UPROPERTY()
	TObjectPtr<UDAG_InventoryComponent> InventoryComponent;

private:
	UPROPERTY()
	TObjectPtr<UDAG_InventoryDragDropManager> DragDropManager = nullptr;;
};


