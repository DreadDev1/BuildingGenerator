//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphConstraints.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/Lib/FlowAbstractGraphPathUtils.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleDatabase.h"
#include "SnapGridFlowModuleResolver.generated.h"

class USnapGridFlowModuleSelectionRule;
class UFlowAbstractGraphBase;
struct FSGFModuleAssemblySideCell;
struct FFlowAGPathNodeGroup;
struct FFAGConstraintsLink;

namespace SnapLib {
    typedef TSharedPtr<struct FModuleNode> FModuleNodePtr;
	typedef TSharedPtr<class IModuleDatabase> IModuleDatabasePtr;
	typedef TSharedPtr<class IModuleDatabaseItem> IModuleDatabaseItemPtr;
}

struct FSnapGridFlowModuleResolverSettings {
	int32 Seed = 0;
	FVector ChunkSize = FVector::ZeroVector;
	FTransform BaseTransform = FTransform::Identity;
	float ModulesWithMinimumDoorsProbability = 1.0f;
	int32 MaxResolveFrames = 10000;
	int32 MaxItemFitnessFrames = 10000;
	int32 NonRepeatingRooms = 3;
	TArray<TObjectPtr<USnapGridFlowModuleSelectionRule>> ModuleResolveRules;
};


typedef TSharedPtr<class FSnapGridFlowModuleDatabaseImpl> FSnapGridFlowModuleDatabaseImplPtr;

class DUNGEONARCHITECTRUNTIME_API FSnapGridFlowModuleResolver {
public:
	FSnapGridFlowModuleResolver(const FSnapGridFlowModuleDatabaseImplPtr& InModuleDatabase, const FSnapGridFlowModuleResolverSettings& InSettings)
		: ModuleDatabase(InModuleDatabase)
		, Settings(InSettings)
	{}

	bool Resolve(UFlowAbstractGraphBase* InGraph, TArray<SnapLib::FModuleNodePtr>& OutModuleNodes) const;

private:
	struct FSGFResolveNodeGroupData {
		FFlowAGPathNodeGroup Group;
		TArray<FFAGConstraintsLink> ConstraintLinks;
	}; 

	struct FSGFModuleResolveState {
		FSGFModuleResolveState(UFlowAbstractGraphBase* InGraph, int32 InSeed)
			: GraphQuery(InGraph)
			, Random(FRandomStream(InSeed))
		{
		}
		
		FFlowAbstractGraphQuery GraphQuery;
		FRandomStream Random;
		TMap<FGuid, SnapLib::FModuleNodePtr> ModuleNodesById;
		TMap<FGuid, TArray<FSGFModuleAssemblySideCell>> ActiveModuleDoorIndices;
		TMap<FGuid, FIntVector> ModuleCoordsById;
		TMap<UFlowAbstractNode*, FSGFResolveNodeGroupData> NodeGroups;
		TMap<SnapLib::IModuleDatabaseItemPtr, TArray<int>> ModuleLastUsedDepth;
		int FrameIndex = 0;
	};
	
	struct FModuleFitCandidate {
		TSharedPtr<class FSnapGridFlowGraphModDBItemImpl> ModuleItem;
		FQuat ModuleRotation;
		int32 AssemblyIndex;
		TArray<FSGFModuleAssemblySideCell> DoorIndices;
		int32 ItemFitness = 0;
		int32 ConnectionWeight = 0;
		int32 ModuleLastUsedDepth = MAX_int32;
		float ModuleWeight = 0;
		uint64 FinalPriority = 0LL;
	};
	
private:
	bool ResolveNodes(FSGFModuleResolveState& InResolveState) const;
	bool ResolveNodes_Linear(FSGFModuleResolveState& InResolveState) const;
	bool ResolveNode_Linear(FSGFModuleResolveState& InResolveState, UFlowAbstractNode* InNode) const;
	
	bool ResolveNodes_Recursive(FSGFModuleResolveState& InResolveState) const;
	bool ResolveNode_Recursive(FSGFModuleResolveState& InResolveState, UFlowAbstractNode* InNode, int32 InDepth, TSet<UFlowAbstractNode*> InVisited) const;
	
	void GetCandidates(FSGFModuleResolveState& InResolveState, UFlowAbstractNode* InNode, int32 InDepth, const struct FSGFModuleAssembly& InAssembly, TArray<FModuleFitCandidate>& OutCandidates) const;
	void RegisterNodeModule(UFlowAbstractNode* InNode, FSGFModuleResolveState& InResolveState, const FModuleFitCandidate& InCandidate) const;
	
	static void DeregisterNodeModule(UFlowAbstractNode* InNode, FSGFModuleResolveState& InResolveState);
	static int32 GetModuleLastUsedDepth(FSGFModuleResolveState& InResolveState, TSharedPtr<FSnapGridFlowGraphModDBItemImpl> InModuleItem, int32 InCurrentDepth, int32 InMaxNonRepeatingDepth);
	static void PushModuleLastUsedDepth(FSGFModuleResolveState& InResolveState, TSharedPtr<FSnapGridFlowGraphModDBItemImpl> InModuleItem, int32 InDepth);
	static void PopModuleLastUsedDepth(FSGFModuleResolveState& InResolveState, TSharedPtr<FSnapGridFlowGraphModDBItemImpl> InModuleItem);
	
private:
	FSnapGridFlowModuleDatabaseImplPtr ModuleDatabase;
	FSnapGridFlowModuleResolverSettings Settings;
};



//////////////////////////////////////// Snap Grid Flow Module Selection Rule //////////////////////////////////////////////
/**
 * Decide whether a module is allowed in this place. This happens in the last stage of the SGF generation process,
 * allowing you to filter out modules based on the node configuration
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, HideDropdown)
class DUNGEONARCHITECTRUNTIME_API USnapGridFlowModuleSelectionRule : public UObject {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
	bool CanSelect(UFlowAbstractNode* Node, const TArray<UFlowAbstractNode*>& ExistingNodes, const FName& Category, const TArray<FName>& Tags, int32 RotationIndex);
};

