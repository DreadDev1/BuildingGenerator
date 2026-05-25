//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraph.h"

#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNode.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphSchema.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphEdge.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"

UDungeonForgeEditorGraph::UDungeonForgeEditorGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Schema = UDungeonForgeEditorGraphSchema::StaticClass();
}


void UDungeonForgeEditorGraph::BeginDestroy() {
	Super::BeginDestroy();
	
	OnClose();
}

void UDungeonForgeEditorGraph::InitFromNodeGraph(UDungeonForgeGraph* InForgeGraph) {
	check(InForgeGraph && !ForgeGraph);
	ForgeGraph = InForgeGraph;

	ForgeGraph->OnGraphParametersChangedDelegate.AddUObject(this, &UDungeonForgeEditorGraph::OnGraphUserParametersChanged);

	ReconstructGraph();
}

void UDungeonForgeEditorGraph::ReconstructGraph() {
	check(ForgeGraph);

	// If there are already some nodes, remove all of them.
	if (!Nodes.IsEmpty())
	{
		Modify();

		TArray<TObjectPtr<class UEdGraphNode>> NodesCopy = Nodes;
		for (UEdGraphNode* Node : NodesCopy)
		{
			RemoveNode(Node);
		}
	}

	TMap<UDungeonForgeGraphNode*, UDungeonForgeEditorGraphNodeBase*> NodeLookup;
	const bool bSelectNewNode = false;

	/*
	UDungeonForgeGraphNode* InputNode = ForgeGraph->GetInputNode();
	FGraphNodeCreator<UDungeonForgeEditorGraphNodeInput> InputNodeCreator(*this);
	UDungeonForgeEditorGraphNodeInput* InputGraphNode = InputNodeCreator.CreateNode(bSelectNewNode);
	InputGraphNode->Construct(InputNode);
	InputNodeCreator.Finalize();
	NodeLookup.Add(InputNode, InputGraphNode);

	UDungeonForgeGraphNode* OutputNode = ForgeGraph->GetOutputNode();
	FGraphNodeCreator<UDungeonForgeEditorGraphNodeOutput> OutputNodeCreator(*this);
	UDungeonForgeEditorGraphNodeOutput* OutputGraphNode = OutputNodeCreator.CreateNode(bSelectNewNode);
	OutputGraphNode->Construct(OutputNode);
	OutputNodeCreator.Finalize();
	NodeLookup.Add(OutputNode, OutputGraphNode);
	*/

	for (UDungeonForgeGraphNode* DungeonForgeNode : ForgeGraph->GetNodes())
	{
		if (!IsValid(DungeonForgeNode))
		{
			continue;
		}

		/*
		// TODO: replace this with a templated lambda because this is all very similar
		if (Cast<UDungeonForgeNamedRerouteDeclarationSettings>(DungeonForgeNode->GetSettings()))
		{
			FGraphNodeCreator<UDungeonForgeEditorGraphNodeNamedRerouteDeclaration> NodeCreator(*this);
			UDungeonForgeEditorGraphNodeNamedRerouteDeclaration* RerouteGraphNode = NodeCreator.CreateNode(bSelectNewNode);
			RerouteGraphNode->Construct(DungeonForgeNode);
			NodeCreator.Finalize();
			NodeLookup.Add(DungeonForgeNode, RerouteGraphNode);
		}
		else if (Cast<UDungeonForgeNamedRerouteUsageSettings>(DungeonForgeNode->GetSettings()))
		{
			FGraphNodeCreator<UDungeonForgeEditorGraphNodeNamedRerouteUsage> NodeCreator(*this);
			UDungeonForgeEditorGraphNodeNamedRerouteUsage* RerouteGraphNode = NodeCreator.CreateNode(bSelectNewNode);
			RerouteGraphNode->Construct(DungeonForgeNode);
			NodeCreator.Finalize();
			NodeLookup.Add(DungeonForgeNode, RerouteGraphNode);
		}
		else if (Cast<UDungeonForgeRerouteSettings>(DungeonForgeNode->GetSettings()))
		{
			FGraphNodeCreator<UDungeonForgeEditorGraphNodeReroute> NodeCreator(*this);
			UDungeonForgeEditorGraphNodeReroute* RerouteGraphNode = NodeCreator.CreateNode(bSelectNewNode);
			RerouteGraphNode->Construct(DungeonForgeNode);
			NodeCreator.Finalize();
			NodeLookup.Add(DungeonForgeNode, RerouteGraphNode);
		}
		else
		*/
		
		{
			FGraphNodeCreator<UDungeonForgeEditorGraphNode> NodeCreator(*this);
			UDungeonForgeEditorGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->Construct(DungeonForgeNode);
			NodeCreator.Finalize();
			NodeLookup.Add(DungeonForgeNode, GraphNode);
		}
	}

	for (const auto& NodeLookupIt : NodeLookup)
	{
		UDungeonForgeGraphNode* DungeonForgeNode = NodeLookupIt.Key;
		UDungeonForgeEditorGraphNodeBase* GraphNode = NodeLookupIt.Value;
		CreateLinks(GraphNode, /*bCreateInbound=*/false, /*bCreateOutbound=*/true, NodeLookup);
	}

	for (const UObject* ExtraNode : ForgeGraph->GetExtraEditorNodes())
	{
		if (const UEdGraphNode* ExtraGraphNode = Cast<UEdGraphNode>(ExtraNode))
		{
			UEdGraphNode* NewNode = DuplicateObject(ExtraGraphNode, /*Outer=*/this);
			const bool bIsUserAction = false;
			AddNode(NewNode, bIsUserAction, bSelectNewNode);
		}
	}

	// Ensure graph structure visualization is nice and fresh upon opening.
	//UpdateStructuralVisualization(nullptr, nullptr);
}

