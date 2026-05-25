//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraphNode.h"
#include "GridFlowLayoutEdGraphNode.generated.h"

class UFlowAbstractNode;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UGridFlowLayoutEdGraphNode : public UFlowLayoutEdGraphNode {
    GENERATED_BODY()

public:
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
};

