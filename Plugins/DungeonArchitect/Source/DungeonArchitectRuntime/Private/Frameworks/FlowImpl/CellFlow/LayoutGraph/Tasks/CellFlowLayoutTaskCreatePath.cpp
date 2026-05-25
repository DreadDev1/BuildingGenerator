//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreatePath.h"

#include "Builders/CellFlow/CellFlowConfig.h"
#include "Frameworks/Flow/Utils/DungeonFlowTagVolume.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"

void UCellFlowLayoutTaskCreatePath::ExtendPathNodes(FFlowExecNodeStatePtr InNodeState, const FFlowAGGrowthState& InPathState, const FRandomStream& InRandom, const FFlowAbstractGraphQuery& InGraphQuery) const {
	Super::ExtendPathNodes(InNodeState, InPathState, InRandom, InGraphQuery);

	UDAFlowCellGraph* CellGraph = InNodeState->GetState<UDAFlowCellGraph>(UDAFlowCellGraph::StateTypeID);
	
	TSet<FGuid> LayoutNodeIds;
	for (const FFlowAGGrowthState_PathItem& PathItem : InPathState.Path) {
		LayoutNodeIds.Add(PathItem.NodeId);
	}
	
	DA::FCellFlowLib::AssignGroupHeights(CellGraph, LayoutNodeIds, CeilingHeightMin, CeilingHeightMax, InGraphQuery, InRandom);
}

UFlowLayoutNodeCreationConstraint* UCellFlowLayoutTaskCreatePath::GetNodeCreationConstraintLogic(ADungeon* InDungeon) const {
	if (IsValid(InDungeon)) {
		if (UCellFlowConfig* CellFlowConfig = Cast<UCellFlowConfig>(InDungeon->Config)) {
			FTransform CoordToWorld = FTransform(FQuat::Identity, FVector::ZeroVector, CellFlowConfig->GridSize)
				* (InDungeon ? InDungeon->GetActorTransform() : FTransform::Identity);
		
			UFlowLayoutNodeCreationConstraint* NodeCreationTagConstraint = NewObject<UFlowLayoutNodeCreationConstraint>();
			NodeCreationTagConstraint->CoordToWorld = CoordToWorld;
			NodeCreationTagConstraint->PathName = FName(PathName);
			NodeCreationTagConstraint->StartNodePathName = FName("");
			NodeCreationTagConstraint->EndNodePathName = FName("");
			ADungeonFlowTagVolume::FindInWorld(InDungeon, NodeCreationTagConstraint->ConstraintVolumes);
		
			return NodeCreationTagConstraint;
		}
	}

	return nullptr;
}

