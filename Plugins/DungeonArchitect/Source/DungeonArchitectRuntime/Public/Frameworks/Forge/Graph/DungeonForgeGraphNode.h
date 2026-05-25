//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeCommon.h"
#include "DungeonForgeGraphNode.generated.h"

class UDungeonForgeNodeSettings;
class UDungeonForgeGraphPin;
class UDungeonForgeNodeSettingsInterface;

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDungeonForgeGraphNodeChanged, UDungeonForgeGraphNode*, EDungeonForgeGraphChangeType);
#endif

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeGraphNode : public UObject {
	GENERATED_BODY()
public:

	friend class UDungeonForgeGraph;
	friend class UDungeonForgeGraphEdge;
	friend class FDungeonForgeGraphCompiler;
	
	UDungeonForgeGraphNode();
	
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostLoad() override;
	
	/** Recursively follow downstream edges and call UpdatePins on each node that has dynamic pins. */
	EDungeonForgeGraphChangeType PropagateDynamicPinTypes(TSet<UDungeonForgeGraphNode*>& TouchedNodes, const UDungeonForgeGraphNode* FromNode = nullptr);
	
	
#if WITH_EDITOR
	/** Transfer all editor only properties to the other node */
	void TransferEditorProperties(UDungeonForgeGraphNode* OtherNode) const;
	
	UFUNCTION(BlueprintCallable, Category = Node)
	void GetNodePosition(int32& OutPositionX, int32& OutPositionY) const;

	UFUNCTION(BlueprintCallable, Category = Node)
	void SetNodePosition(int32 InPositionX, int32 InPositionY);
#endif

	/** Returns the settings interface (settings or instance of settings) on this node */
	UDungeonForgeNodeSettingsInterface* GetSettingsInterface() const { return SettingsInterface.Get(); }

	/** Changes the default settings in the node */
	void SetSettingsInterface(UDungeonForgeNodeSettingsInterface* InSettingsInterface, bool bUpdatePins = true);

	/** Returns the settings this node holds (either directly or through an instance) */
	UFUNCTION(BlueprintCallable, Category = Node)
	UDungeonForgeNodeSettings* GetSettings() const;
	
	/** Triggers some uppdates after creating a new node and changing its settings */
	void UpdateAfterSettingsChangeDuringCreation();
	
	UDungeonForgeGraphPin* GetInputPin(const FName& Label);
	const UDungeonForgeGraphPin* GetInputPin(const FName& Label) const;
	UDungeonForgeGraphPin* GetOutputPin(const FName& Label);
	const UDungeonForgeGraphPin* GetOutputPin(const FName& Label) const;
	const TArray<TObjectPtr<UDungeonForgeGraphPin>>& GetInputPins() const { return InputPins; }
	const TArray<TObjectPtr<UDungeonForgeGraphPin>>& GetOutputPins() const { return OutputPins; }
	
	bool HasInboundEdges() const;
	int32 GetInboundEdgesNum() const;

	UPROPERTY()
	FGuid NodeGuid;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node)
	FName NodeTitle = NAME_None;

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node)
	FLinearColor NodeTitleColor = FLinearColor::White;
#endif // WITH_EDITORONLY_DATA

protected:
	/** Updates pins based on node settings. Attempts to migrate pins via matching. Broadcasts node change events for affected nodes. */
	EDungeonForgeGraphChangeType UpdatePins();
	/** Updates pins based on node settings PinAllocator creates new pin objects. Attempts to migrate pins via matching. Broadcasts node change events for affected nodes. */
	EDungeonForgeGraphChangeType UpdatePins(TFunctionRef<UDungeonForgeGraphPin* (UDungeonForgeGraphNode*)> PinAllocator);
	
	// When we create a new graph, we initialize the input/output nodes as default, with default pins.
	// Those default pins are not serialized, therefore if we change the default pins, combined with the use
	// of recycling objects in Unreal, can lead to pins that are garbage or even worse: valid pins but not the right
	// one, potentially making the edges connecting wrong pins together!
	// That is why we have a specific function to create default pins, and we have to make sure that those
	// default pins are always created the same way.
	void CreateDefaultPins(TFunctionRef<UDungeonForgeGraphPin* (UDungeonForgeGraphNode*)> PinAllocator);

public:
	/** Returns the owning graph */
	UFUNCTION(BlueprintCallable, Category = Node)
	UDungeonForgeGraph* GetGraph() const;

	/** Adds an edge in the owning graph to the given "To" node. */
	UFUNCTION(BlueprintCallable, Category = Node)
	UDungeonForgeGraphNode* AddEdgeTo(FName FromPinLabel, UDungeonForgeGraphNode* To, FName ToPinLabel);

	/** Removes an edge originating from this node */
	UFUNCTION(BlueprintCallable, Category = Node)
	bool RemoveEdgeTo(FName FromPinLabel, UDungeonForgeGraphNode* To, FName ToPinLabel);

	/** Get title for node of specified type. */
	FText GetNodeTitle(EDungeonForgeNodeTitleType TitleType) const;

	/** Whether user has renamed the node. */
	bool HasAuthoredTitle() const;

	/** Title to use if no title is authored. */
	FText GetDefaultTitle() const;

	/** Authored part of node title (like "Create Attribute 1"). */
	FText GetAuthoredTitleLine() const;

	/** Whether to flip the order of the title lines - display generated title first and authored second. */
	bool HasFlippedTitleLines() const;

	/** Generated part of node title, not user editable (like "MyValue = 5.0"). */
	FText GetGeneratedTitleLine() const;

#if WITH_EDITOR
	/** Tooltip that describes node functionality and other information. */
	FText GetNodeTooltipText() const;

	FOnDungeonForgeGraphNodeChanged OnNodeChangedDelegate;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	int32 PositionX;

	UPROPERTY()
	int32 PositionY;

	UPROPERTY()
	FString NodeComment;

	UPROPERTY()
	uint8 bCommentBubblePinned : 1;

	UPROPERTY()
	uint8 bCommentBubbleVisible : 1;
#endif // WITH_EDITORONLY_DATA

protected:
	/** Note: do not set this property directly from code, use SetSettingsInterface instead */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Node)
	TObjectPtr<UDungeonForgeNodeSettingsInterface> SettingsInterface;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Node)
	TArray<TObjectPtr<UDungeonForgeGraphPin>> InputPins;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Node)
	TArray<TObjectPtr<UDungeonForgeGraphPin>> OutputPins;

	// TODO: CreateDefaultPins()
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void OnSettingsChanged(UDungeonForgeNodeSettings* InSettings, EDungeonForgeGraphChangeType ChangeType);
#endif
	
};

