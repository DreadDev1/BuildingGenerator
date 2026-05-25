//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Compiler/Impl/DungeonThemeCompilerV1.h"

#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompilerUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/ProceduralDungeonTransformLogic.h"

#include "EdGraph/EdGraph.h"

class UEdGraphNode_DungeonThemeMarkerEmitter;

bool FDungeonThemeCompilerV1::Compile(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) {
	OutErrors.Reset();
	if (InThemeAsset) {
		TArray<FDungeonThemeVisualNodeData> Props;
		TArray<FDungeonThemeGraphBuildError> CompileErrors;
		RebuildGraph(InThemeAsset, Props, CompileErrors);
		if (CompileErrors.Num() == 0) {
			InThemeAsset->Props_DEPRECATED = Props;
			InThemeAsset->Version = static_cast<int32>(EDungeonThemeAssetVersion::V1_Initial);
			InThemeAsset->Modify();
			return true;
		}
	}
	return false;
}

void FDungeonThemeCompilerV1::RebuildGraph(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeVisualNodeData>& OutProps,
		TArray<FDungeonThemeGraphBuildError>& OutErrors)
{
	namespace DTC = DA::ThemeGraph::Compiler;
	const UEdGraph_DungeonTheme* DungeonGraph = InThemeAsset ? Cast<UEdGraph_DungeonTheme>(InThemeAsset->UpdateGraph) : nullptr;
	if (!DungeonGraph || !InThemeAsset) {
		return;
	}
	
    OutProps.Reset();
    // TODO: Check for cycles

    TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
    DungeonGraph->GetNodesOfClass<UEdGraphNode_DungeonThemeMarker>(MarkerNodes);
    for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerNodes) {
        TArray<UEdGraphNode_DungeonThemeActorBase*> ActorNodes;
        DTC::GetChildNodes<UEdGraphNode_DungeonThemeActorBase>(MarkerNode, ActorNodes);
        ActorNodes.Sort(DTC::ExecutionSortComparer());
        for (UEdGraphNode_DungeonThemeActorBase* ActorNode : ActorNodes) {
            FDungeonThemeVisualNodeData& NodeData = OutProps.AddDefaulted_GetRef();
        	FDungeonThemeCompilerUtils::CreateNodeData(InThemeAsset, ActorNode, NodeData);
        	
        	// Add deprecated data
			NodeData.NodeId_DEPRECATED = FName(*ActorNode->NodeGuid.ToString());
        	NodeData.MarkerName_DEPRECATED = MarkerNode->MarkerName;
        	
            // Insert Child Marker emitters
            TArray<UEdGraphNode_DungeonThemeMarkerEmitter*> EmitterNodes;
            DTC::GetChildNodes<UEdGraphNode_DungeonThemeMarkerEmitter>(ActorNode, EmitterNodes);
            for (UEdGraphNode_DungeonThemeMarkerEmitter* EmitterNode : EmitterNodes) {
                //if (!EmitterNode || !EmitterNode->IsValidLowLevel()) continue;
                if (EmitterNode && EmitterNode->ParentMarker) {
                    FDungeonThemeMarkerEmitterNodeData MarkerEmitter;
                    MarkerEmitter.MarkerName_DEPRECATED = EmitterNode->ParentMarker->MarkerName;
                    MarkerEmitter.Offset = EmitterNode->Offset;
                    NodeData.MarkerEmitters_DEPRECATED.Add(MarkerEmitter);
                }
            }
        }
    }
}

