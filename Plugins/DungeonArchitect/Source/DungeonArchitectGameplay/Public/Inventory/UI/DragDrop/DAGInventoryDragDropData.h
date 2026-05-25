//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "DAGInventoryDragDropData.generated.h"

class UDAG_InventoryComponent;

UCLASS()
class DUNGEONARCHITECTGAMEPLAY_API UDAG_InventoryDragDropData : public UDragDropOperation {
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	TObjectPtr<UDAG_InventoryComponent> InventoryComponent;

	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	int32 SlotIndex;
};

