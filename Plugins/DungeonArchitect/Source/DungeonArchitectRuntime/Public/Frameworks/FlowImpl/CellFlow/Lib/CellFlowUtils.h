//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"

struct FDAFlowCellGroupNode;
struct FCellFlowGridMarkerSetup;
struct FCellFlowGridEdgeInfo;

class FCellFlowUtils {
public:
	static void GetEdgeEndPoints(const FCellFlowGridEdgeInfo& InEdge, const FCellFlowGridEdgeInfo& InEdgeTwin, FIntPoint& OutEdgeCoordSrc, FIntPoint& OutEdgeCoordDst);
	
	template<typename TMarkerSetup>
	static bool GetRandomMarkerSetup(const FRandomStream& InRandom, const TArray<TMarkerSetup>& MarkerSetupList, TMarkerSetup& OutMarkerSetup);
	
	template<typename TMarkerList>
	static int32 GetWeightedRandomIndex(const FRandomStream& InRandom, float TotalSelectionWeight, const TArray<TMarkerList>& MarkerSetupList);
	
	template <typename TMarkerSetup, typename TMarkerSetupList>
	static void GenerateGroupNodeMarkerSetup(const TMarkerSetupList& DefaultMarkerSetup, const TMap<FString, TMarkerSetupList>& PathMarkerSetup, const TArray<FDAFlowCellGroupNode>& InGroupNodes,
			const FFlowAbstractGraphQuery& InGraphQuery, const FRandomStream& InRandom, TMap<int32, TMarkerSetup>& OutGroupMarkers);
};

#include "CellFlowUtils.inl"

