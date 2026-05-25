//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraph.h"
#include "Frameworks/Snap/Lib/Serialization/SnapGraphSerializer.h"
#include "Frameworks/Snap/Lib/Serialization/SnapModuleInstanceSerialization.h"

class FSGFSnapConnectionSerializePolicy {
public:
    explicit FSGFSnapConnectionSerializePolicy(USnapGridFlowAbstractGraph* InLayoutGraph) : LayoutGraphPtr(InLayoutGraph) {}
    void GatherConnectionInfo(const FGuid& ModuleA, const FGuid& ModuleB, ESnapConnectionDoorType& OutConnectionType, FString& OutCustomMarkerName) const;

private:
    TWeakObjectPtr<USnapGridFlowAbstractGraph> LayoutGraphPtr;
};


typedef TSnapGraphSerializer<FSnapModuleInstanceSerializedData, FSGFSnapConnectionSerializePolicy> FSnapGridFlowGraphSerializer;

