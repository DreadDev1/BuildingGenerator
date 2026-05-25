//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "DungeonForgeEditorGraph.generated.h"

enum class EDungeonForgeGraphParameterEvent;
class FDungeonForgeEditorModeToolkit;
class UDungeonForgeGraph;
class UDungeonForgeGraphInterface;
class UDungeonForgeGraphNode;
class UDungeonForgeEditorGraphNodeBase;

UCLASS()
class UDungeonForgeEditorGraph : public UEdGraph {
	GENERATED_UCLASS_BODY()
public:
	// ~Begin UObject interface
	virtual void BeginDestroy() override;
	// ~End UObject interface
	
	/** Initialize the editor graph from a DungeonForgeGraph */
	void InitFromNodeGraph(UDungeonForgeGraph* InForgeGraph);

	/** If the underlying graph changed without UI interaction, use this function to reconstruct the UI elements. */
	void ReconstructGraph();

	void OnClose();

	TObjectPtr<UDungeonForgeGraph> GetForgeGraph() const { return ForgeGraph; }
	
	void SetEditor(const TWeakPtr<FDungeonForgeEditorModeToolkit>& InEditor) { ForgeEditor = InEditor; }
	TWeakPtr<FDungeonForgeEditorModeToolkit> GetEditor() const { return ForgeEditor; }
	
protected:
	void CreateLinks(UDungeonForgeEditorGraphNodeBase* InGraphNode, bool bCreateInbound, bool bCreateOutbound, const TMap<UDungeonForgeGraphNode*, UDungeonForgeEditorGraphNodeBase*>& InForgeNodeToForgeEditorNodeMap);
	void OnGraphUserParametersChanged(UDungeonForgeGraphInterface* InGraph, EDungeonForgeGraphParameterEvent ChangeType, FName ChangedPropertyName);
	
private:
	UPROPERTY()
	TObjectPtr<UDungeonForgeGraph> ForgeGraph = nullptr;
	
	TWeakPtr<FDungeonForgeEditorModeToolkit> ForgeEditor = nullptr;
};
	

