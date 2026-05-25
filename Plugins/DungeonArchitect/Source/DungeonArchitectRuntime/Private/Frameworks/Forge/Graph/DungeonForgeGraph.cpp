//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphCompilationData.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTask.h"
#include "Frameworks/Forge/Internal/Compiler/DungeonForgeGraphCompiler.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

namespace DungeonForgeGraphUtils
{
	/** Returns true if the two descriptors are valid and compatible */
	bool ArePropertiesCompatible(const FPropertyBagPropertyDesc* InSourcePropertyDesc, const FPropertyBagPropertyDesc* InTargetPropertyDesc)
	{
		return InSourcePropertyDesc && InTargetPropertyDesc && InSourcePropertyDesc->CompatibleType(*InTargetPropertyDesc);
	}

	/** Checks if the value for a source property in a source struct has the same value that the target property in the target struct. */
	bool ArePropertiesIdentical(const FPropertyBagPropertyDesc* InSourcePropertyDesc, const FInstancedPropertyBag& InSourceInstance, const FPropertyBagPropertyDesc* InTargetPropertyDesc, const FInstancedPropertyBag& InTargetInstance)
	{
		if (!InSourceInstance.IsValid() || !InTargetInstance.IsValid() || !InSourcePropertyDesc || !InSourcePropertyDesc->CachedProperty || !InTargetPropertyDesc || !InTargetPropertyDesc->CachedProperty)
		{
			return false;
		}

		if (!InSourcePropertyDesc->CompatibleType(*InTargetPropertyDesc))
		{
			return false;
		}

		const uint8* SourceValueAddress = InSourceInstance.GetValue().GetMemory() + InSourcePropertyDesc->CachedProperty->GetOffset_ForInternal();
		const uint8* TargetValueAddress = InTargetInstance.GetValue().GetMemory() + InTargetPropertyDesc->CachedProperty->GetOffset_ForInternal();

		return InSourcePropertyDesc->CachedProperty->Identical(SourceValueAddress, TargetValueAddress);
	}

	/** Copy the value for a source property in a source struct to the target property in the target struct. */
	void CopyPropertyValue(const FPropertyBagPropertyDesc* InSourcePropertyDesc, const FInstancedPropertyBag& InSourceInstance, const FPropertyBagPropertyDesc* InTargetPropertyDesc, FInstancedPropertyBag& InTargetInstance)
	{
		if (!InSourceInstance.IsValid() || !InTargetInstance.IsValid() || !InSourcePropertyDesc || !InSourcePropertyDesc->CachedProperty || !InTargetPropertyDesc || !InTargetPropertyDesc->CachedProperty)
		{
			return;
		}

		// Can't copy if they are not compatible.
		if (!InSourcePropertyDesc->CompatibleType(*InTargetPropertyDesc))
		{
			return;
		}

		const uint8* SourceValueAddress = InSourceInstance.GetValue().GetMemory() + InSourcePropertyDesc->CachedProperty->GetOffset_ForInternal();
		uint8* TargetValueAddress = InTargetInstance.GetMutableValue().GetMemory() + InTargetPropertyDesc->CachedProperty->GetOffset_ForInternal();

		InSourcePropertyDesc->CachedProperty->CopyCompleteValue(TargetValueAddress, SourceValueAddress);
	}

	EDungeonForgeGraphChangeType NotifyTouchedNodes(const TSet<UDungeonForgeGraphNode*>& InTouchedNodes, EDungeonForgeGraphChangeType InChangeType)
	{
		EDungeonForgeGraphChangeType FinalChangeType = EDungeonForgeGraphChangeType::None;

		// Build a final list of all touched nodes, so we can broadcast the change once below.
		TSet<UDungeonForgeGraphNode*> FinalTouchedNodes = InTouchedNodes;

		for (UDungeonForgeGraphNode* TouchedNode : InTouchedNodes)
		{
			if (TouchedNode)
			{
				const EDungeonForgeGraphChangeType NodeChangeType = InChangeType | TouchedNode->PropagateDynamicPinTypes(FinalTouchedNodes);

				FinalChangeType |= NodeChangeType;
			}
		}

		// Do change notifications for the final set.
#if WITH_EDITOR
		for (UDungeonForgeGraphNode* TouchedNode : FinalTouchedNodes)
		{
			check(TouchedNode);
			TouchedNode->OnNodeChangedDelegate.Broadcast(TouchedNode, EDungeonForgeGraphChangeType::Node | InChangeType);
		}
#endif

		return FinalChangeType;
	}
}


