//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/Lib/FlowAbstractGraphPathUtils.h"

#include "Core/Utils/MathUtils.h"
#include "Core/Utils/StackSystem.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphConstraints.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractNode.h"
#include "Frameworks/Flow/Utils/DungeonFlowTagVolume.h"

void FNullFlowAGNodeGroupGenerator::Generate(const FFlowAbstractGraphQuery& InGraphQuery, const UFlowAbstractNode* InCurrentNode,
                                             int32 InPathIndex, int32 InPathLength, const FRandomStream& InRandom, const TSet<FGuid>& InVisited,
                                             TArray<FFlowAGPathNodeGroup>& OutGroups) const {
    OutGroups.Reset();
    if (InCurrentNode) {
        FFlowAGPathNodeGroup& Group = OutGroups.AddDefaulted_GetRef();
        Group.bIsGroup = false;
        Group.GroupNodes.Add(InCurrentNode->NodeId);
        Group.GroupEdgeNodes.Add(InCurrentNode->NodeId);
    }
}

bool UFlowLayoutNodeCreationConstraint::CanCreateNodeAt(const UFlowAbstractNode* Node, int32 TotalPathLength, int32 CurrentPathPosition) {
    FVector NodeWorldCoord = CoordToWorld.TransformPosition(Node->Coord);
    FName NodePathName = PathName;
    if (CurrentPathPosition == 0 && !StartNodePathName.IsNone()) {
        NodePathName = StartNodePathName;
    }
    else if (CurrentPathPosition == TotalPathLength - 1 && !EndNodePathName.IsNone()) {
        NodePathName = EndNodePathName;
    }

    bool bResult = true;
    for (const TWeakObjectPtr<ADungeonFlowTagVolume>& VolumePtr : ConstraintVolumes) {
        if (VolumePtr.IsValid()) {
            for (const FFlowPathTagConstraints& PathConstraint : VolumePtr->PathConstraints) {
                if (PathConstraint.PathName == NodePathName) {
                    if (PathConstraint.ConstraintType == EFlowPathTagConstraintsType::AllNodesInPath) {
                        // Check if we're inside the bounds
                        FVector AdjustedCoord = NodeWorldCoord;
                        AdjustedCoord.Z = VolumePtr->GetActorLocation().Z;
                        
                        FBox VolumeBounds(ForceInit);
                        {
                            FVector VolumeBoundsOrigin{}, VolumeBoundsExtent{};
                            VolumePtr->GetActorBounds(false, VolumeBoundsOrigin, VolumeBoundsExtent);
                            VolumeBounds = FBox(VolumeBoundsOrigin - VolumeBoundsExtent, VolumeBoundsOrigin + VolumeBoundsExtent);
                        }
                        
                        if (!VolumeBounds.IsInsideOrOn(AdjustedCoord)) {
                            bResult = false;
                            break;
                        }
                    }
                }
                else if (PathConstraint.ConstraintType == EFlowPathTagConstraintsType::ExcludeFromPath) {
                    if (PathConstraint.PathName.IsNone() || PathConstraint.PathName == NodePathName) {
                        // Make sure we're not in the bounds
                        FVector AdjustedCoord = NodeWorldCoord;
                        AdjustedCoord.Z = VolumePtr->GetActorLocation().Z;
                        if (VolumePtr->EncompassesPoint(AdjustedCoord)) {  
                            bResult = false;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return bResult;
}

void FFlowAbstractGraphPathUtils::FinalizePath(const FFlowAGStaticGrowthState& StaticState, FFlowAGGrowthState& State,
                                               const TFunction<void(UFlowAbstractNode*, const FFlowAGGrowthState_PathItem&)>& FinalizeNode) {
    TArray<FFlowAGGrowthState_PathItem>& Path = State.Path;

    if (Path.Num() == 0) {
        return;
    }
    
    // Create merged node groups
    {
        // Use a random seed that's combined with the dungeon UID, This avoids node UID collision with different dungeons on the same map 
        const FRandomStream NamingSchemeRandomStream(FFlowAbstractGraphUtils::CreateUidRandomSeed(StaticState.Random, StaticState.Dungeon));
        for (const FFlowAbstractNodeGroup& GroupInfo : State.NodeGroups) {
            CreateMergedCompositeNode(*StaticState.GraphQuery, GroupInfo, NamingSchemeRandomStream);
        }
    }
    
    UFlowAbstractGraphBase* Graph = StaticState.GraphQuery->GetGraph<UFlowAbstractGraphBase>();
    TMap<FGuid, FGuid> ChildToParentMap; // [ChildNodeId -> ParentNodeId]
    for (UFlowAbstractNode* ParentNode : Graph->GraphNodes) {
        if (ParentNode->MergedCompositeNodes.Num() > 1) {
            for (UFlowAbstractNode* ChildNode : ParentNode->MergedCompositeNodes) {
                FGuid& ParentId = ChildToParentMap.FindOrAdd(ChildNode->NodeId);
                ParentId = ParentNode->NodeId;
            }
        }
    }

    const int32 PathLength = Path.Num();
    for (int i = 0; i < PathLength; i++) {
        FFlowAGGrowthState_PathItem& PathItem = Path[i];
        const FGuid OrigNodeId = PathItem.NodeId;
        const FGuid OrigPrevNodeId = PathItem.PreviousNodeId;
        if (FGuid* ParentNodeID = ChildToParentMap.Find(PathItem.NodeId)) {
            PathItem.NodeId = *ParentNodeID;
        }
        if (FGuid* ParentNodeID = ChildToParentMap.Find(PathItem.PreviousNodeId)) {
            PathItem.PreviousNodeId = *ParentNodeID;
        }
        
        UFlowAbstractNode* PathNode = StaticState.GraphQuery->GetNode(PathItem.NodeId);
        if (!PathNode) continue;
        PathNode->bActive = true;
        PathNode->Color = StaticState.NodeColor;
        PathNode->PathIndex = i;
        PathNode->PathLength = PathLength;

        FString PathName;
        if (i == 0 && StaticState.StartNodePathNameOverride.Len() > 0) {
            PathName = StaticState.StartNodePathNameOverride;
        }
        else if (i == Path.Num() - 1 && StaticState.EndNodePathNameOverride.Len() > 0) {
            PathName = StaticState.EndNodePathNameOverride;
        }
        else {
            PathName = StaticState.PathName;
        }
        PathNode->PathName = PathName;
        

        // Link the path nodes
        if (i > 0) {
            const FGuid LinkSrc = PathItem.PreviousNodeId;
            const FGuid LinkDst = PathItem.NodeId;
            const FGuid LinkSrcSub = OrigPrevNodeId;
            const FGuid LinkDstSub = OrigNodeId;
            TArray<UFlowAbstractLink*> PossibleLinks = StaticState.Graph->GetLinks(LinkSrc, LinkDst, true);
            UFlowAbstractLink* TargetLink = nullptr;
            for (UFlowAbstractLink* PossibleLink : PossibleLinks) {
                if (!PossibleLink) continue;
                if (PossibleLink->Source == LinkSrc && PossibleLink->Destination == LinkDst) {
                    bool bValid = (!PossibleLink->SourceSubNode.IsValid() || PossibleLink->SourceSubNode == LinkSrcSub);
                    bValid &= (!PossibleLink->DestinationSubNode.IsValid() || PossibleLink->DestinationSubNode == LinkDstSub);

                    // Found the correct link
                    if (bValid) {
                        PossibleLink->Type = EFlowAbstractLinkType::Connected;
                        break;
                    }
                }
                else if (PossibleLink->Source == LinkDst && PossibleLink->Destination == LinkSrc) {
                    bool bValid = (!PossibleLink->SourceSubNode.IsValid() || PossibleLink->SourceSubNode == LinkDstSub);
                    bValid &= (!PossibleLink->DestinationSubNode.IsValid() || PossibleLink->DestinationSubNode == LinkSrcSub);

                    // Found the correct link
                    if (bValid) {
                        PossibleLink->Type = EFlowAbstractLinkType::Connected;
                        PossibleLink->ReverseDirection();
                        break;
                    }
                }
            }
        }
    }

    for (int PathIdx = 0; PathIdx < PathLength; PathIdx++) {
        const FFlowAGGrowthState_PathItem& PathItem = State.Path[PathIdx];
        UFlowAbstractNode* Node = StaticState.GraphQuery->GetNode(PathItem.NodeId);
        check(Node);

        FinalizeNode(Node, PathItem);
    };
    
    auto GetValidConnectionLinks = [&StaticState](const FGuid& LinkSrc, const FGuid& LinkDst) {
        TArray<UFlowAbstractLink*> Result;
        const TArray<UFlowAbstractLink*> Links = StaticState.Graph->GetLinks(LinkSrc, LinkDst, true);
        for (UFlowAbstractLink* Link : Links) {
            // Check if the assembly has a valid connection through this link
            bool bSrcNodeConnectionValid{};
            if (const UFlowAbstractNode* DstSubNode = TryGetSubNodeFromLink(*StaticState.GraphQuery, LinkDst, Link)) {
                if (const UFlowAbstractNode* SrcNode = StaticState.GraphQuery->GetNode(LinkSrc)) {
                    bSrcNodeConnectionValid = StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, SrcNode, {}, {DstSubNode});
                }
            }
            
            bool bDstNodeConnectionValid{};
            if (const UFlowAbstractNode* SrcSubNode = TryGetSubNodeFromLink(*StaticState.GraphQuery, LinkSrc, Link)) {
                if (const UFlowAbstractNode* DstNode = StaticState.GraphQuery->GetNode(LinkDst)) {
                    bDstNodeConnectionValid = StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, DstNode, {SrcSubNode}, {});
                }
            }
            
            if (bSrcNodeConnectionValid && bDstNodeConnectionValid) {
                Result.Add(Link);
            }
        }
        return Result;
    };
    
    // Setup the start / end links
    if (StaticState.HeadNode) {
        const FGuid LinkSrc = StaticState.HeadNode->NodeId;
        const FGuid LinkDst = Path[0].NodeId;
        TArray<UFlowAbstractLink*> ValidConnectionLinks = GetValidConnectionLinks(LinkSrc, LinkDst);
        FDAMathUtils::Shuffle(ValidConnectionLinks, *StaticState.Random);
        for (UFlowAbstractLink* ConnectionLink : ValidConnectionLinks) {
            ConnectionLink->Type = EFlowAbstractLinkType::Connected;
            if (ConnectionLink->Source == LinkDst && ConnectionLink->Destination == LinkSrc) {
                ConnectionLink->ReverseDirection();
            }
            break;
        }
    }

    // Find the end node, if any so that it can merge back to the specified branch (specified in variable EndOnPath)
    if (State.TailNode.IsValid()) {
        const FGuid LinkSrc = Path[Path.Num() - 1].NodeId;
        const FGuid LinkDst = State.TailNode->NodeId;
        TArray<UFlowAbstractLink*> ValidConnectionLinks = GetValidConnectionLinks(LinkSrc, LinkDst);
        FDAMathUtils::Shuffle(ValidConnectionLinks, *StaticState.Random);
        for (UFlowAbstractLink* ConnectionLink : ValidConnectionLinks) {
            ConnectionLink->Type = EFlowAbstractLinkType::Connected;
            if (ConnectionLink->Source == LinkDst && ConnectionLink->Destination == LinkSrc) {
                ConnectionLink->ReverseDirection();
            }
            break;
        }
    }
}

UFlowAbstractNode* FFlowAbstractGraphPathUtils::CreateMergedCompositeNode(FFlowAbstractGraphQuery& GraphQuery, const FFlowAbstractNodeGroup& NodeGroup, const FRandomStream& InRandom) {
    if (NodeGroup.GroupNodes.Num() <= 1) {
        return nullptr;
    }
    
    UFlowAbstractGraphBase* Graph = GraphQuery.GetGraph<UFlowAbstractGraphBase>();
    TSet<UFlowAbstractNode*> SubNodes;
    TSet<FGuid> SubNodeIds;
    TSet<UFlowGraphItem*> SubItems;
    
    FVector Coord = FVector::ZeroVector;
    for (const FGuid& SubNodeId : NodeGroup.GroupNodes) {
        UFlowAbstractNode* SubNode = Graph->GetNode(SubNodeId);
        if (!SubNode) continue;
        SubNodes.Add(SubNode);
        SubNodeIds.Add(SubNodeId);
        SubItems.Append(SubNode->NodeItems);
        Coord += SubNode->Coord;
    }
    const int32 NumSubNodes = SubNodes.Num();
    if (NumSubNodes > 0) {
        Coord /= NumSubNodes;

        UFlowAbstractNode* NewNode = Graph->CreateNode(InRandom);
        NewNode->bActive = true;
        NewNode->NodeItems = SubItems.Array();
        NewNode->Coord = Coord;
        NewNode->MergedCompositeNodes = SubNodes.Array();

        // Remove all the sub nodes from the graph 
        for (UFlowAbstractNode* SubNode : SubNodes) {
            Graph->GraphNodes.Remove(SubNode);
        }

        for (UFlowAbstractLink* Link : Graph->GraphLinks) {
            if (SubNodeIds.Contains(Link->Source)) {
                Link->SourceSubNode = Link->Source;
                Link->Source = NewNode->NodeId;
            }
            if (SubNodeIds.Contains(Link->Destination)) {
                Link->DestinationSubNode = Link->Destination;
                Link->Destination = NewNode->NodeId;
            }
        }
        
        Graph->GraphLinks = Graph->GraphLinks.FilterByPredicate([](const UFlowAbstractLink* Link) { return Link->Source != Link->Destination; });
        GraphQuery.Rebuild();
        return NewNode;
    }

    return nullptr;
}

void FFlowAbstractGraphPathUtils::GetNodeCoords(const FFlowAbstractGraphQuery& GraphQuery, const TArray<FGuid>& GroupNodes, TSet<FVector>& OutCoords) {
    for (const FGuid& NodeId : GroupNodes) {
        if (const UFlowAbstractNode* Node = GraphQuery.GetNode(NodeId)) {
            GetNodeCoords(Node, OutCoords);
        }
    }
}

void FFlowAbstractGraphPathUtils::GetNodeCoords(const UFlowAbstractNode* InNode, TSet<FVector>& OutCoords) {
    if (InNode) {
        if (InNode->MergedCompositeNodes.Num() > 0) {
            for (const UFlowAbstractNode* ChildNode : InNode->MergedCompositeNodes) {
                if (ChildNode) {
                    OutCoords.Add(ChildNode->Coord);
                }
            }
        }
        else {
            OutCoords.Add(InNode->Coord);
        }
    }
}

UFlowAbstractNode* FFlowAbstractGraphPathUtils::TryGetSubNodeFromLink(const FFlowAbstractGraphQuery& GraphQuery, const FGuid& InTargetNodeId, const UFlowAbstractLink* InLink) {
    FGuid TargetNodeId = InTargetNodeId;
    if (InLink->Source == TargetNodeId && InLink->SourceSubNode.IsValid()) {
        TargetNodeId = InLink->SourceSubNode;
    }
    else if (InLink->Destination == TargetNodeId && InLink->DestinationSubNode.IsValid()) {
        TargetNodeId = InLink->DestinationSubNode;
    }

    UFlowAbstractNode* TargetNode = GraphQuery.GetSubNode(TargetNodeId);
    if (!TargetNode) {
        TargetNode = GraphQuery.GetNode(TargetNodeId);
    }
    return TargetNode;
}

//////////////////////////////////////// FFlowAGPathStackGrowthTask ////////////////////////////////////////

void FFlowAGPathStackGrowthTask::Execute(const FFlowAGPathStackFrame& InFrameState, const FFlowAGStaticGrowthState& StaticState,
                                         FlowPathGrowthSystem& StackSystem) {

    check(StaticState.MinPathSize > 0 && StaticState.MaxPathSize > 0);
    check(StaticState.GraphQuery);

    const FFlowAGGrowthState& State = InFrameState.State;
    const UFlowAbstractNode* CurrentNode = InFrameState.CurrentNode.Get();
    const UFlowAbstractNode* IncomingNode = InFrameState.IncomingNode.Get();

    const int32 PathIndex = State.Path.Num();
    const bool bFirstNodeInPath = (PathIndex == 0);
    
    if (bFirstNodeInPath && StaticState.HeadNode) {
        // Check if we can connect from the head node to this node
        if (!StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, StaticState.HeadNode, {}, {CurrentNode})) {
            return;
        }
    }

    TArray<FFAGConstraintsLink> BaseIncomingConstraintLinks;
    if (bFirstNodeInPath && StaticState.HeadNode) {
        UFlowAbstractNode* HeadSubNode = StaticState.HeadNode;
        if (StaticState.HeadNode->MergedCompositeNodes.Num() > 1) {
            for (UFlowAbstractLink* GraphLink : StaticState.Graph->GraphLinks) {
                if (GraphLink->Type != EFlowAbstractLinkType::Unconnected) continue;
                if (GraphLink->Source == CurrentNode->NodeId && GraphLink->Destination == StaticState.HeadNode->NodeId) {
                    HeadSubNode = StaticState.GraphQuery->GetSubNode(GraphLink->DestinationSubNode);
                    check(HeadSubNode);
                    break;
                }
                else if (GraphLink->Source == StaticState.HeadNode->NodeId && GraphLink->Destination == CurrentNode->NodeId) {
                    HeadSubNode = StaticState.GraphQuery->GetSubNode(GraphLink->SourceSubNode);
                    check(HeadSubNode);
                    break;
                }
            }
        }
        BaseIncomingConstraintLinks.Add({CurrentNode, HeadSubNode, EFAGConstraintsLinkDirection::Incoming});
    }
    if (IncomingNode) {
        BaseIncomingConstraintLinks.Add({CurrentNode, IncomingNode, EFAGConstraintsLinkDirection::Incoming});
    }

    auto CanCreateNodeAt = [&StaticState](const FFlowAGPathStackFrame& InFrameState, int32 PathLength, int32 PathIndex) {
        if (StaticState.NodeCreationConstraint.IsValid()) {
            if (!StaticState.NodeCreationConstraint->CanCreateNodeAt(InFrameState.CurrentNode.Get(), PathLength, PathIndex)) {
                return false;
            }
        }
        return true;
    };
    
    auto FuncExecute = [&](int PathLength) {
        if (!CanCreateNodeAt(InFrameState, PathLength, PathIndex)) {
            return;
        }

        TArray<FFlowAGPathNodeGroup> SortedNodeGroups;
        {
            TArray<FFlowAGPathNodeGroup> PossibleNodeGroups;
            check(StaticState.NodeGroupGenerator.IsValid());
            StaticState.NodeGroupGenerator->Generate(*StaticState.GraphQuery, CurrentNode, PathIndex, PathLength, *StaticState.Random, State.Visited, PossibleNodeGroups);

            FDAMathUtils::Shuffle(PossibleNodeGroups, *StaticState.Random);

            while (PossibleNodeGroups.Num() > 0) {
                int IndexToProcess = 0;
                {
                    float MaxWeight = 0;
                    for (const FFlowAGPathNodeGroup& PossibleNodeGroup : PossibleNodeGroups) {
                        MaxWeight = FMath::Max(MaxWeight, PossibleNodeGroup.Weight);
                    }

                    float FrameSelectionWeight = StaticState.Random->FRand() * MaxWeight;
                    for (int i = 0; i < PossibleNodeGroups.Num(); i++) {
                        if (FrameSelectionWeight <= PossibleNodeGroups[i].Weight) {
                            IndexToProcess = i;
                            break;
                        }
                    }
                }
                SortedNodeGroups.Add(PossibleNodeGroups[IndexToProcess]);
                PossibleNodeGroups.RemoveAt(IndexToProcess);
            }
        }

        TArray<FFlowAGPathStackFrame> FramesToPush;
        for (const FFlowAGPathNodeGroup& GrowthNodeGroup : SortedNodeGroups) {
            // Check if we can use this newly created group node by connecting in to it
            if (!StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, GrowthNodeGroup, PathIndex, PathLength, BaseIncomingConstraintLinks)) continue;

            FFlowAGGrowthState NextState = State;

            // Update the frame path and visited state
            NextState.Visited.Append(GrowthNodeGroup.GroupNodes);
            FFlowAGGrowthState_PathItem PathFrame;
            PathFrame.NodeId = CurrentNode->NodeId;
            PathFrame.PreviousNodeId = IncomingNode ? IncomingNode->NodeId : FGuid();
            PathFrame.UserData = GrowthNodeGroup.UserData;
            NextState.Path.Push(PathFrame);

            // Add path node group info
            if (GrowthNodeGroup.bIsGroup) {
                FFlowAbstractNodeGroup NodeGroup;
                NodeGroup.GroupId = FGuid::NewGuid();
                NodeGroup.GroupNodes = GrowthNodeGroup.GroupNodes;
                NextState.NodeGroups.Push(NodeGroup);
            }

            // Check if we reached the desired path size
            if (NextState.Path.Num() >= PathLength) {
                // Check if we are near the sink node, if any
                if (StaticState.SinkNodes.Num() == 0) {
                    // No sink nodes defined.
                    StackSystem.FinalizeResult({ NextState, StaticState });
                    return;
                }

                {
                    TArray<int32> SinkNodeIndices = FDAMathUtils::GetShuffledIndices(StaticState.SinkNodes.Num(), *StaticState.Random);
                    const TArray<int32> GroupEdgeNodeIndices = FDAMathUtils::GetShuffledIndices(GrowthNodeGroup.GroupEdgeNodes.Num(), *StaticState.Random);
                    for (int32 GroupEdgeNodeIndex : GroupEdgeNodeIndices) {
                        const FGuid& GroupEdgeNodeId = GrowthNodeGroup.GroupEdgeNodes[GroupEdgeNodeIndex];
                        TArray<FGuid> ConnectedNodeIds = StaticState.Graph->GetConnectedNodes(GroupEdgeNodeId);
                        const TArray<int32> ConnectedNodeIndices = FDAMathUtils::GetShuffledIndices(ConnectedNodeIds.Num(), *StaticState.Random);
                        for (int32 ConnectedNodeIndex : ConnectedNodeIndices) {
                            const FGuid& ConnectedNodeId = ConnectedNodeIds[ConnectedNodeIndex];
                            UFlowAbstractNode* ConnectedNode = StaticState.GraphQuery->GetNode(ConnectedNodeId);
                            for (int32 SinkNodeIndex : SinkNodeIndices) {
                                UFlowAbstractNode* SinkNode = StaticState.SinkNodes[SinkNodeIndex].Get();
                                if (!SinkNode) continue;

                                if (NextState.Path.Num() == 1 && SinkNode == StaticState.HeadNode) {
                                    // If the path node size is 1, we don't want to connect back to the head node
                                    continue;
                                }
                                if (ConnectedNode == SinkNode) {
                                    UFlowAbstractNode* GroupEdgeNode = StaticState.GraphQuery->GetNode(GroupEdgeNodeId);
                                    // TODO: Iterate through the edge nodes and check if we can connect to the tail node
                                    TArray<FFAGConstraintsLink> IncomingConstraintLinks = BaseIncomingConstraintLinks;
                                    UFlowAbstractNode* ConnectedSubNode = ConnectedNode;
                                    if (ConnectedNode->MergedCompositeNodes.Num() > 1) {
                                        for (UFlowAbstractLink* GraphLink : StaticState.Graph->GraphLinks) {
                                            if (GraphLink->Type != EFlowAbstractLinkType::Unconnected) continue;
                                            if (GraphLink->Source == GroupEdgeNodeId && GraphLink->Destination == ConnectedNodeId) {
                                                ConnectedSubNode = StaticState.GraphQuery->GetSubNode(GraphLink->DestinationSubNode);
                                                check(ConnectedSubNode);
                                                break;
                                            }
                                            else if (GraphLink->Source == ConnectedNodeId && GraphLink->Destination == GroupEdgeNodeId) {
                                                ConnectedSubNode = StaticState.GraphQuery->GetSubNode(GraphLink->SourceSubNode);
                                                check(ConnectedSubNode);
                                                break;
                                            }
                                        }
                                    }
                                    IncomingConstraintLinks.Add({GroupEdgeNode, ConnectedSubNode, EFAGConstraintsLinkDirection::Outgoing});
                                    if (!StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, GrowthNodeGroup, PathIndex, PathLength, IncomingConstraintLinks)) {
                                        continue;
                                    }

                                    TArray<const UFlowAbstractNode*> SinkIncomingNodes = {GroupEdgeNode};
                                    if (SinkNode == StaticState.HeadNode) {
                                        // The sink and the head are the same. Add the first node to the connected list
                                        UFlowAbstractNode* FirstNodeInPath = StaticState.GraphQuery->GetNode(NextState.Path[0].NodeId);
                                        if (FirstNodeInPath) {
                                            SinkIncomingNodes.Add(FirstNodeInPath);
                                        }
                                    }

                                    if (!StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, SinkNode, SinkIncomingNodes, {}))
                                        continue;

                                    NextState.TailNode = SinkNode;
                                    StackSystem.FinalizeResult({ NextState, StaticState });
                                    return;
                                }
                            }
                        }
                    }
                }

                // no sink nodes nearby and we've reached the max path size
                return;
            }


            // Try to grow from each outgoing node
            {
                const TArray<int32> GroupEdgeNodeIndices = FDAMathUtils::GetShuffledIndices(GrowthNodeGroup.GroupEdgeNodes.Num(), *StaticState.Random);
                for (int32 GroupEdgeNodeIndex : GroupEdgeNodeIndices) {
                    const FGuid& GroupEdgeNodeId = GrowthNodeGroup.GroupEdgeNodes[GroupEdgeNodeIndex];
                    TArray<FGuid> ConnectedNodeIds = StaticState.Graph->GetConnectedNodes(GroupEdgeNodeId);
                    const TArray<int32> ConnectedNodeIndices = FDAMathUtils::GetShuffledIndices(ConnectedNodeIds.Num(), *StaticState.Random);
                    for (int32 ConnectedNodeIndex : ConnectedNodeIndices) {
                        const FGuid& ConnectedNodeId = ConnectedNodeIds[ConnectedNodeIndex];
                        if (NextState.Visited.Contains(ConnectedNodeId)) continue;

                        UFlowAbstractNode* ConnectedNode = StaticState.GraphQuery->GetNode(ConnectedNodeId);
                        if (!ConnectedNode) continue;
                        if (ConnectedNode->bActive) continue;
                        UFlowAbstractNode* GroupEdgeNode = StaticState.GraphQuery->GetNode(GroupEdgeNodeId);

                        TArray<FFAGConstraintsLink> IncomingConstraintLinks = BaseIncomingConstraintLinks;
                        IncomingConstraintLinks.Add({GroupEdgeNode, ConnectedNode, EFAGConstraintsLinkDirection::Outgoing});
                        if (!StaticState.GraphConstraint->IsValid(*StaticState.GraphQuery, GrowthNodeGroup, PathIndex, PathLength, IncomingConstraintLinks)) {
                            continue;
                        }

                        FFlowAGPathStackFrame NextFrame;
                        NextFrame.State = NextState;
                        NextFrame.CurrentNode = ConnectedNode;
                        NextFrame.IncomingNode = GroupEdgeNode;
                        FramesToPush.Add(NextFrame);
                    }
                }
            }
        }

        Algo::Reverse(FramesToPush);
        for (const FFlowAGPathStackFrame& Frame : FramesToPush) {
            StackSystem.PushFrame(Frame);
        }
    };
    
    //const int32 PathLength = FMath::Clamp(PathIndex + 1, StaticState.MinPathSize, StaticState.MaxPathSize);
    const int32 PathLength1 = FMath::Clamp(PathIndex + 1, StaticState.MinPathSize, StaticState.MaxPathSize);
    const int32 PathLength2 = StaticState.MaxPathSize;
    FuncExecute(PathLength1);
    if (PathLength1 != PathLength2) {
        FuncExecute(PathLength2);
    }
}

