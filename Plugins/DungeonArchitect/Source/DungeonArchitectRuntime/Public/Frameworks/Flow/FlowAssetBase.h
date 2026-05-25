//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "FlowAssetBase.generated.h"

class UEdGraph;
class UFlowExecScript;

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UFlowAssetBase : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UFlowExecScript> ExecScript;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UEdGraph> ExecEdGraph;
#endif // WITH_EDITORONLY_DATA

    static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
};

