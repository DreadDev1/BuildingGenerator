//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/CellFlow/CellFlowTransformLogic.h"


void UCellFlowTransformLogic::GetNodeOffset_Implementation(UCellFlowModel* Model, UCellFlowConfig* Config, UCellFlowQuery* Query,
														  const FRandomStream& RandomStream, const FTransform& MarkerTransform, FTransform& Offset) {
	Offset = FTransform::Identity;
}

