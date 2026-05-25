//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsBase.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphUtils.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"

////////////////////////////////////// UCellFlowLayoutTaskCreateCells //////////////////////////////////////

class UCellFlowConfig;

UCellFlowLayoutTaskCreateCellsBase::UCellFlowLayoutTaskCreateCellsBase() {
    InputConstraint = EFlowExecTaskInputConstraint::NoInput;
}

void UCellFlowLayoutTaskCreateCellsBase::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
    check(Input.IncomingNodeOutputs.Num() == 0);
    
    if (!Input.Random) {
        Output.ExecutionResult = EFlowTaskExecutionResult::FailHalt;
        Output.ErrorMessage = "Internal Error. Invalid State";
        return;
    }
    
    Output.State = MakeShareable(new FFlowExecNodeState);
    
    // Build the graph object
    const FRandomStream& Random = *Input.Random;

    // Generate the cell graph
    UDAFlowCellGraph* CellGraph = NewObject<UDAFlowCellGraph>();
    {
        using namespace DA;
        
        // Generate Grid Cells 
        GenerateCells(CellGraph, Input, Output.State);
		
        FCellAreaLookup AreaLookup;
        AreaLookup.Init(CellGraph);
	
        FCellGraphBuilder::CollapseEdges(CellGraph, MinGroupArea, ClusterBlobbiness, Random, AreaLookup);
        FCellGraphBuilder::AssignGroupColors(CellGraph);
        FCellGraphBuilder::AssignGroupPreviewLocations(CellGraph, AreaLookup);
        FinalizeCells(CellGraph);
    }

    // Generate the layout graph
    UCellFlowLayoutGraph* LayoutGraph = NewObject<UCellFlowLayoutGraph>();
    
    // Use a random seed that's combined with the dungeon UID, This avoids node UID collision with different dungeons on the same map 
    const FRandomStream NamingSchemeRandomStream(FFlowAbstractGraphUtils::CreateUidRandomSeed(Input.Random, Input.Dungeon));
    CreateGraphs(LayoutGraph, CellGraph, NamingSchemeRandomStream);

    // Create a new state, since this will our first node
    Output.State->SetStateObject(UFlowAbstractGraphBase::StateTypeID, LayoutGraph);
    Output.State->SetStateObject(UDAFlowCellGraph::StateTypeID, CellGraph);
    
    Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

void UCellFlowLayoutTaskCreateCellsBase::CreateGraphs(UCellFlowLayoutGraph* LayoutGraph, UDAFlowCellGraph* CellGraph, const FRandomStream& InRandom) {
    TMap<int32, FGuid> GroupToFlowNode;
    TMap<FGuid, UFlowAbstractNode*> FlowNodes;
    for (FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
        if (GroupNode.IsActive()) {
            const FVector Coord = FVector(GroupNode.PreviewLocation, 0);
            
            // Create a new Layout node
            UFlowAbstractNode* LayoutNode = LayoutGraph->CreateNode(InRandom);
            LayoutNode->Coord = Coord;

            // Save the ID of the layout node in the cell group node
            GroupNode.LayoutNodeID = LayoutNode->NodeId;

            // Cache to lookup tables
            GroupToFlowNode.Add(GroupNode.GroupId, LayoutNode->NodeId);
            FlowNodes.Add(LayoutNode->NodeId, LayoutNode);
        }
    }

    auto GetFlowNodeFromGroupId = [&](int GroupId) -> UFlowAbstractNode* {
        const FGuid* FlowNodeIdPtr = GroupToFlowNode.Find(GroupId);
        return (FlowNodeIdPtr == nullptr) ? nullptr : FlowNodes[*FlowNodeIdPtr];
    };
    
    for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
        if (GroupNode.IsActive()) {
            if (const UFlowAbstractNode* SrcNode = GetFlowNodeFromGroupId(GroupNode.GroupId)) {
                for (const int32 OtherGroupId : GroupNode.Connections) {
                    if (GroupNode.GroupId < OtherGroupId) {
                        if (const UFlowAbstractNode* DstNode = GetFlowNodeFromGroupId(OtherGroupId)) {
                            LayoutGraph->CreateLink(SrcNode->NodeId, DstNode->NodeId, InRandom);
                        }
                    }
                }
            }
        }
    }
}

bool UCellFlowLayoutTaskCreateCellsBase::GetParameter(const FString& InParameterName, FDAAttribute& OutValue) {
    //FLOWTASKATTR_GET_VECTOR(WorldSize);

    return false;
    
}

bool UCellFlowLayoutTaskCreateCellsBase::SetParameter(const FString& InParameterName, const FDAAttribute& InValue) {
    //FLOWTASKATTR_SET_VECTORF(WorldSize)

    return false;
    
}

bool UCellFlowLayoutTaskCreateCellsBase::SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) {
    //FLOWTASKATTR_SET_PARSE_VECTORF(WorldSize)
    
    return false;
}

void UCellFlowLayoutTaskCreateCellsBase::GenerateCells(UDAFlowCellGraph* InCellGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const {
    GenerateCellsImpl(InCellGraph, Input, OutputState);

    // Copy the initial connects as the leaf node adjacency list
    // This will not change as the group connection list changes, when it merges
    for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
        if (GroupNode.LeafNodes.Num() == 1) {
            TSet<int32>::TConstIterator It(GroupNode.LeafNodes);
            int32 LeafNodeIdx = *It;
            if (UDAFlowCellLeafNode* LeafNode = InCellGraph->LeafNodes[LeafNodeIdx]) {
                check(LeafNode->CellId == LeafNodeIdx);
                
                LeafNode->AdjacentLeafs = GroupNode.Connections;
            }
        }
    }
}

void UCellFlowLayoutTaskCreateCellsBase::FinalizeCells(UDAFlowCellGraph* InCellGraph) const {
    FinalizeCellsImpl(InCellGraph);
}

void UCellFlowLayoutTaskCreateCellsBase::FinalizeCellsImpl(UDAFlowCellGraph* InCellGraph) const {
    
}

