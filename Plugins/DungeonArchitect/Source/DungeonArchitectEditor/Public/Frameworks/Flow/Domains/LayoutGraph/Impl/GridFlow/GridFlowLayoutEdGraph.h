//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraph.h"
#include "GridFlowLayoutEdGraph.generated.h"

class UGridFlowAbstractGraph;
UCLASS()
class DUNGEONARCHITECTEDITOR_API UGridFlowLayoutEdGraph : public UFlowLayoutEdGraph {
    GENERATED_BODY()

public:
    UGridFlowAbstractGraph* GetGridScriptGraph() const;
};


