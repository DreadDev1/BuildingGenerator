//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DAGInventorySlotViewWidget.generated.h"

class UImage;
class UTextBlock;
class UDAG_ItemDefinition;


UCLASS()
class DUNGEONARCHITECTGAMEPLAY_API UDAG_InventorySlotViewWidget : public UUserWidget {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), Category="Inventory")
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), Category="Inventory")
	TObjectPtr<UTextBlock> ItemCountText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget, OptionalWidget = true), Category="Inventory")
	TObjectPtr<UImage> ResourceDropIcon;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateView(UDAG_ItemDefinition* ItemDefinition, int32 Count);
};

