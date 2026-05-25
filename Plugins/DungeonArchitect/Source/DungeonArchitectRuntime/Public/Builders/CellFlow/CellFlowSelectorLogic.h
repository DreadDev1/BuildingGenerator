//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Rules/Selector/DungeonSelectorLogic.h"
#include "CellFlowSelectorLogic.generated.h"

class UCellFlowModel;
class UCellFlowConfig;
class UCellFlowBuilder;
class UCellFlowQuery;

UCLASS(Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UCellFlowSelectorLogic : public UDungeonSelectorLogic {
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
	bool SelectNode(UCellFlowModel* Model, UCellFlowConfig* Config, UCellFlowBuilder* Builder, UCellFlowQuery* Query,
					const FRandomStream& RandomStream, const FTransform& MarkerTransform);

	virtual bool SelectNode_Implementation(UCellFlowModel* Model, UCellFlowConfig* Config, UCellFlowBuilder* Builder,
	                                       UCellFlowQuery* Query, const FRandomStream& RandomStream, const FTransform& MarkerTransform);
	
};

