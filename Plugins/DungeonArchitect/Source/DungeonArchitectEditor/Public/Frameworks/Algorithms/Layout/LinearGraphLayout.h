//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Algorithms/Layout/GraphLayout.h"

class UEdGraphNode;

struct FLinearGraphLayoutConfig {
    float InterDistanceX = 100;
    float InterDistanceY = 100;
    bool bHorizontalLayout = true;
};

typedef TSharedPtr<struct FLinearLayoutNode> FLinearLayoutNodePtr;

struct FLinearLayoutNode {
    FGuid NodeId;
    int32 LocationX = 0;
    int32 LocationY = 0;
    TArray<FLinearLayoutNodePtr> OutgoingNodes;
    TArray<FLinearLayoutNodePtr> IncomingNodes;
};


class DUNGEONARCHITECTEDITOR_API FLinearGraphLayout : public IGraphLayout {
public:
    FLinearGraphLayout(const FLinearGraphLayoutConfig& InConfig) : Config(InConfig) {
    }

    virtual ~FLinearGraphLayout() {
    }

    virtual void PerformLayout(UEdGraph* Graph) override;


private:
    FLinearGraphLayoutConfig Config;

};