EPropertyBagResult UDungeonForgeGraphInterface::SetGraphParameter(const FName PropertyName, const uint64 Value, const UEnum* Enum)
{
	FInstancedPropertyBag* UserParameters = GetMutableUserParametersStruct();
	check(UserParameters);

	const EPropertyBagResult Result = FDungeonForgeGraphParameterExtension::SetGraphParameter(*UserParameters, PropertyName, Value, Enum);
	if (Result == EPropertyBagResult::Success) {
		OnGraphParametersChanged(EDungeonForgeGraphParameterEvent::ValueModifiedLocally, PropertyName);
	}
	return Result;
}

bool UDungeonForgeGraphInterface::UpdateArrayGraphParameter(const FName PropertyName, TFunctionRef<bool(FPropertyBagArrayRef& PropertyBagArrayRef)> Callback)
{
	FInstancedPropertyBag* UserParameters = GetMutableUserParametersStruct();
	check(UserParameters);

	TValueOrError<FPropertyBagArrayRef, EPropertyBagResult> Result = UserParameters->GetMutableArrayRef(PropertyName);

	if (!Result.HasError() && Result.HasValue() && Callback(Result.GetValue()))
	{
		OnGraphParametersChanged(EDungeonForgeGraphParameterEvent::ValueModifiedLocally, PropertyName);
		return true;
	}
	else
	{
		return false;
	}
}

bool UDungeonForgeGraphInterface::UpdateSetGraphParameter(const FName PropertyName, TFunctionRef<bool(FPropertyBagSetRef& PropertyBagSetRef)> Callback)
{
	FInstancedPropertyBag* UserParameters = GetMutableUserParametersStruct();
	check(UserParameters);

	TValueOrError<FPropertyBagSetRef, EPropertyBagResult> Result = UserParameters->GetMutableSetRef(PropertyName);

	if (!Result.HasError() && Result.HasValue() && Callback(Result.GetValue()))
	{
		OnGraphParametersChanged(EDungeonForgeGraphParameterEvent::ValueModifiedLocally, PropertyName);
		return true;
	}
	else
	{
		return false;
	}
}

bool UDungeonForgeGraphInterface::IsInstance() const
{
	return this != GetGraph();
}

bool UDungeonForgeGraphInterface::IsEquivalent(const UDungeonForgeGraphInterface* Other) const
{
	if (this == Other)
	{
		return true;
	}

	const UDungeonForgeGraph* OtherGraph = Other ? Other->GetGraph() : nullptr;
	const UDungeonForgeGraph* ThisGraph = GetGraph();

	if (ThisGraph != OtherGraph)
	{
		return false;
	}
	else if (!ThisGraph && !OtherGraph)
	{
		return true;
	}

	const FInstancedPropertyBag* OtherParameters = Other->GetUserParametersStruct();
	const FInstancedPropertyBag* ThisParameters = this->GetUserParametersStruct();
	check(OtherParameters && ThisParameters);

	if (ThisParameters->GetNumPropertiesInBag() != OtherParameters->GetNumPropertiesInBag())
	{
		return false;
	}

	const UPropertyBag* OtherPropertyBag = OtherParameters->GetPropertyBagStruct();
	const UPropertyBag* ThisPropertyBag = ThisParameters->GetPropertyBagStruct();

	if (!ThisPropertyBag || !OtherPropertyBag)
	{
		return ThisPropertyBag == OtherPropertyBag;
	}

	// TODO: Be more resitant to different layout.
	// For now we are only comparing structs that must have the same layout.
	TConstArrayView<FPropertyBagPropertyDesc> OtherParametersDescs = OtherPropertyBag->GetPropertyDescs();
	TConstArrayView<FPropertyBagPropertyDesc> ThisParametersDescs = ThisPropertyBag->GetPropertyDescs();
	check(OtherParametersDescs.Num() == ThisParametersDescs.Num());

	// TODO: Hashing might be more efficient.
	for (int32 i = 0; i < ThisParametersDescs.Num(); ++i)
	{
		const FPropertyBagPropertyDesc& ThisParametersDesc = ThisParametersDescs[i];
		const FPropertyBagPropertyDesc& OtherParametersDesc = OtherParametersDescs[i];

		if (!DungeonForgeGraphUtils::ArePropertiesCompatible(&ThisParametersDesc, &OtherParametersDesc))
		{
			return false;
		}

		if (!DungeonForgeGraphUtils::ArePropertiesIdentical(&ThisParametersDesc, *ThisParameters, &OtherParametersDesc, *OtherParameters))
		{
			return false;
		}
	}

	return true;
}