void UDungeonForgeEditorGraph::OnClose() {
	if (ForgeGraph) {
		ForgeGraph->OnGraphParametersChangedDelegate.RemoveAll(this);
	}
}

void UDungeonForgeEditorGraph::CreateLinks(UDungeonForgeEditorGraphNodeBase* GraphNode, bool bCreateInbound, bool bCreateOutbound,
		const TMap<UDungeonForgeGraphNode*, UDungeonForgeEditorGraphNodeBase*>& InForgeNodeToForgeEditorNodeMap)
{
	check(GraphNode);
	const UDungeonForgeGraphNode* ForgeNode = GraphNode->GetForgeNode();
	check(ForgeNode);

	if (bCreateInbound)
	{
		for (UDungeonForgeGraphPin* InputPin : ForgeNode->GetInputPins())
		{
			if (!InputPin || InputPin->Properties.bInvisiblePin)
			{
				continue;
			}

			UEdGraphPin* InPin = GraphNode->FindPin(InputPin->Properties.Label, EEdGraphPinDirection::EGPD_Input);
			if (!InPin)
			{
				UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid InputPin for %s"), *InputPin->Properties.Label.ToString());
				ensure(false);
				continue;
			}

			for (const UDungeonForgeGraphEdge* InboundEdge : InputPin->Edges)
			{
				if (!InboundEdge || !InboundEdge->IsValid())
				{
					UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid inbound edge for %s"), *InputPin->Properties.Label.ToString());
					ensure(false);
					continue;
				}

				const UDungeonForgeGraphNode* InboundNode = InboundEdge->InputPin ? InboundEdge->InputPin->Node : nullptr;
				if (!ensure(InboundNode))
				{
					UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid inbound node for %s"), *InputPin->Properties.Label.ToString());
					continue;
				}

				UDungeonForgeEditorGraphNodeBase* const* ConnectedGraphNode = InboundNode ? InForgeNodeToForgeEditorNodeMap.Find(InboundNode) : nullptr;
				UEdGraphPin* OutPin = ConnectedGraphNode ? (*ConnectedGraphNode)->FindPin(InboundEdge->InputPin->Properties.Label, EEdGraphPinDirection::EGPD_Output) : nullptr;
				if (OutPin)
				{
					OutPin->MakeLinkTo(InPin);
				}
				else
				{
					UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Could not create link to InputPin %s from Node %s"), *InputPin->Properties.Label.ToString(), *InboundNode->GetFName().ToString());
					ensure(false);
				}
			}
		}
	}

	if (bCreateOutbound)
	{
		for (UDungeonForgeGraphPin* OutputPin : ForgeNode->GetOutputPins())
		{
			if (!OutputPin || OutputPin->Properties.bInvisiblePin)
			{
				continue;
			}

			UEdGraphPin* OutPin = GraphNode->FindPin(OutputPin->Properties.Label, EEdGraphPinDirection::EGPD_Output);
			if (!OutPin)
			{
				UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid OutputPin for %s"), *OutputPin->Properties.Label.ToString());
				ensure(false);
				continue;
			}

			for (const UDungeonForgeGraphEdge* OutboundEdge : OutputPin->Edges)
			{
				if (!OutboundEdge || !OutboundEdge->IsValid())
				{
					UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid outbound edge for %s"), *OutputPin->Properties.Label.ToString());
					ensure(false);
					continue;
				}

				const UDungeonForgeGraphNode* OutboundNode = OutboundEdge->OutputPin ? OutboundEdge->OutputPin->Node : nullptr;
				if (!ensure(OutboundNode))
				{
					UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid outbound node for %s"), *OutputPin->Properties.Label.ToString());
					continue;
				}

				UDungeonForgeEditorGraphNodeBase* const* ConnectedGraphNode = OutboundNode ? InForgeNodeToForgeEditorNodeMap.Find(OutboundNode) : nullptr;
				UEdGraphPin* InPin = ConnectedGraphNode ? (*ConnectedGraphNode)->FindPin(OutboundEdge->OutputPin->Properties.Label, EEdGraphPinDirection::EGPD_Input) : nullptr;
				if (InPin)
				{
					OutPin->MakeLinkTo(InPin);
				}
				else
				{
					UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Could not create link from OutputPin %s to Node %s"), *OutputPin->Properties.Label.ToString(), *OutboundNode->GetFName().ToString());
					ensure(false);
				}
			}
		}
	}
}

