//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Compiler/Impl/DungeonThemeCompilerV2.h"

#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompilerUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

bool FDungeonThemeCompilerV2::Compile(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) {
	OutErrors.Reset();
	if (InThemeAsset) {
		TArray<FDungeonThemeGraphBuildError> CompileErrors;
		CompiledGraph(InThemeAsset, CompileErrors);
		if (CompileErrors.Num() == 0) {
			InThemeAsset->Version = static_cast<int32>(EDungeonThemeAssetVersion::V2_MovedToCompiledData);
			InThemeAsset->Modify();
			return true;
		}
	}
	return false;
}

void FDungeonThemeCompilerV2::CompiledGraph(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) {
	FDungeonThemeCompiledGraph& Graph = InThemeAsset->CompiledThemeGraph;
	Graph = {};

	namespace DTC = DA::ThemeGraph::Compiler;
	
	if (UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(InThemeAsset->UpdateGraph)) {
		// Create the marker node data
		{
			TArray<UEdGraphNode_DungeonThemeMarker*> MarkerEdNodes;
			ThemeGraph->GetNodesOfClass(MarkerEdNodes);
			for (UEdGraphNode_DungeonThemeMarker* MarkerEdNode : MarkerEdNodes) {
				if (MarkerEdNode) {
					FDungeonThemeCompilerUtils::CreateNodeData(MarkerEdNode, Graph.MarkerNodes.AddDefaulted_GetRef());
				}
			}
		}

		// Create the visual node data
		{
			TArray<UEdGraphNode_DungeonThemeActorBase*> VisualEdNodes;
			ThemeGraph->GetNodesOfClass(VisualEdNodes);
			for (UEdGraphNode_DungeonThemeActorBase* VisualEdNode : VisualEdNodes) {
				if (VisualEdNode) {
					FDungeonThemeCompilerUtils::CreateNodeData(InThemeAsset, VisualEdNode, Graph.VisualNodes.AddDefaulted_GetRef());
				}
			}
		}

		// Create the marker emitter node data
		{
			TArray<UEdGraphNode_DungeonThemeMarkerEmitter*> MarkerEmitterEdNodes;
			ThemeGraph->GetNodesOfClass(MarkerEmitterEdNodes);
			for (UEdGraphNode_DungeonThemeMarkerEmitter* MarkerEmitterEdNode : MarkerEmitterEdNodes) {
				if (MarkerEmitterEdNode) {
					FDungeonThemeCompilerUtils::CreateNodeData(MarkerEmitterEdNode, Graph.MarkerEmitterNodes.AddDefaulted_GetRef());
				}
			}
		}
	}
}