EDungeonForgeGraphChangeType UDungeonForgeGraphInterface::GetChangeTypeForGraphParameterChange(EDungeonForgeGraphParameterEvent InChangeType, FName InChangedPropertyName)
{
	// If the parameter had its order changed in the struct, was just added or was removed but was not used in the graph, it is not a change that requires a refresh, so we go with Cosmetic change type.
	if (InChangeType == EDungeonForgeGraphParameterEvent::PropertyMoved || InChangeType == EDungeonForgeGraphParameterEvent::Added || InChangeType == EDungeonForgeGraphParameterEvent::RemovedUnused)
	{
		return EDungeonForgeGraphChangeType::Cosmetic;
	}

	// If it is not linked to a single property, or it was removed and used, we need to refresh, so we go with Settings change type.
	if (InChangedPropertyName == NAME_None || InChangeType == EDungeonForgeGraphParameterEvent::RemovedUsed)
	{
		return EDungeonForgeGraphChangeType::Settings;
	}

	const UDungeonForgeGraph* Graph = GetGraph();
	const FInstancedPropertyBag* UserParameters = GetUserParametersStruct();
	if (!ensure(Graph && UserParameters))
	{
		// Should never happen, but if there is no graph nor user parameters, there is nothing to do.
		return EDungeonForgeGraphChangeType::None;
	}
	
	return EDungeonForgeGraphChangeType::Settings;
}

UDungeonForgeGraphNode* UDungeonForgeGraph::AddNode(UDungeonForgeNodeSettingsInterface* InSettingsInterface) {
	if (!InSettingsInterface || !InSettingsInterface->GetSettings())
	{
		return nullptr;
	}

	UDungeonForgeGraphNode* Node = InSettingsInterface->GetSettings()->CreateNode();

	if (Node)
	{
		Node->SetFlags(RF_Transactional);

		Modify();

		// Assign settings to node & reparent
		Node->SetSettingsInterface(InSettingsInterface);

		// Reparent node to this graph
		Node->Rename(nullptr, this, REN_DontCreateRedirectors);

#if WITH_EDITOR
		const FName DefaultNodeName = InSettingsInterface->GetSettings()->GetDefaultNodeName();
		if (DefaultNodeName != NAME_None)
		{
			const FName NodeName = MakeUniqueObjectName(this, UDungeonForgeGraphNode::StaticClass(), DefaultNodeName);
			// Flags added because default flags favor tick/interactive, not load-time renaming.
			Node->Rename(*NodeName.ToString(), nullptr, REN_DontCreateRedirectors);
		}
#endif

		Nodes.Add(Node);
		OnNodeAdded(Node);
	}

	return Node;
}

UDungeonForgeGraphNode* UDungeonForgeGraph::AddNodeOfType(TSubclassOf<class UDungeonForgeNodeSettings> InSettingsClass, UDungeonForgeNodeSettings*& OutDefaultNodeSettings) {
	UDungeonForgeNodeSettings* Settings = NewObject<UDungeonForgeNodeSettings>(GetTransientPackage(), InSettingsClass, NAME_None, RF_Transactional);

	if (!Settings)
	{
		return nullptr;
	}

	UDungeonForgeGraphNode* Node = AddNode(Settings);

	if (Node)
	{
		Settings->Rename(nullptr, Node, REN_DontCreateRedirectors);
	}

	OutDefaultNodeSettings = Settings;
	return Node;
}

