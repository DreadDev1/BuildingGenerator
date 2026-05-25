//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UEdGraphNode;
class AActor;
class UEdGraph;
class SGridPanel;
class FViewport;
class UEdGraphNode_DungeonThemeBase;
class FDungeonArchitectThemeEditor;
class ADungeon;
class UEdGraphNode_DungeonThemeMarker;
class FDAAssetThumbnailCacheManager;
class UEdGraphNode_DungeonThemeActorBase;
struct FDungeonThemeGraphBuildError;
class UDungeonThemeAsset;

class FDungeonArchitectThemeEditorUtils {
public:
	static void InitializeNewThemeAsset(UDungeonThemeAsset* NewAsset);
	static void InitializeThemeAsset(UDungeonThemeAsset* NewAsset);
	static bool CompileThemeGraph(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors);
	static void SaveThemeAsset(UDungeonThemeAsset* InThemeAsset, FViewport* InViewportForThumbnail = nullptr);
	static bool GetBoundsForSelectedNodes(const UEdGraph* Graph, class FSlateRect& Rect, float Padding = 0.0f);
	static TSharedPtr<SGridPanel> CreateNodeThumbWidget(const UEdGraphNode_DungeonThemeActorBase* InVisualNode, const TSharedPtr<FDAAssetThumbnailCacheManager>& ThumbnailCacheManager);
	static TSharedPtr<SGridPanel> CreateAssetThumbWidget(const TArray<UObject*>& ThumbnailObjects, const TSharedPtr<FDAAssetThumbnailCacheManager>& ThumbnailCacheManager);
	static UEdGraphNode_DungeonThemeActorBase* FindThemeNodeFromSpawnedActor(AActor* InSpawnedActor, UEdGraph* InThemeGraph);
	static UEdGraphNode_DungeonThemeMarker* GetAttachedMarkerNode(const UEdGraphNode_DungeonThemeActorBase* InVisualNode, bool bRecursive = false);
	static UEdGraphNode_DungeonThemeBase* GetParentNode(const UEdGraphNode_DungeonThemeBase* InNode);
	static UEdGraphNode_DungeonThemeActorBase* CreateVisualNodeUnderNode(UObject* InAssetObject,
			const UEdGraphNode_DungeonThemeBase* InParentNode, int32 InInsertIndex = INDEX_NONE);
	static AActor* FindActorSpawnedByMarker(int32 InMarkerId, const ADungeon* InDungeon, const UEdGraphNode_DungeonThemeActorBase* InVisualNode);
	static AActor* FindActorSpawnedByMarker(int32 InMarkerId, const ADungeon* InDungeon, const FName& InNodeTag);
	static UEdGraphNode_DungeonThemeMarker* FindParentMarker(UEdGraphNode* InNode);
	static TArray<UEdGraphNode_DungeonThemeMarker*> FindParentMarkers(UEdGraphNode* InNode);
	static UEdGraphNode_DungeonThemeBase* GetParentNode(UEdGraphNode* GraphNode);
	static void GetNodeTags(const UEdGraphNode_DungeonThemeActorBase* InNode, TArray<FName>& OutTags);
	
private:
	static TSharedPtr<FDungeonArchitectThemeEditor> GetThemeEditorForAsset(const UEdGraph* Graph);
	static void FindParentMarkersRecursive(UEdGraphNode* InNode, TSet<UEdGraphNode*>& Visited, TArray<UEdGraphNode_DungeonThemeMarker*>& OutMarkerNodes);
};

