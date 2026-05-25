//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/SimpleCity/SimpleCityModel.h"
#include "Frameworks/ThemeEngine/Rules/Selector/DungeonSelectorLogic.h"
#include "SimpleCitySelectorLogic.generated.h"

/**
*
*/
UCLASS(Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API USimpleCitySelectorLogic : public UDungeonSelectorLogic {
    GENERATED_BODY()

public:

    /** called when something enters the sphere component */
    UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
    bool SelectNode(USimpleCityModel* Model);
    virtual bool SelectNode_Implementation(USimpleCityModel* Model);


};