UDungeonForgeGraphNode* UDungeonForgeGraph::AddNodeCopy(const UDungeonForgeNodeSettings* InSettings, UDungeonForgeNodeSettings*& DefaultNodeSettings) {
	if (!InSettings)
	{
		return nullptr;
	}

	UDungeonForgeNodeSettings* SettingsCopy = DuplicateObject(InSettings, nullptr);
	UDungeonForgeGraphNode* NewNode = AddNode(SettingsCopy);

	if (SettingsCopy)
	{
		SettingsCopy->Rename(nullptr, NewNode, REN_DontCreateRedirectors);
	}

	DefaultNodeSettings = SettingsCopy;
	return NewNode;
}

void UDungeonForgeGraph::RemoveNode(UDungeonForgeGraphNode* InNode) {
	RemoveNodes_Internal(MakeArrayView<UDungeonForgeGraphNode*>(&InNode, 1));
}

void UDungeonForgeGraph::RemoveNodes(TArray<UDungeonForgeGraphNode*>& InNodes) {
	RemoveNodes_Internal(InNodes);	
}

UDungeonForgeGraphNode* UDungeonForgeGraph::AddEdge(UDungeonForgeGraphNode* From, const FName& FromPinLabel, UDungeonForgeGraphNode* To, const FName& ToPinLabel) {
	AddLabeledEdge(From, FromPinLabel, To, ToPinLabel);
	return To;
}

bool UDungeonForgeGraph::RemoveEdge(UDungeonForgeGraphNode* From, const FName& FromLabel, UDungeonForgeGraphNode* To, const FName& ToLabel) {
	if (!From || !To)
	{
		UE_LOG(LogDungeonForge, Error, TEXT("Invalid from/to node in RemoveEdge"));
		return false;
	}

#if WITH_EDITOR
	DisableNotificationsForEditor();
#endif

	UDungeonForgeGraphPin* OutPin = From->GetOutputPin(FromLabel);
	UDungeonForgeGraphPin* InPin = To->GetInputPin(ToLabel);

	TSet<UDungeonForgeGraphNode*> TouchedNodes;
	if (OutPin)
	{
		OutPin->BreakEdgeTo(InPin, &TouchedNodes);
	}

	const EDungeonForgeGraphChangeType ChangeType = DungeonForgeGraphUtils::NotifyTouchedNodes(TouchedNodes, EDungeonForgeGraphChangeType::Structural);

#if WITH_EDITOR
	// After all nodes are notified, re-enable graph notifications and send graph change notification.
	EnableNotificationsForEditor();

	if (TouchedNodes.Num() > 0)
	{
		NotifyGraphStructureChanged(ChangeType);
	}
#endif

	return TouchedNodes.Num() > 0;
}

TObjectPtr<UDungeonForgeGraphNode> UDungeonForgeGraph::ReconstructNewNode(const UDungeonForgeGraphNode* InNode) {
	UDungeonForgeNodeSettings* NewSettings = nullptr;
	TObjectPtr<UDungeonForgeGraphNode> NewNode = AddNodeCopy(InNode->GetSettings(), NewSettings);

#if WITH_EDITOR
	InNode->TransferEditorProperties(NewNode);
#endif // WITH_EDITOR

	return NewNode;
}

