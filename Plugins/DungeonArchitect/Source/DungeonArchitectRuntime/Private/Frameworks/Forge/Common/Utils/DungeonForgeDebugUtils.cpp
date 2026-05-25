//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Common/Utils/DungeonForgeDebugUtils.h"

#include "Core/Dungeon.h"
#include "Core/Utils/Debug/DungeonDebugVisualizer.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphExecutor.h"

class UDungeonDebugVisualizer;

void FDungeonForgeDebugUtils::UpdateNodeDebugVisualization(const FDungeonForgeContext* Context) {
#if WITH_EDITOR
	if (!Context || !Context->Node.IsValid()) {
		return;
	}
	
	bool bDebug{};
	if (UDungeonForgeNodeSettingsInterface* NodeSettings = Context->Node->GetSettingsInterface()) {
		bDebug = NodeSettings->bDebug;
	}
	
	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	const FGuid NodeGuid = Context->Node->NodeGuid;
	if (bDebug) {
		TObjectPtr<UDungeonDebugVisualizer>& DebugVisualizer = ExecState->Dungeon->TemporaryDebugVisualizers.FindOrAdd(NodeGuid);
		if (DebugVisualizer) {
			DebugVisualizer->Clear();
		}
		else {
			DebugVisualizer = NewObject<UDungeonDebugVisualizer>(ExecState->Dungeon.Get());
			DebugVisualizer->RegisterComponent();
		}

		check(DebugVisualizer);
					
		// Draw debug data
		for (const FDungeonForgeTaggedData& OutputData : Context->OutputData.TaggedData) {
			if (IsValid(OutputData.Data)) {
				OutputData.Data->WriteDebugData(DebugVisualizer);
			}
		}
	}
	else {
		// Destroy the debug visualizer component, if it exists
		if (ExecState->Dungeon->TemporaryDebugVisualizers.Contains(NodeGuid)) {
			TObjectPtr<UDungeonDebugVisualizer> DebugVisualizer = ExecState->Dungeon->TemporaryDebugVisualizers[NodeGuid];
			ExecState->Dungeon->TemporaryDebugVisualizers.Remove(NodeGuid);
			DebugVisualizer->Clear();
			DebugVisualizer->DestroyComponent();
		}
	}
#endif // WITH_EDITOR
}

