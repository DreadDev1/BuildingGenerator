//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Inventory/UI/DAGInventorySlotViewWidget.h"
#include "Inventory/UI/DragDrop/DAGInventoryDragMode.h"

#include "Components/Overlay.h"
#include "DAGInventoryDragDropManager.generated.h"

class UDAG_InventoryGridWidget;
struct FDAG_InventorySlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDAG_OnInventoryItemDropped, UDAG_ItemDefinition*, Item, int32, Count);


UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, HideDropDown)
class UDAG_InventoryEventHandler : public UObject {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Inventory", Meta=(WorldContext="WorldContextObject"))
	void OnItemDroppedInWorld(UObject* WorldContextObject, UDAG_ItemDefinition* ItemDefinition, int32 Count);
};

UCLASS()
class DUNGEONARCHITECTGAMEPLAY_API UDAG_InventoryDragDropManager
	: public UOverlay
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	FORCEINLINE IDAG_InventoryDragMode* GetDragMode() const { return DragMode.Get(); }
	virtual void SynchronizeProperties() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Begin FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	// End FTickableGameObject interface

	void UpdateCursorPosition() const;
	void SetCursorVisibility(ESlateVisibility InVisibility) const;
	void SetCursorVisuals(const FDAG_InventorySlot& InCursorSlot) const;
	//void DropItemsToWorld(UDAItemDefinition* ItemDefinition, int32 Count) const;

	/** Call this function when your screen closes.   This will drop any items held in the hand back to the inventory or the world */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void NotifyScreenClosed();

	void OnInventorySlotMouseEnter(UDAG_InventorySlotWidget* InSlot);
	void OnInventorySlotMouseLeave(const UDAG_InventorySlotWidget* InSlot);
	void OnInventoryGridMouseEnter(UDAG_InventoryGridWidget* InGrid);
	void OnInventoryGridMouseLeave(const UDAG_InventoryGridWidget* InGrid);

	/** Is the cursor outside the inventory grid? clicking on this will request the resource be dropped on the ground */
	bool IsCursorOutsideInventoryArea() const;
	
private:
	void RebuildCursorWidget();
	
	FReply HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const;
	FReply HandleMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const;
	FReply HandleMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const;
	FReply HandleMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const;
	void HandleMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const;
	void HandleMouseLeave(const FPointerEvent& MouseEvent) const;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	EDAG_InventoryDragMode DragModeType = EDAG_InventoryDragMode::DragAndDrop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TSubclassOf<UDAG_InventorySlotViewWidget> CursorWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Selection Logic", meta=(EditCondition="bUseSelectionLogic"))
	TArray<TObjectPtr<UDAG_InventoryEventHandler>> EventListeners;

private:
	UPROPERTY()
	TObjectPtr<UDAG_InventorySlotViewWidget> CursorWidget = nullptr;
	
	TWeakObjectPtr<UDAG_InventorySlotWidget> ActiveHoveredSlot;
	TWeakObjectPtr<UDAG_InventoryGridWidget> ActiveHoveredInventoryGrid;
	
	UPROPERTY()
	TObjectPtr<UOverlaySlot> CursorWidgetSlot = nullptr;;
	
	TUniquePtr<IDAG_InventoryDragMode> DragMode;
};