bool UDungeonForgeGraph::AddLabeledEdge(UDungeonForgeGraphNode* From, const FName& FromPinLabel, UDungeonForgeGraphNode* To, const FName& ToPinLabel) {
	if (!From || !To)
	{
		UE_LOG(LogDungeonForge, Error, TEXT("Invalid edge nodes"));
		return false;
	}

	UDungeonForgeGraphPin* FromPin = From->GetOutputPin(FromPinLabel);

	if (!FromPin)
	{
		UE_LOG(LogDungeonForge, Error, TEXT("From node %s does not have the %s label"), *From->GetName(), *FromPinLabel.ToString());
		return false;
	}

	UDungeonForgeGraphPin* ToPin = To->GetInputPin(ToPinLabel);

	if (!ToPin)
	{
		UE_LOG(LogDungeonForge, Error, TEXT("To node %s does not have the %s label"), *To->GetName(), *ToPinLabel.ToString());
		return false;
	}

#if WITH_EDITOR
	DisableNotificationsForEditor();
#endif

	TSet<UDungeonForgeGraphNode*> TouchedNodes;

	// Create edge
	FromPin->AddEdgeTo(ToPin, &TouchedNodes);

	bool bToPinBrokeOtherEdges = false;

	// Add an edge to a pin that doesn't allow multiple connections requires to do some cleanup
	if (!ToPin->AllowsMultipleConnections())
	{
		bToPinBrokeOtherEdges = ToPin->BreakAllIncompatibleEdges(&TouchedNodes);
	}

	const EDungeonForgeGraphChangeType ChangeType = DungeonForgeGraphUtils::NotifyTouchedNodes(TouchedNodes, EDungeonForgeGraphChangeType::Structural);

#if WITH_EDITOR
	// After all nodes are notified, re-enable graph notifications and send graph change notification.
	EnableNotificationsForEditor();

	NotifyGraphStructureChanged(ChangeType);
#endif

	return bToPinBrokeOtherEdges;
}

bool UDungeonForgeGraph::Contains(UDungeonForgeGraphNode* Node) const {
	return Node == InputNode || Node == OutputNode || Nodes.Contains(Node);
}

bool UDungeonForgeGraph::Contains(const UDungeonForgeGraph* InGraph) const {
	bool bContains = false;
	/*
	auto ContainsSelectedGraph = [InGraph, &bContains](UDungeonForgeGraphNode* Node)
	{
		if (UPCGBaseSubgraphNode* SubgraphNode = Cast<UPCGBaseSubgraphNode>(Node))
		{
			if (InGraph == SubgraphNode->GetSubgraph())
			{
				bContains = true;
				return false; // stop execution
			}
		}

		return true;
	};

	ForEachNodeRecursively(ContainsSelectedGraph);
	*/
	return bContains;
}

UDungeonForgeGraphNode* UDungeonForgeGraph::FindNodeWithSettings(const UDungeonForgeNodeSettingsInterface* InSettings, bool bRecursive) const {
	UDungeonForgeGraphNode* NodeFound = nullptr;

	auto FindNode = [&NodeFound, InSettings](UDungeonForgeGraphNode* InNode)
	{
		if (InNode && InNode->GetSettingsInterface() == InSettings)
		{
			NodeFound = InNode;
			return false; // stop execution
		}
		else
		{
			return true;
		}
	};

	if (bRecursive)
	{
		ForEachNodeRecursively(FindNode);
	}
	else
	{
		ForEachNode(FindNode);
	}

	return NodeFound;
}

void UDungeonForgeGraph::AddNode(UDungeonForgeGraphNode* InNode) {
	AddNodes_Internal(MakeArrayView<UDungeonForgeGraphNode*>(&InNode, 1));
}

void UDungeonForgeGraph::AddNodes(TArray<UDungeonForgeGraphNode*>& InNodes) {
	AddNodes_Internal(InNodes);
}

bool UDungeonForgeGraph::ForEachNode(TFunctionRef<bool(UDungeonForgeGraphNode*)> Action) const {
	if (!Action(InputNode) ||
		!Action(OutputNode))
	{
		return false;
	}

	for (UDungeonForgeGraphNode* Node : Nodes)
	{
		if (!Action(Node))
		{
			return false;
		}
	}

	return true;
}

bool UDungeonForgeGraph::ForEachNodeRecursively(TFunctionRef<bool(UDungeonForgeGraphNode*)> Action) const {
	TSet<const UDungeonForgeGraph*> VisitedGraphs;
	return ForEachNodeRecursively_Internal(Action, VisitedGraphs);
}

