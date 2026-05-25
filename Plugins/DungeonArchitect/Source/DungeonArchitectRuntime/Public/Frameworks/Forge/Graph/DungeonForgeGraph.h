//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Common/Utils/DungeonForgeGraphParameterExtension.h"
#include "Frameworks/Forge/DungeonForgeCommon.h"

#include "StructUtils/PropertyBag.h"
#include "Templates/SubclassOf.h"
#include "DungeonForgeGraph.generated.h"

class UDungeonForgeGraphNode;
class UDungeonForgeNodeSettings;
class UDungeonForgeNodeSettingsInterface;
class UDungeonForgeGraphInterface;
class UDungeonForgeGraphCompilationData;
struct FDungeonForgeCompilerDiagnostics;

#if WITH_EDITOR
class UDungeonForgeEditorGraph;
struct FEdGraphPinType;
#endif // WITH_EDITOR


enum class EDungeonForgeGraphParameterEvent
{
	GraphChanged,
	GraphPostLoad,
	Added,
	RemovedUnused,
	RemovedUsed,
	PropertyMoved,
	PropertyRenamed,
	PropertyTypeModified,
	ValueModifiedLocally,
	ValueModifiedByParent,
	MultiplePropertiesAdded,
	None
};


#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDungeonForgeGraphChanged, UDungeonForgeGraphInterface* /*Graph*/, EDungeonForgeGraphChangeType /*ChangeType*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDungeonForgeGraphStructureChanged, UDungeonForgeGraphInterface* /*Graph*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDungeonForgeGraphParametersChanged, UDungeonForgeGraphInterface* /*Graph*/, EDungeonForgeGraphParameterEvent /*ChangeType*/, FName /*ChangedPropertyName*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDungeonForgeNodeSourceCompiled, const UDungeonForgeGraphNode*, const FDungeonForgeCompilerDiagnostics&);
#endif // WITH_EDITOR


class UDungeonForgeGraph;
class UDungeonForgeGraphNode;


