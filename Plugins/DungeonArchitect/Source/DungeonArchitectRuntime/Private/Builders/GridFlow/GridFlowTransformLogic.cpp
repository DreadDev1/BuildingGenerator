//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/GridFlow/GridFlowTransformLogic.h"

#include "Builders/GridFlow/GridFlowQuery.h"

void UGridFlowTransformLogic::GetNodeOffset_Implementation(UGridFlowModel* Model, UGridFlowConfig* Config,
                                                           UGridFlowQuery* Query, const FRandomStream& RandomStream,
                                                           int32 GridX, int32 GridY, const FTransform& MarkerTransform, FTransform& Offset) {
    Offset = FTransform::Identity;
}