bool UDungeonForgeGraph::RemoveInboundEdges(UDungeonForgeGraphNode* InNode, const FName& InboundLabel) {
	
	check(InNode);
	TSet<UDungeonForgeGraphNode*> TouchedNodes;

#if WITH_EDITOR
	DisableNotificationsForEditor();
#endif

	if (UDungeonForgeGraphPin* InputPin = InNode->GetInputPin(InboundLabel))
	{
		InputPin->BreakAllEdges(&TouchedNodes);
	}

	const EDungeonForgeGraphChangeType ChangeType = DungeonForgeGraphUtils::NotifyTouchedNodes(TouchedNodes, EDungeonForgeGraphChangeType::Structural);

#if WITH_EDITOR
	// After all nodes are notified, re-enable graph notifications and send graph change notification.
	EnableNotificationsForEditor();

	if (TouchedNodes.Num() > 0)
	{
		NotifyGraphStructureChanged(ChangeType);
	}
#endif

	return TouchedNodes.Num() > 0;
}

bool UDungeonForgeGraph::RemoveOutboundEdges(UDungeonForgeGraphNode* InNode, const FName& OutboundLabel) {
	check(InNode);
	// Make a list of downstream nodes which may need pin updates when the edges change
	TSet<UDungeonForgeGraphNode*> TouchedNodes;

#if WITH_EDITOR
	DisableNotificationsForEditor();
#endif

	if (UDungeonForgeGraphPin* OutputPin = InNode->GetOutputPin(OutboundLabel))
	{
		OutputPin->BreakAllEdges(&TouchedNodes);
	}

	const EDungeonForgeGraphChangeType ChangeType = DungeonForgeGraphUtils::NotifyTouchedNodes(TouchedNodes, EDungeonForgeGraphChangeType::Structural);

#if WITH_EDITOR
	// After all nodes are notified, re-enable graph notifications and send graph change notification.
	EnableNotificationsForEditor();

	if (TouchedNodes.Num() > 0)
	{
		NotifyGraphStructureChanged(ChangeType);
	}
#endif

	return TouchedNodes.Num() > 0;
}

bool UDungeonForgeGraph::ForEachNodeRecursively_Internal(TFunctionRef<bool(UDungeonForgeGraphNode*)> Action, TSet<const UDungeonForgeGraph*>& VisitedGraphs) const {
	check(!VisitedGraphs.Contains(this));
	VisitedGraphs.Add(this);

	auto RecursiveCall = [&Action, &VisitedGraphs](UDungeonForgeGraphNode* Node) -> bool
	{
		if (!Action(Node))
		{
			return false;
		}

		/*
		if (UPCGBaseSubgraphNode* SubgraphNode = Cast<UPCGBaseSubgraphNode>(Node))
		{
			if (const UPCGGraph* Subgraph = SubgraphNode->GetSubgraph())
			{
				if (!VisitedGraphs.Contains(Subgraph))
				{
					return Subgraph->ForEachNodeRecursively_Internal(Action, VisitedGraphs);
				}
			}
		}
		*/

		return true;
	};

	return ForEachNode(RecursiveCall);
}

void UDungeonForgeGraph::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector) {
#if WITH_EDITOR
	UDungeonForgeGraph* This = CastChecked<UDungeonForgeGraph>(InThis);
	Collector.AddReferencedObject(This->ForgeEditorGraph, This);
#endif
	Super::AddReferencedObjects(InThis, Collector);
}

#if WITH_EDITOR
void UDungeonForgeGraph::PreSave(FObjectPreSaveContext ObjectSaveContext) {
	Super::PreSave(ObjectSaveContext);
	
	if (ObjectSaveContext.IsCooking()) {
		FDungeonForgeGraphCompiler GraphCompiler(/*bIsCooking=*/true);
		TArray<FDungeonForgeGraphTask> CompiledTasks;
		GraphCompiler.Compile(this, CompiledTasks);

		CookedCompilationData = NewObject<UDungeonForgeGraphCompilationData>(this);
		CookedCompilationData->Tasks.Reserve(CompiledTasks.Num());

		for (FDungeonForgeGraphTask& GraphTask : CompiledTasks) {
			GraphTask.PrepareForCook();
			CookedCompilationData->Tasks.Emplace(MoveTemp(GraphTask));
		}
	}
}