void UDungeonForgeEditorGraph::OnGraphUserParametersChanged(UDungeonForgeGraphInterface* InGraph, EDungeonForgeGraphParameterEvent ChangeType, FName ChangedPropertyName) {
	if ((ChangeType != EDungeonForgeGraphParameterEvent::RemovedUnused && ChangeType != EDungeonForgeGraphParameterEvent::RemovedUsed) || InGraph != ForgeGraph)
	{
		return;
	}

	// If a parameter was removed, just look for getter nodes that do exists in the editor graph, but not in the DungeonForge graph.
	TArray<UDungeonForgeEditorGraphNodeBase*> NodesToRemove;
	for (UEdGraphNode* EditorNode : Nodes)
	{
		if (UDungeonForgeEditorGraphNodeBase* ForgeEditorNode = Cast<UDungeonForgeEditorGraphNodeBase>(EditorNode))
		{
			if (UDungeonForgeGraphNode* ForgeNode = ForgeEditorNode->GetForgeNode())
			{
				/*
				if (UDungeonForgeUserParameterGetSettings* Settings = Cast<UDungeonForgeUserParameterGetSettings>(ForgeNode->GetSettings()))
				{
					if (!ForgeGraph->Contains(ForgeNode))
					{
						NodesToRemove.Add(ForgeEditorNode);
					}
				}
				*/
			}
		}
	}

	if (NodesToRemove.IsEmpty())
	{
		return;
	}

	Modify();

	for (UDungeonForgeEditorGraphNodeBase* NodeToRemove : NodesToRemove)
	{
		NodeToRemove->DestroyNode();
	}
}