UCLASS(BlueprintType, Abstract, ClassGroup = (Procedural))
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeGraphInterface : public UObject {
	GENERATED_BODY()
public:
	/** Return the underlying Forge Graph for this interface. */
	UFUNCTION(BlueprintCallable, Category = Graph)
	UDungeonForgeGraph* GetMutableForgeGraph() { return GetGraph(); }

	/** Return the underlying Forge Graph for this interface. */
	UFUNCTION(BlueprintCallable, Category = Graph)
	const UDungeonForgeGraph* GetConstForgeGraph() const { return GetGraph(); }

	virtual UDungeonForgeGraph* GetGraph() PURE_VIRTUAL(UDungeonForgeGraphInterface::GetGraph, return nullptr;)
	virtual const UDungeonForgeGraph* GetGraph() const PURE_VIRTUAL(UDungeonForgeGraphInterface::GetGraph, return nullptr;)

	virtual const FInstancedPropertyBag* GetUserParametersStruct() const PURE_VIRTUAL(UDungeonForgeGraphInterface::GetUserParametersStruct, return nullptr;)

	/** A graph interface is equivalent to another graph interface if they are the same (same ptr), or if they have the same graph. Will be overridden when graph instance supports overrides. */
	virtual bool IsEquivalent(const UDungeonForgeGraphInterface* Other) const;
	
	bool IsInstance() const;
	
#if WITH_EDITOR
	FOnDungeonForgeGraphChanged OnGraphChangedDelegate;
	FOnDungeonForgeGraphParametersChanged OnGraphParametersChangedDelegate;
	FOnDungeonForgeNodeSourceCompiled OnNodeSourceCompiledDelegate;

	virtual TOptional<FText> GetTitleOverride() const { return bOverrideTitle ? Title : TOptional<FText>(); }
	virtual TOptional<FLinearColor> GetColorOverride() const { return bOverrideColor ? Color : TOptional<FLinearColor>(); }
protected:
	/** By default export to library is visible only for graph that are assets, but can be enabled/disabled if needed. */
	UFUNCTION()
	virtual bool IsExportToLibraryEnabled() const { return IsAsset(); }

	UFUNCTION()
	virtual bool AreOverridesEnabled() const { return IsExportToLibraryEnabled() && bExposeToLibrary; }
#endif // WITH_EDITOR

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AssetInfo, AssetRegistrySearchable, meta = (EditCondition = "IsExportToLibraryEnabled", EditConditionHides))
	bool bExposeToLibrary = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AssetInfo, AssetRegistrySearchable, meta = (EditCondition = "AreOverridesEnabled", EditConditionHides))
	bool bOverrideTitle = false;

	/** Override of the title for the subgraph node for this graph. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AssetInfo, AssetRegistrySearchable, meta = (EditCondition = "AreOverridesEnabled && bOverrideTitle", EditConditionHides))
	FText Title;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AssetInfo, meta = (EditCondition = "AreOverridesEnabled", EditConditionHides))
	bool bOverrideColor = false;

	/** Override of the color for the subgraph node for this graph. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AssetInfo, meta = (EditCondition = "AreOverridesEnabled && bOverrideColor", EditConditionHides))
	FLinearColor Color = FLinearColor::White;
#endif

	template <typename T>
	TValueOrError<T, EPropertyBagResult> GetGraphParameter(const FName PropertyName) const
	{
		const FInstancedPropertyBag* UserParameters = GetUserParametersStruct();
		check(UserParameters);

		if constexpr (std::is_enum_v<T> && StaticEnum<T>())
		{
			return FDungeonForgeGraphParameterExtension::GetGraphParameter<T>(*UserParameters, PropertyName, StaticEnum<T>());
		}
		else
		{
			return FDungeonForgeGraphParameterExtension::GetGraphParameter<T>(*UserParameters, PropertyName);
		}
	}

	virtual bool IsGraphParameterOverridden(const FName PropertyName) const { return false; }

	template <typename T>
	EPropertyBagResult SetGraphParameter(const FName PropertyName, const T& Value)
	{
		FInstancedPropertyBag* UserParameters = GetMutableUserParametersStruct();
		check(UserParameters);

		EPropertyBagResult Result;
		if constexpr (std::is_enum_v<T> && StaticEnum<T>())
		{
			Result = FDungeonForgeGraphParameterExtension::SetGraphParameter(*UserParameters, PropertyName, Value, StaticEnum<T>());
		}
		else
		{
			Result = FDungeonForgeGraphParameterExtension::SetGraphParameter<T>(*UserParameters, PropertyName, Value);
		}

		if (Result == EPropertyBagResult::Success)
		{
			OnGraphParametersChanged(EDungeonForgeGraphParameterEvent::ValueModifiedLocally, PropertyName);
		}

		return Result;
	}

	EPropertyBagResult SetGraphParameter(const FName PropertyName, const uint64 Value, const UEnum* Enum);

	/**
	 * Allows to manipulate directly FPropertyBagArrayRef, while propagating changes to child instances. 
	 * @param PropertyName Name of the property to access. Must be an Array.
	 * @param Callback Callback to call with the FPropertyBagArrayRef. Returns a bool telling if there was a change.
	 * @returns True if the change succeeded.
	 */
	bool UpdateArrayGraphParameter(const FName PropertyName, TFunctionRef<bool(FPropertyBagArrayRef& PropertyBagArrayRef)> Callback);

	/**
	 * Allows to manipulate directly FPropertyBagSetRef, while propagating changes to child instances. 
	 * @param PropertyName Name of the property to access. Must be a Set.
	 * @param Callback Callback to call with the FPropertyBagSetRef. Returns a bool telling if there was a change.
	 * @returns True if the change succeeded.
	 */
	bool UpdateSetGraphParameter(const FName PropertyName, TFunctionRef<bool(FPropertyBagSetRef& PropertyBagSetRef)> Callback);

	virtual void OnGraphParametersChanged(EDungeonForgeGraphParameterEvent InChangeType, FName InChangedPropertyName) PURE_VIRTUAL(UDungeonForgeGraphInterface::OnGraphParametersChanged, )

protected:
	virtual FInstancedPropertyBag* GetMutableUserParametersStruct() PURE_VIRTUAL(UDungeonForgeGraphInterface::GetMutableUserParametersStruct, return nullptr;)

	/** Detecting if we need to refresh the graph depending on the type of change in the Graph Parameter. */
	EDungeonForgeGraphChangeType GetChangeTypeForGraphParameterChange(EDungeonForgeGraphParameterEvent InChangeType, FName InChangedPropertyName);
	
};


UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeGraph : public UDungeonForgeGraphInterface {
	GENERATED_BODY()
public:
	/** Creates a node using the given settings interface. Does not manage ownership - done outside of this method. */
	UDungeonForgeGraphNode* AddNode(UDungeonForgeNodeSettingsInterface* InSettings);

	/** Creates a default node based on the settings class wanted. Returns the newly created node. */
	UFUNCTION(BlueprintCallable, Category = Graph, meta=(DeterminesOutputType = "InSettingsClass", DynamicOutputParam = "DefaultNodeSettings"))
	UDungeonForgeGraphNode* AddNodeOfType(TSubclassOf<class UDungeonForgeNodeSettings> InSettingsClass, UDungeonForgeNodeSettings*& DefaultNodeSettings);

	template <typename T, typename = typename std::enable_if_t<std::is_base_of_v<UDungeonForgeNodeSettings, T>>>
	UDungeonForgeGraphNode* AddNodeOfType(T*& DefaultNodeSettings);

	/** Creates a node and copies the input settings. Returns the created node. */
	UFUNCTION(BlueprintCallable, Category = Graph, meta = (DeterminesOutputType = "InSettings", DynamicOutputParam = "OutCopiedSettings"))
	UDungeonForgeGraphNode* AddNodeCopy(const UDungeonForgeNodeSettings* InSettings, UDungeonForgeNodeSettings*& DefaultNodeSettings);

	/** Removes a node from the graph. */
	UFUNCTION(BlueprintCallable, Category = Graph)
	void RemoveNode(UDungeonForgeGraphNode* InNode);

	/** Bulk removal of nodes, to avoid notifying the world everytime. */
	UFUNCTION(BlueprintCallable, Category = Graph)
	void RemoveNodes(TArray<UDungeonForgeGraphNode*>& InNodes);

	/** Adds a directed edge in the graph. Returns the "To" node for easy chaining */
	UFUNCTION(BlueprintCallable, Category = Graph)
	UDungeonForgeGraphNode* AddEdge(UDungeonForgeGraphNode* From, const FName& FromPinLabel, UDungeonForgeGraphNode* To, const FName& ToPinLabel);

	/** Removes an edge in the graph. Returns true if an edge was removed. */
	UFUNCTION(BlueprintCallable, Category = Graph)
	bool RemoveEdge(UDungeonForgeGraphNode* From, const FName& FromLabel, UDungeonForgeGraphNode* To, const FName& ToLabel);

	/** Returns the graph input node */
	UFUNCTION(BlueprintCallable, Category = Graph)
	UDungeonForgeGraphNode* GetInputNode() const { return InputNode; }

	/** Returns the graph output node */
	UFUNCTION(BlueprintCallable, Category = Graph)
	UDungeonForgeGraphNode* GetOutputNode() const { return OutputNode; }

	/** Duplicate a given node by creating a new node with the same settings and properties, but without any edges and add it to the graph */
	TObjectPtr<UDungeonForgeGraphNode> ReconstructNewNode(const UDungeonForgeGraphNode* InNode);

	/** Creates an edge between two nodes/pins based on the labels. Returns true if the To node has removed other edges (happens with single pins) */
	bool AddLabeledEdge(UDungeonForgeGraphNode* From, const FName& InboundLabel, UDungeonForgeGraphNode* To, const FName& OutboundLabel);

	/** Returns true if the current graph contains directly the specified node. This does not query recursively (through subgraphs). */
	bool Contains(UDungeonForgeGraphNode* Node) const;

	/** Returns true if the current graph contains a subgraph node using statically the specified graph, recursively. */
	bool Contains(const UDungeonForgeGraph* InGraph) const;

	/** Returns the node with the given settings in the graph, if any */
	UDungeonForgeGraphNode* FindNodeWithSettings(const UDungeonForgeNodeSettingsInterface* InSettings, bool bRecursive = false) const;

	const TArray<UDungeonForgeGraphNode*>& GetNodes() const { return Nodes; }
	void AddNode(UDungeonForgeGraphNode* InNode);
	void AddNodes(TArray<UDungeonForgeGraphNode*>& InNodes);

	/** Calls the lambda on every node in the graph or until the Action call returns false */
	bool ForEachNode(TFunctionRef<bool(UDungeonForgeGraphNode*)> Action) const;

	/** Calls the lambda on every node (going through subgraphs too) or until the Action call returns false */
	bool ForEachNodeRecursively(TFunctionRef<bool(UDungeonForgeGraphNode*)> Action) const;

	bool RemoveInboundEdges(UDungeonForgeGraphNode* InNode, const FName& InboundLabel);
	bool RemoveOutboundEdges(UDungeonForgeGraphNode* InNode, const FName& OutboundLabel);

	TObjectPtr<UDungeonForgeGraphCompilationData> GetCookedCompilationData() { return CookedCompilationData; }
	const TObjectPtr<UDungeonForgeGraphCompilationData> GetCookedCompilationData() const { return CookedCompilationData; }

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	
	
#if WITH_EDITOR
	/** Trigger a recompilation of the relevant permutations of this graph and check for change in the compiled tasks. */
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	static void DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass);
	
	void DisableNotificationsForEditor();
	void EnableNotificationsForEditor();
	void ToggleUserPausedNotificationsForEditor();
	bool NotificationsForEditorArePausedByUser() const { return bUserPausedNotificationsInGraphEditor; }

	void PreNodeUndo(UDungeonForgeGraphNode* InForgeNode);
	void PostNodeUndo(UDungeonForgeGraphNode* InForgeNode);

	const TArray<TObjectPtr<UObject>>& GetExtraEditorNodes() const { return ExtraEditorNodes; }
	void SetExtraEditorNodes(const TArray<TObjectPtr<const UObject>>& InNodes);

	FOnDungeonForgeGraphChanged OnGraphChangedDelegate;
	FOnDungeonForgeGraphParametersChanged OnGraphParametersChangedDelegate;