void UDungeonForgeGraph::DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass) {
	Super::DeclareConstructClasses(OutConstructClasses, SpecificSubclass);
	OutConstructClasses.Add(FTopLevelAssetPath(UDungeonForgeGraphPin::StaticClass()));
}

void UDungeonForgeGraph::DisableNotificationsForEditor() {
	
	check(GraphChangeNotificationsDisableCounter >= 0);
	++GraphChangeNotificationsDisableCounter;
}

void UDungeonForgeGraph::EnableNotificationsForEditor() {
	check(GraphChangeNotificationsDisableCounter > 0);
	--GraphChangeNotificationsDisableCounter;

	if (GraphChangeNotificationsDisableCounter == 0 && bDelayedChangeNotification)
	{
		NotifyGraphChanged(DelayedChangeType);
		bDelayedChangeNotification = false;
		DelayedChangeType = EDungeonForgeGraphChangeType::None;
	}
}

void UDungeonForgeGraph::ToggleUserPausedNotificationsForEditor() {
	if (bUserPausedNotificationsInGraphEditor) {
		EnableNotificationsForEditor();
	}
	else {
		DisableNotificationsForEditor();
	}

	bUserPausedNotificationsInGraphEditor = !bUserPausedNotificationsInGraphEditor;
}

void UDungeonForgeGraph::PreNodeUndo(UDungeonForgeGraphNode* InForgeNode) {
	if (InForgeNode) {
		InForgeNode->OnNodeChangedDelegate.RemoveAll(this);
	}
}

void UDungeonForgeGraph::PostNodeUndo(UDungeonForgeGraphNode* InForgeNode) {
	if (InForgeNode) {
		InForgeNode->OnNodeChangedDelegate.AddUObject(this, &UDungeonForgeGraph::OnNodeChanged);
	}
}

void UDungeonForgeGraph::SetExtraEditorNodes(const TArray<TObjectPtr<const UObject>>& InNodes) {
	ExtraEditorNodes.Empty();

	for (const UObject* Node : InNodes)
	{
		ExtraEditorNodes.Add(DuplicateObject(Node, this));
	}
}

#endif // WITH_EDITOR

void UDungeonForgeGraph::OnNodeAdded(UDungeonForgeGraphNode* InNode, bool bNotify)
{
	OnNodesAdded(MakeArrayView<UDungeonForgeGraphNode*>(&InNode, 1), bNotify);
}

void UDungeonForgeGraph::OnNodesAdded(TArrayView<UDungeonForgeGraphNode*> InNodes, bool bNotify)
{
#if WITH_EDITOR
	EDungeonForgeGraphChangeType ChangeType = EDungeonForgeGraphChangeType::Structural;

	for (UDungeonForgeGraphNode* Node : InNodes)
	{
		if (Node)
		{
			Node->OnNodeChangedDelegate.AddUObject(this, &UDungeonForgeGraph::OnNodeChanged);
		}
	}

	if (bNotify)
	{
		NotifyGraphStructureChanged(ChangeType);
	}
#endif
}

void UDungeonForgeGraph::OnNodeRemoved(UDungeonForgeGraphNode* InNode, bool bNotify)
{
	OnNodesRemoved(MakeArrayView<UDungeonForgeGraphNode*>(&InNode, 1), bNotify);
}

void UDungeonForgeGraph::OnNodesRemoved(TArrayView<UDungeonForgeGraphNode*> InNodes, bool bNotify)
{
#if WITH_EDITOR
	for (UDungeonForgeGraphNode* Node : InNodes)
	{
		if (Node)
		{
			Node->OnNodeChangedDelegate.RemoveAll(this);
		}
	}

	if (bNotify)
	{
		NotifyGraphStructureChanged(EDungeonForgeGraphChangeType::Structural);
	}
#endif
}

