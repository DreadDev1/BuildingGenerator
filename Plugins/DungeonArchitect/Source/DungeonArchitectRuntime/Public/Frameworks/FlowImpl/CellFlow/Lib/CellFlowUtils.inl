#pragma once
#include "CellFlowLib.h"

template <typename TMarkerSetup, typename TMarkerSetupList>
void FCellFlowUtils::GenerateGroupNodeMarkerSetup(const TMarkerSetupList& DefaultMarkerSetup, const TMap<FString, TMarkerSetupList>& PathMarkerSetup, const TArray<FDAFlowCellGroupNode>& InGroupNodes,
				const FFlowAbstractGraphQuery& InGraphQuery, const FRandomStream& InRandom, TMap<int32, TMarkerSetup>& OutGroupMarkers) {
	auto GetChunkMarkers = [&](int GroupIdx) -> TMarkerSetup {
		TMarkerSetup MarkerSetup;
		const FDAFlowCellGroupNode& GroupNode = InGroupNodes[GroupIdx];
		if (GroupNode.IsActive()) {
			const UFlowAbstractNode* LayoutNode = InGraphQuery.GetNode(GroupNode.LayoutNodeID);
			if (const TMarkerSetupList* PathMarkerListPtr = PathMarkerSetup.Find(LayoutNode->PathName)) {
				if (GetRandomMarkerSetup(InRandom, PathMarkerListPtr->MarkerSetupList, MarkerSetup)) {
					return MarkerSetup;
				}
			}
		}
		
		if (GetRandomMarkerSetup(InRandom, DefaultMarkerSetup.MarkerSetupList, MarkerSetup)) {
			return MarkerSetup;
		}
		return TMarkerSetup{};
	};
	
	OutGroupMarkers.Reset();
	for (int GroupIdx = 0; GroupIdx < InGroupNodes.Num(); GroupIdx++) {
		const FDAFlowCellGroupNode& GroupNode = InGroupNodes[GroupIdx];
		TMarkerSetup& MarkerSetupRef = OutGroupMarkers.FindOrAdd(GroupNode.GroupId);
		MarkerSetupRef = GetChunkMarkers(GroupNode.GroupId);
	}
}


template<typename TMarkerSetup>
bool FCellFlowUtils::GetRandomMarkerSetup(const FRandomStream& InRandom, const TArray<TMarkerSetup>& MarkerSetupList, TMarkerSetup& OutMarkerSetup) {
	float TotalSelectionWeight{};
	for (const TMarkerSetup& MarkerList : MarkerSetupList) {
		TotalSelectionWeight += MarkerList.SelectionWeight;
	}

	const int MarkerListIndex = GetWeightedRandomIndex(InRandom, TotalSelectionWeight, MarkerSetupList);
	if (!MarkerSetupList.IsValidIndex(MarkerListIndex)) {
		return false; 
	}

	OutMarkerSetup = MarkerSetupList[MarkerListIndex];
	return true;
}

template<typename TMarkerList>
int32 FCellFlowUtils::GetWeightedRandomIndex(const FRandomStream& InRandom, float TotalSelectionWeight, const TArray<TMarkerList>& MarkerSetupList) {
	if (MarkerSetupList.Num() == 0) {
		return INDEX_NONE;
	}
	
	const float RandomPosition = InRandom.FRand() * TotalSelectionWeight;
	float CurrentPosition{};
	int SelectedIndex{INDEX_NONE};
	for (int i = 0; i < MarkerSetupList.Num(); i++) {
		const TMarkerList& MarkerList = MarkerSetupList[i];
		const float NextPosition = CurrentPosition + MarkerList.SelectionWeight;
		if (RandomPosition >= CurrentPosition && RandomPosition < NextPosition) {
			SelectedIndex = i;
			break;
		}

		CurrentPosition = NextPosition;
	}
	return SelectedIndex;
}

