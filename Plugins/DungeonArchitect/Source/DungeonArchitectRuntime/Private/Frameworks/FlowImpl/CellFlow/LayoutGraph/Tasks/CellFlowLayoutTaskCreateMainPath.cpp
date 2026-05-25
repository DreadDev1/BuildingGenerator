//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateMainPath.h"

#include "Builders/CellFlow/CellFlowConfig.h"
#include "Frameworks/Flow/Utils/DungeonFlowTagVolume.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"

void UCellFlowLayoutTaskCreateMainPath::ExtendPathNodes(FFlowExecNodeStatePtr InNodeState, const FFlowAGGrowthState& InPathState, const FRandomStream& InRandom, const FFlowAbstractGraphQuery& InGraphQuery) const {
	Super::ExtendPathNodes(InNodeState, InPathState, InRandom, InGraphQuery);
	
	UDAFlowCellGraph* CellGraph = InNodeState->GetState<UDAFlowCellGraph>(UDAFlowCellGraph::StateTypeID);
	
	TSet<FGuid> LayoutNodeIds;
	for (const FFlowAGGrowthState_PathItem& PathItem : InPathState.Path) {
		LayoutNodeIds.Add(PathItem.NodeId);
	}
	
	DA::FCellFlowLib::AssignGroupHeights(CellGraph, LayoutNodeIds, CeilingHeightMin, CeilingHeightMax, InGraphQuery, InRandom);
}


UFlowLayoutNodeCreationConstraint* UCellFlowLayoutTaskCreateMainPath::GetNodeCreationConstraintLogic(ADungeon* InDungeon) const {
	if (InDungeon) {
		if (UCellFlowConfig* CellFlowConfig = Cast<UCellFlowConfig>(InDungeon->Config)) {
			FTransform CoordToWorld = FTransform(FQuat::Identity, FVector::ZeroVector, CellFlowConfig->GridSize)
				* (InDungeon ? InDungeon->GetActorTransform() : FTransform::Identity);
		
			UFlowLayoutNodeCreationConstraint* NodeCreationTagConstraint = NewObject<UFlowLayoutNodeCreationConstraint>();
			NodeCreationTagConstraint->CoordToWorld = CoordToWorld;
			NodeCreationTagConstraint->PathName = FName(PathName);
			NodeCreationTagConstraint->StartNodePathName = FName(StartNodePathName);
			NodeCreationTagConstraint->EndNodePathName = FName(GoalNodePathName);
			ADungeonFlowTagVolume::FindInWorld(InDungeon, NodeCreationTagConstraint->ConstraintVolumes);
			
			return NodeCreationTagConstraint;
		}
	}

	return nullptr;
}

