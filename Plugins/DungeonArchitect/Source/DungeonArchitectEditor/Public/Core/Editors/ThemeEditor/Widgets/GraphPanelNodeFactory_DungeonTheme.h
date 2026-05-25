//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class DUNGEONARCHITECTEDITOR_API FGraphPanelNodeFactory_DungeonTheme : public FGraphPanelNodeFactory {
public:
    FGraphPanelNodeFactory_DungeonTheme();

private:
    virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};