void UDungeonForgeGraph::RemoveNodes_Internal(TArrayView<UDungeonForgeGraphNode*> InNodes) {
	
	if (InNodes.IsEmpty())
	{
		return;
	}

	Modify();

#if WITH_EDITOR
	DisableNotificationsForEditor();
#endif

	TSet<UDungeonForgeGraphNode*> TouchedNodes;

	for (UDungeonForgeGraphNode* Node : InNodes)
	{
		check(Node);

		for (UDungeonForgeGraphPin* InputPin : Node->InputPins)
		{
			InputPin->BreakAllEdges(&TouchedNodes);
		}

		for (UDungeonForgeGraphPin* OutputPin : Node->OutputPins)
		{
			OutputPin->BreakAllEdges(&TouchedNodes);
		}

		// We're about to remove InNode, so don't bother triggering updates
		TouchedNodes.Remove(Node);

		// Add the node to the transaction, to make sure we reconnect everything correctly on Undo/Redo
		Node->Modify();

		Nodes.Remove(Node);
	}

#if WITH_EDITOR
	EnableNotificationsForEditor();
#endif

	DungeonForgeGraphUtils::NotifyTouchedNodes(TouchedNodes, EDungeonForgeGraphChangeType::Structural);

	OnNodesRemoved(InNodes);
}

void UDungeonForgeGraph::AddNodes_Internal(TArrayView<UDungeonForgeGraphNode*> InNodes) {
	if (InNodes.IsEmpty())
	{
		return;
	}

	Modify();

	for (UDungeonForgeGraphNode* Node : InNodes)
	{
		check(Node);
		Node->Rename(nullptr, this, REN_DontCreateRedirectors);

#if WITH_EDITOR
		const FName DefaultNodeName = Node->GetSettings()->GetDefaultNodeName();
		if (DefaultNodeName != NAME_None)
		{
			FName NodeName = MakeUniqueObjectName(this, UDungeonForgeGraphNode::StaticClass(), DefaultNodeName);
			Node->Rename(*NodeName.ToString(), nullptr, REN_DontCreateRedirectors);
		}
#endif

		Nodes.Add(Node);
	}

	OnNodesAdded(InNodes);
}

#if WITH_EDITOR

void UDungeonForgeGraph::NotifyGraphStructureChanged(EDungeonForgeGraphChangeType ChangeType, bool bForce) {
	NotifyGraphChanged(ChangeType);
}

void UDungeonForgeGraph::NotifyGraphChanged(EDungeonForgeGraphChangeType ChangeType) {
	TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgeGraph::NotifyGraphChanged);

	if (GraphChangeNotificationsDisableCounter > 0)
	{
		bDelayedChangeNotification = true;
		DelayedChangeType |= ChangeType;
		return;
	}

	// Skip recursive cases which can happen either through direct recursivity (A -> A) or indirectly (A -> B -> A)
	if (bIsNotifying)
	{
		return;
	}

	bIsNotifying = true;

	OnGraphChangedDelegate.Broadcast(this, ChangeType);

	bIsNotifying = false;
}

void UDungeonForgeGraph::OnNodeChanged(UDungeonForgeGraphNode* InNode, EDungeonForgeGraphChangeType ChangeType) {
	TRACE_CPUPROFILER_EVENT_SCOPE(UDungeonForgeGraph::OnNodeChanged);

	if ((ChangeType & ~EDungeonForgeGraphChangeType::Cosmetic) != EDungeonForgeGraphChangeType::None)
	{
		NotifyGraphStructureChanged(ChangeType);
	}
}

void UDungeonForgeGraph::NotifyGraphParametersChanged(EDungeonForgeGraphParameterEvent InChangeType, FName InChangedPropertyName) {
	if (bIsNotifying)
	{
		return;
	}

	bIsNotifying = true;
	OnGraphParametersChangedDelegate.Broadcast(this, InChangeType, InChangedPropertyName);
	bIsNotifying = false;

	NotifyGraphChanged(GetChangeTypeForGraphParameterChange(InChangeType, InChangedPropertyName));
}

#endif // WITH_EDITOR


