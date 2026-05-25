//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Graph/DungeonThemeCompiledGraphUpgrader.h"

#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeGraphNodeData.h"

void FDungeonThemeCompiledGraphUpgrader::Upgrade(UDungeonThemeAsset* InThemeAsset) {
	if (!InThemeAsset || InThemeAsset->IsLatestVersion()) {
		return;
	}
    
	if (InThemeAsset->Version <= static_cast<int32>(EDungeonThemeAssetVersion::V1_Initial)) {
		UpgradeToVersion2_MovedToCompiledData(InThemeAsset);
	}

	check(InThemeAsset->Version == static_cast<int32>(EDungeonThemeAssetVersion::LatestVersion));
}

void FDungeonThemeCompiledGraphUpgrader::UpgradeToVersion2_MovedToCompiledData(UDungeonThemeAsset* InThemeAsset) {
	check(InThemeAsset->Version <= static_cast<int32>(EDungeonThemeAssetVersion::V1_Initial));

	FDungeonThemeCompiledGraph& CompiledGraph = InThemeAsset->CompiledThemeGraph;
	CompiledGraph = {};
	
	// Upgrade the asset
	CompiledGraph.VisualNodes = InThemeAsset->Props_DEPRECATED;
	TMap<FString, FDungeonThemeMarkerNodeData> MarkerNodeRegistry;
	float GlobalExecOrder = 0;
	for (FDungeonThemeVisualNodeData& VisualNodeData : CompiledGraph.VisualNodes) {
		FGuid::Parse(VisualNodeData.NodeId_DEPRECATED.ToString(), VisualNodeData.NodeGuid);
		VisualNodeData.ExecutionOrder = GlobalExecOrder++; 
		VisualNodeData.ParentNodes = {};
		
		// Setup the marker node
		{
			const FString& MarkerName = VisualNodeData.MarkerName_DEPRECATED;
			if (!MarkerNodeRegistry.Contains(MarkerName)) {
				FDungeonThemeMarkerNodeData& MarkerNode = MarkerNodeRegistry.FindOrAdd(MarkerName);
				MarkerNode.MarkerName = MarkerName;
				MarkerNode.NodeGuid = FGuid::NewGuid();
			}
		
			FDungeonThemeMarkerNodeData& MarkerNode = MarkerNodeRegistry[MarkerName];
			VisualNodeData.ParentNodes.Add(MarkerNode.NodeGuid);
		}

		// Setup the marker emitter nodes
		int32 EmitterExecOrder = 0;
		TArray<FDungeonThemeMarkerEmitterNodeData> NewEmitters = VisualNodeData.MarkerEmitters_DEPRECATED;
		for (FDungeonThemeMarkerEmitterNodeData& NewEmitter : NewEmitters) {
			NewEmitter.NodeGuid = FGuid::NewGuid();
			NewEmitter.ExecutionOrder = EmitterExecOrder++;
			if (FDungeonThemeMarkerNodeData* ParentMarkerNodeData = MarkerNodeRegistry.Find(NewEmitter.MarkerName_DEPRECATED)) {
				NewEmitter.MarkerNodeGuid = ParentMarkerNodeData->NodeGuid;
			}
			NewEmitter.ParentNodes.Add(VisualNodeData.NodeGuid);
		}
		CompiledGraph.MarkerEmitterNodes.Append(NewEmitters);
	}

	// save the generated marker nodes
	MarkerNodeRegistry.GenerateValueArray(CompiledGraph.MarkerNodes);

	// Setup the marker emitter node guids
	for (FDungeonThemeMarkerEmitterNodeData& EmitterData : CompiledGraph.MarkerEmitterNodes) {
		if (FDungeonThemeMarkerNodeData* MarkerNodeData = MarkerNodeRegistry.Find(EmitterData.MarkerName_DEPRECATED)) {
			EmitterData.MarkerNodeGuid = MarkerNodeData->NodeGuid; 
		}
	}
	
	InThemeAsset->Version = static_cast<int32>(EDungeonThemeAssetVersion::V2_MovedToCompiledData);
	InThemeAsset->Modify();
}


