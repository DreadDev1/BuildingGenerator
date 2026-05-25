//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UEdGraphNode_DungeonThemeActorBase;
struct FDungeonThemeVisualNodeData;
struct FDungeonMarkerInstance;
struct FDungeonThemeEngineEventHandlers;
struct FDungeonMarkerBuildData;
struct FRandomStream;
class UDungeonThemeAsset;
class ADungeon;

class DUNGEONARCHITECTRUNTIME_API FDungeonThemeEngineUtils {
public:
	
	static FName CreateNodeTagFromId(const FName& NodeId) {
		return *FString("NODE-").Append(NodeId.ToString());
	}    
	static FName CreateNodeTagFromId(const FGuid& NodeGuid) {
		const FName NodeId = FName(NodeGuid.ToString());
		return *FString("NODE-").Append(NodeId.ToString());
	}

	static FGuid ParseNodeGuid(const FName& InNodeTag);

	/** Execute a theme node and create the spawn data if we inserted this node into the sceen
	 * @return True if the node was inserted
	 */
	static bool ProcessThemeNodeInsertion(const FDungeonThemeVisualNodeData& ThemeNode, const FTransform& InParentOffset,
	                                      const FDungeonThemeEngineEventHandlers& EventHandlers, const FDungeonMarkerInstance& MarkerInstance,
	                                      const FRandomStream& InRandom, int32 MarkerIdx, const ADungeon* InDungeon, FDungeonMarkerBuildData& OutNodeSpawnInfo);
};
