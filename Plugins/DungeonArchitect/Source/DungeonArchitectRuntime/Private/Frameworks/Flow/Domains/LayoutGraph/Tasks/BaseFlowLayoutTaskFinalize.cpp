//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskFinalize.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractLink.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTaskAttributeMacros.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraph.h"

void UBaseFlowLayoutTaskFinalize::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
    check(Input.IncomingNodeOutputs.Num() == 1);

    Output.State = Input.IncomingNodeOutputs[0].State->Clone();
    UFlowAbstractGraphBase* Graph = Output.State->GetState<UFlowAbstractGraphBase>(UFlowAbstractGraphBase::StateTypeID);
    const FFlowAbstractGraphQuery GraphQuery(Graph);

    TMap<FGuid, FFlowGraphItemContainer> ItemMap;
    for (UFlowAbstractNode* Node : Graph->GraphNodes) {
        if (!Node) continue;
        for (const UFlowGraphItem* Item : Node->NodeItems) {
            FFlowGraphItemContainer& ItemInfo = ItemMap.FindOrAdd(Item->ItemId);
            ItemInfo.ItemId = Item->ItemId;
            ItemInfo.HostNode = Node;
        }
    }
    for (UFlowAbstractLink* Link : Graph->GraphLinks) {
        for (const UFlowGraphItem* Item : Link->LinkItems) {
            FFlowGraphItemContainer& ItemInfo = ItemMap.FindOrAdd(Item->ItemId);
            ItemInfo.ItemId = Item->ItemId;
            ItemInfo.HostLink = Link;
        }
    }

    TMap<FGuid, float> Weights = FFlowAbstractGraphUtils::CalculateNodeWeights(GraphQuery, 10);

    // Make the links one directional if the difference in the source/dest nodes is too much
    for (UFlowAbstractLink* Link : Graph->GraphLinks) {
        if (Link->Type == EFlowAbstractLinkType::Unconnected) continue;
        const UFlowAbstractNode* Source = GraphQuery.GetNode(Link->Source);
        const UFlowAbstractNode* Destination = GraphQuery.GetNode(Link->Destination);
        if (!Source || !Destination) continue;
        if (!Source->bActive || !Destination->bActive) continue;

        const float* WeightSourcePtr = Weights.Find(Link->Source);
        const float* WeightDestinationPtr = Weights.Find(Link->Destination);
        if (WeightSourcePtr && WeightDestinationPtr) {
            const float WeightDiff = *WeightSourcePtr - *WeightDestinationPtr + 1;
            if (WeightDiff > OneWayDoorPromotionWeight) {
                if (FFlowAbstractGraphUtils::CanPromoteToOneWayLink(GraphQuery, Link->LinkId)) {
                    Link->Type = EFlowAbstractLinkType::OneWay;
                }
            }
        }
    }

    // Remove unconnected links
    TArray<UFlowAbstractLink*> LinksCopy = Graph->GraphLinks;
    for (UFlowAbstractLink* LinkCopy : LinksCopy) {
        if (LinkCopy->Type == EFlowAbstractLinkType::Unconnected) {
            Graph->GraphLinks.Remove(LinkCopy);
        }
    }

    // Emit debug info
    if (bShowDebugData) {
        for (UFlowAbstractNode* Node : Graph->GraphNodes) {
            if (!Node) continue;
            const float* WeightPtr = Weights.Find(Node->NodeId);
            if (!WeightPtr) continue;
            
            UFlowGraphItem* DebugItem = Node->CreateNewItem(UFlowGraphItem::StaticClass());
            DebugItem->ItemType = EFlowGraphItemType::Custom;
            DebugItem->CustomInfo.PreviewText = FString::Printf(TEXT("%d"), FMath::RoundToInt(*WeightPtr));
            DebugItem->CustomInfo.PreviewBackgroundColor = FLinearColor::Black;
            DebugItem->CustomInfo.PreviewTextColor = FLinearColor::White;
            DebugItem->CustomInfo.FontScale = 0.75f;
        }
    }

    
    Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}


bool UBaseFlowLayoutTaskFinalize::GetParameter(const FString& InParameterName, FDAAttribute& OutValue) {
    FLOWTASKATTR_GET_FLOAT(OneWayDoorPromotionWeight);

    return false;
}

bool UBaseFlowLayoutTaskFinalize::SetParameter(const FString& InParameterName, const FDAAttribute& InValue) {
    FLOWTASKATTR_SET_FLOAT(OneWayDoorPromotionWeight);

    return false;
}

bool UBaseFlowLayoutTaskFinalize::SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) {
    FLOWTASKATTR_SET_PARSE_FLOAT(OneWayDoorPromotionWeight);

    return false;
}