//////////////////////////////////////////// FFlowAGPathingSystem ////////////////////////////////////////////

void FFlowAGPathingSystem::RegisterGrowthSystem(const UFlowAbstractNode* StartNode, const FFlowAGStaticGrowthState& StaticState, const int32 Count) {
    check(Count > 0);

    for (int i = 0; i < Count; i++) {
        FFlowAGPathStackFrame InitFrame;
        InitFrame.CurrentNode = StartNode;
        InitFrame.IncomingNode = nullptr;
        FlowPathGrowthSystemPtr GrowthSystem = MakeShareable(new FlowPathGrowthSystem(StaticState));
        GrowthSystem->Initialize(InitFrame);
        GrowthSystems.Add(GrowthSystem);
    }
}

void FFlowAGPathingSystem::Execute(int32 StartIdx, int32 EndIdx) {
    bool bRunning = true;
    while (bRunning && !bTimeout && !bFoundResult) {
        bRunning = false;
        for (int i = StartIdx; i <= EndIdx; i++) {
            FlowPathGrowthSystemPtr GrowthSystem = GrowthSystems[i];
            if (GrowthSystem->IsRunning()) {
                GrowthSystem->ExecuteStep();
                bRunning |= GrowthSystem->IsRunning();
                if (GrowthSystem->FoundResult()) {
                    bFoundResult = true;
                    Result = GrowthSystem->GetResult();
                    break;
                }

                FrameCounter++;
                if (FrameCounter >= MaxFramesToProcess) {
                    bTimeout = true;
                    break;
                }
            }
        }
    }
}

void FFlowAGPathingSystem::Execute(int32 InNumParallelSearches) {
    InNumParallelSearches = FMath::Max(InNumParallelSearches, 1);
    
    FrameCounter = 0;
    for (int i = 0; i < GrowthSystems.Num(); i += InNumParallelSearches) {
        const int32 StartIdx = i;
        const int32 EndIdx = FMath::Min(i + InNumParallelSearches - 1, GrowthSystems.Num() - 1);
        Execute(StartIdx, EndIdx);
        if (bFoundResult || bTimeout) {
            break;
        }
    }
}

