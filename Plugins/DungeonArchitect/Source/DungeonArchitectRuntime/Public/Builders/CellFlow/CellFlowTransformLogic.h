//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/DungeonTransformLogic.h"
#include "CellFlowTransformLogic.generated.h"

class UCellFlowModel;
class UCellFlowConfig;
class UCellFlowBuilder;
class UCellFlowQuery;

/**
*
*/
UCLASS(Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UCellFlowTransformLogic : public UDungeonTransformLogic {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
	void GetNodeOffset(UCellFlowModel* Model, UCellFlowConfig* Config, UCellFlowQuery* Query,
					   const FRandomStream& RandomStream, const FTransform& MarkerTransform, FTransform& Offset);
	virtual void GetNodeOffset_Implementation(UCellFlowModel* Model, UCellFlowConfig* Config, UCellFlowQuery* Query,
											  const FRandomStream& RandomStream, const FTransform& MarkerTransform, FTransform& Offset);

};

