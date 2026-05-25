//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Algorithms/Layout/GraphLayout.h"
#include "Frameworks/Algorithms/Layout/LayeredLayoutLib.h"

class DUNGEONARCHITECTEDITOR_API FLayeredGraphLayout : public IGraphLayout {
public:
    FLayeredGraphLayout(const FLayeredGraphLayoutConfig& InConfig)
        : Config(InConfig)
    {
    }

    virtual void PerformLayout(UEdGraph* Graph) override;


private:
    FLayeredGraphLayoutConfig Config;

};

