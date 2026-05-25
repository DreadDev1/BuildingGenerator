//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompilerUtils.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeGraphNodeData.h"
#include "Frameworks/ThemeEngine/Rules/Selector/DungeonSelectorLogic.h"
#include "Frameworks/ThemeEngine/Rules/Selector/GenericDungeonSelectorLogic.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/DungeonTransformLogic.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/ProceduralDungeonTransformLogic.h"

#include "EdGraph/EdGraphPin.h"

void FDungeonThemeCompilerUtils::CreateNodeData(UDungeonThemeAsset* InOwner,
                                                const UEdGraphNode_DungeonThemeActorBase* InActorNode, FDungeonThemeVisualNodeData& OutNodeData)
{
	namespace DTC = DA::ThemeGraph::Compiler;
    
	OutNodeData = {};
	if (!InActorNode) {
		return;
	}

	OutNodeData.NodeGuid = InActorNode->NodeGuid;
	OutNodeData.ExecutionOrder = InActorNode->NodePosX;
	OutNodeData.AssetObject = InActorNode->GetNodeAssetObject(InOwner);
	OutNodeData.Probability = InActorNode->Probability;
	OutNodeData.Offset = InActorNode->Offset;
	OutNodeData.ConsumeOnAttach = InActorNode->ConsumeOnAttach;
	OutNodeData.ActorTags = InActorNode->ActorTags;
	OutNodeData.bAlignToVoxelGeometry = InActorNode->bAlignToVoxelGeometry;

	// Clone the selection logic instances
	DTC::CloneUObjectArray(InOwner, InActorNode->SelectionLogics, OutNodeData.SelectionLogics);
	DTC::CloneUObjectArray(InOwner, InActorNode->GenericSelectionLogics, OutNodeData.GenericSelectionLogics);
	OutNodeData.bUseSelectionLogic = InActorNode->bUseSelectionLogic;
	OutNodeData.bLogicOverridesAffinity = InActorNode->bLogicOverridesAffinity;

	// Clone the transform logic instance
	DTC::CloneUObjectArray(InOwner, InActorNode->TransformLogics, OutNodeData.TransformLogics);
	OutNodeData.bUseTransformLogic = InActorNode->bUseTransformLogic;

	DTC::CloneUObjectArray(InOwner, InActorNode->ProceduralTransformLogics, OutNodeData.ProceduralTransformLogics);
	OutNodeData.bUseProceduralTransformLogic = InActorNode->bUseProceduralTransformLogic;

	// Clone the spawn logic instance
	DTC::CloneUObjectArray(InOwner, InActorNode->SpawnLogics, OutNodeData.SpawnLogics);
	OutNodeData.bUseSpawnLogic = InActorNode->bUseSpawnLogic;

	if (UEdGraphPin* InputPin = InActorNode->GetInputPin()) {
		for (UEdGraphPin* ParentNodePin : InputPin->LinkedTo) {
			if (ParentNodePin) {
				if (UEdGraphNode* ParentNode = ParentNodePin->GetOwningNode()) {
					OutNodeData.ParentNodes.Add(ParentNode->NodeGuid);
				}
			}
		} 
	}
}

void FDungeonThemeCompilerUtils::CreateNodeData(const UEdGraphNode_DungeonThemeMarker* InMarkerNode, FDungeonThemeMarkerNodeData& OutNodeData) {
	OutNodeData = {};
	if (InMarkerNode) {
		OutNodeData.MarkerName =  InMarkerNode->MarkerName;
		OutNodeData.NodeGuid = InMarkerNode->NodeGuid;
	}
}

void FDungeonThemeCompilerUtils::CreateNodeData(const UEdGraphNode_DungeonThemeMarkerEmitter* InMarkerEmitterNode, FDungeonThemeMarkerEmitterNodeData& OutNodeData) {
	OutNodeData = {};
	if (InMarkerEmitterNode) {
		OutNodeData.NodeGuid = InMarkerEmitterNode->NodeGuid;
		OutNodeData.ExecutionOrder = InMarkerEmitterNode->NodePosX;
		OutNodeData.Offset = InMarkerEmitterNode->Offset;
		if (InMarkerEmitterNode->ParentMarker) {
			OutNodeData.MarkerNodeGuid = InMarkerEmitterNode->ParentMarker->NodeGuid;
			OutNodeData.MarkerName_DEPRECATED = InMarkerEmitterNode->ParentMarker->MarkerName; 
		}
		if (UEdGraphPin* InputPin = InMarkerEmitterNode->GetInputPin()) {
			for (UEdGraphPin* ParentNodePin : InputPin->LinkedTo) {
				if (ParentNodePin) {
					if (UEdGraphNode* ParentNode = ParentNodePin->GetOwningNode()) {
						OutNodeData.ParentNodes.Add(ParentNode->NodeGuid);
					}
				}
			} 
		}
	}
}

