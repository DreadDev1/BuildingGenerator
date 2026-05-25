//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UEdGraph;

class DUNGEONARCHITECTEDITOR_API IGraphLayout {
public:
    virtual ~IGraphLayout() = default;
    virtual void PerformLayout(UEdGraph* Graph) = 0;
};