#endif // WITH_EDITOR

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Graph, meta = (NoResetToDefault))
	TArray<TObjectPtr<UDungeonForgeGraphNode>> Nodes;
	
	// Add input/output nodes
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Graph, meta = (NoResetToDefault))
	TObjectPtr<UDungeonForgeGraphNode> InputNode;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Graph, meta = (NoResetToDefault))
	TObjectPtr<UDungeonForgeGraphNode> OutputNode;
	
#if WITH_EDITORONLY_DATA
	// Extra data to hold information that is useful only in editor (like comments)
	UPROPERTY()
	TArray<TObjectPtr<UObject>> ExtraEditorNodes;
	
	// Editor graph created from Dungeon Forge Editor but owned by this, reference is collected using AddReferencedObjects
	TObjectPtr<UDungeonForgeEditorGraph> ForgeEditorGraph = nullptr;
#endif // WITH_EDITORONLY_DATA
	
	// Parameters
	UPROPERTY(EditAnywhere, Category = Instance, meta = (DisplayName = "Parameters", NoResetToDefault, DefaultType = "EPropertyBagPropertyType::Double", IsPinTypeAccepted = "UserParametersIsPinTypeAccepted", CanRemoveProperty = "UserParametersCanRemoveProperty"))
	FInstancedPropertyBag UserParameters;
	
#if WITH_EDITOR
	friend class FDungeonForgeEditorModeToolkit;
#endif // WITH_EDITOR
	
protected:
	/** Internal function to react to add/remove nodes. bNotify can be set to false to not notify the world. */
	void OnNodeAdded(UDungeonForgeGraphNode* InNode, bool bNotify = true);
	void OnNodesAdded(TArrayView<UDungeonForgeGraphNode*> InNodes, bool bNotify = true);
	void OnNodeRemoved(UDungeonForgeGraphNode* InNode, bool bNotify = true);
	void OnNodesRemoved(TArrayView<UDungeonForgeGraphNode*> InNodes, bool bNotify = true);

	void RemoveNodes_Internal(TArrayView<UDungeonForgeGraphNode*> InNodes);
	void AddNodes_Internal(TArrayView<UDungeonForgeGraphNode*> InNodes);
	bool ForEachNodeRecursively_Internal(TFunctionRef<bool(UDungeonForgeGraphNode*)> Action, TSet<const UDungeonForgeGraph*>& VisitedGraphs) const;

protected:
	TObjectPtr<UDungeonForgeGraphCompilationData> CookedCompilationData = nullptr;
	
#if WITH_EDITOR
private:
	/** Sends a change notification. Demotes change if the compiled tasks are not significantly changed. */
	void NotifyGraphStructureChanged(EDungeonForgeGraphChangeType ChangeType, bool bForce = false);
	void NotifyGraphChanged(EDungeonForgeGraphChangeType ChangeType);
	void OnNodeChanged(UDungeonForgeGraphNode* InNode, EDungeonForgeGraphChangeType ChangeType);
	void NotifyGraphParametersChanged(EDungeonForgeGraphParameterEvent InChangeType, FName InChangedPropertyName);

	// Keep track of the previous PropertyBag, to see if we had a change in the number of properties, or if it is a rename/move.
	TObjectPtr<const UPropertyBag> PreviousPropertyBag;
	
	int32 GraphChangeNotificationsDisableCounter = 0;
	EDungeonForgeGraphChangeType DelayedChangeType = EDungeonForgeGraphChangeType::None;
	bool bDelayedChangeNotification = false;
	bool bIsNotifying = false;
	bool bUserPausedNotificationsInGraphEditor = false;
#endif // WITH_EDITOR
	
};

template <typename T, typename>
UDungeonForgeGraphNode* UDungeonForgeGraph::AddNodeOfType(T*& DefaultNodeSettings) {
	UDungeonForgeNodeSettings* TempSettings = DefaultNodeSettings;
	UDungeonForgeGraphNode* Node = AddNodeOfType(T::StaticClass(), TempSettings);
	DefaultNodeSettings = Cast<T>(TempSettings);
	return Node;
}

