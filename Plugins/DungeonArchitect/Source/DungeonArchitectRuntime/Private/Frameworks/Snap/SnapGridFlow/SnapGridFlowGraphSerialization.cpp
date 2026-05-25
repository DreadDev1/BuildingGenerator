//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowGraphSerialization.h"

#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraph.h"

void FSGFSnapConnectionSerializePolicy::GatherConnectionInfo(const FGuid& ModuleA, const FGuid& ModuleB,
                                                            ESnapConnectionDoorType& OutConnectionType, FString& OutCustomMarkerName) const {
	OutCustomMarkerName = {};
	OutConnectionType = ESnapConnectionDoorType::NotApplicable;

	if (USnapGridFlowAbstractGraph* LayoutGraph = LayoutGraphPtr.Get()) {
		if (const UFlowAbstractLink* Link = LayoutGraph->GetLink(ModuleA, ModuleB, false)) {
			if (Link->Type == EFlowAbstractLinkType::Unconnected) {
				OutConnectionType = ESnapConnectionDoorType::NotApplicable;
			}
			else if (Link->Type == EFlowAbstractLinkType::OneWay) {
				const UFlowAbstractNode* SourceNode = Link->SourceSubNode.IsValid() ? LayoutGraph->FindSubNode(Link->SourceSubNode) : LayoutGraph->GetNode(Link->Source);
				const UFlowAbstractNode* DestNode = Link->DestinationSubNode.IsValid() ? LayoutGraph->FindSubNode(Link->DestinationSubNode) : LayoutGraph->GetNode(Link->Destination);
				check(SourceNode && DestNode);

				const float SourceZ = SourceNode->Coord.Z;
				const float DestZ = DestNode->Coord.Z;


				if (FMath::IsNearlyEqual(SourceZ, DestZ, 1e-4f)) {
					OutConnectionType = ESnapConnectionDoorType::OneWayDoor;
				}
				else if (SourceZ < DestZ) {
					OutConnectionType = ESnapConnectionDoorType::OneWayDoorUp;
				}
				else {
					OutConnectionType = ESnapConnectionDoorType::OneWayDoorDown;
				}
			}
			else {
				check(Link->Type == EFlowAbstractLinkType::Connected);
				if (UFlowGraphItem* LockItem = Link->FindItemOfType(EFlowGraphItemType::Lock)) {
					OutConnectionType = ESnapConnectionDoorType::LockedDoor;
					OutCustomMarkerName = LockItem->MarkerName;
				}
				else {
					OutConnectionType = ESnapConnectionDoorType::NormalDoor;
				}
			}
		}
	}
}

