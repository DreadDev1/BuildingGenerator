//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/Debug/SceneDebugDataComponent.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskScatterProps.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"
#include "CellFlowLib.generated.h"

UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UDAFlowCellLeafNode : public UObject {
	GENERATED_BODY()
public:

	UPROPERTY()
	int CellId = -1;

	UPROPERTY()
	int LogicalZ = 0;
	
	UPROPERTY()
	TSet<int32> AdjacentLeafs;  // Adjacent leaf nodes, might belong to other groups
	 
public:
	virtual float GetArea() const { return 0; }
	virtual FVector2d GetCenter() const { return FVector2D::ZeroVector; }
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGroupNode {
	GENERATED_BODY()

	FDAFlowCellGroupNode()
		: LayoutNodeID(FGuid())
		, GroupColor(FLinearColor::White)
		, PreviewLocation(FVector2D::ZeroVector)
	{	
	}
	
	UPROPERTY()
	int GroupId = -1;

	UPROPERTY()
	FGuid LayoutNodeID;

	UPROPERTY()
	FLinearColor GroupColor;

	UPROPERTY()
	int32 GroupHeight = 1;
	
	UPROPERTY()
	TSet<int> LeafNodes;
	
	UPROPERTY()
	TSet<int> Connections;  // Connections to the other groups

	UPROPERTY()
	FVector2D PreviewLocation;

	FORCEINLINE bool IsActive() const { return LeafNodes.Num() > 0; }
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGraphSpawnInfo {
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Coord = FVector::ZeroVector;
	
	UPROPERTY()
	TWeakObjectPtr<const UFlowGraphItem> Item;
	
	UPROPERTY()
	int32 GroupId = INDEX_NONE;
};


USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGraphGridStairInfo {
	GENERATED_BODY()

	UPROPERTY()
	int EdgeIndex = INDEX_NONE;		// Indexes into the Half Edge list
	
	UPROPERTY()
	float AngleRadians = 0;	// Rotation angle in 90 degree steps

	UPROPERTY()
	FVector Direction = FVector::ZeroVector;		// Leading down the stairs
	
	UPROPERTY()
	FVector LocalLocation = FVector::ZeroVector;		// Leading down the stairs
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGraphGridInfo {
	GENERATED_BODY()

	UPROPERTY()
	TArray<FCellFlowGridEdgeInfo> HalfEdges;

	UPROPERTY()
	TMap<int32, FDAFlowCellGraphGridStairInfo> Stairs;	// maps the edge index to the stair info

	UPROPERTY()
	TArray<FDAFlowCellGraphSpawnInfo> SpawnInfo;
}; 

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGraphDCELEdgeInfo {
	GENERATED_BODY()
	
	UPROPERTY()
	int EdgeIndex = INDEX_NONE;		// Indexes into the DCEL Edges array
	
	UPROPERTY()
	double LogicalWidth = 0;
	
	UPROPERTY()
	FVector LogicalLocation = FVector::ZeroVector; // right below the highest point

	UPROPERTY()
	FVector Direction = FVector::ZeroVector;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGraphDCELInfo {
	GENERATED_BODY()

	UPROPERTY()
	TArray<int> DoorEdges;		// Indexes into the DCEL Edges array
	
	UPROPERTY()
	TMap<int32, FDAFlowCellGraphDCELEdgeInfo> Stairs;	// Maps the EdgeId to the Stair info

	UPROPERTY()
	TMap<int32, FDAFlowCellGraphDCELEdgeInfo> Doors;	// Maps the EdgeId to the Door info

	UPROPERTY()
	TArray<FDAFlowCellGraphSpawnInfo> SpawnInfo;
}; 


USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAFlowCellGraphGlobalSettings {
	GENERATED_BODY()

	UPROPERTY()
	bool bSeparateAdjacentRooms = true;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDAFlowCellGraph : public UObject, public IFlowExecCloneableState {
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<TObjectPtr<UDAFlowCellLeafNode>> LeafNodes;
	
	UPROPERTY()
	TArray<FDAFlowCellGroupNode> GroupNodes;

	UPROPERTY()
	FDAFlowCellGraphGridInfo GridInfo{};
	
	UPROPERTY()
	FDAFlowCellGraphDCELInfo DCELInfo{};

	UPROPERTY()
	TArray<FCellFlowLayoutTaskScatterPropSettings> ScatterSettings;

	UPROPERTY()
	FDAFlowCellGraphGlobalSettings GlobalSettings{};

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bRenderInactiveGroups = true;
#endif // WITH_EDITORONLY_DATA

	static const FName StateTypeID;
public:
	template<typename TNode>
	TNode* CreateLeafNode() {
		TNode* LeafNode = NewObject<TNode>(this);
		LeafNodes.Add(LeafNode);
		return LeafNode;
	}

	UPROPERTY()
	FDASceneDebugData SceneDebugData;
	
	virtual void CloneFromStateObject(const UObject* SourceObject) override;
};

struct FFlowAGGrowthState;
class UCellFlowConfigMarkerSettings;

namespace DA {
	class FCellAreaLookup {
	public:
		void Init(UDAFlowCellGraph* InGraph);
		FORCEINLINE float GetLeafArea(int LeafId) const { return LeafAreas[LeafId]; }
		FORCEINLINE float GetGroupArea(int GroupId) const { return GroupAreas[GroupId]; }
		void SetGroupArea(int GroupId, float NewArea);
		bool GetGroupWithLeastArea(const FRandomStream& InRandom, int& OutGroupId) const;
		
	private:
		TArray<float> LeafAreas;
		TArray<float> GroupAreas;

		TMap<float, TArray<int>> ActiveGroupIdsByArea;
	};
	
	class FCellGraphBuilder {
	public:
		static void CollapseEdges(UDAFlowCellGraph* InGraph, float MinGroupArea, float InClusterBlobbiness, const FRandomStream& Random, FCellAreaLookup& AreaLookup);
		static void AssignGroupColors(UDAFlowCellGraph* InGraph);
		static void AssignGroupPreviewLocations(UDAFlowCellGraph* InGraph, const FCellAreaLookup& InAreaLookup);
		static void GenerateEdgeList(UDAFlowCellGraph* CellGraph, TArray<FCellFlowGridEdgeInfo>& HalfEdges, const FFlowAbstractGraphQuery& LayoutGraphQuery, bool bHandleInactiveGroups);
		
	private:
		static bool CollapsedBestGroupEdge(UDAFlowCellGraph* InGraph, float MinGroupArea, float InClusterBlobbiness, const FRandomStream& Random, FCellAreaLookup& AreaLookup);
		static void MergeGroups(UDAFlowCellGraph* InGraph, int GroupA, int GroupB, FCellAreaLookup& AreaLookup);
	};

	class FCellFlowLib {
	public:
		// Select a random FCellFlowSizedMarkerDef based on selection weights
		static bool SelectRandomWeightedMarker(const TArray<FCellFlowSizedMarkerDef>& InMarkers, const FRandomStream& InRandomStream, FCellFlowSizedMarkerDef& OutRandomMarker);
		static void AssignGroupHeights(UDAFlowCellGraph* CellGraph, const TSet<FGuid>& LayoutNodeIds, int32 CeilingHeightMin, int32 CeilingHeightMax, const FFlowAbstractGraphQuery& LayoutGraphQuery, const FRandomStream& Random);
	};
}

