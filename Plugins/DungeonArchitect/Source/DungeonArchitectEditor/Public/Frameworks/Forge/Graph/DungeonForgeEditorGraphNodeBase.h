//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "DungeonForgeEditorGraphNodeBase.generated.h"

class UDungeonForgeGraphPin;
enum class EDungeonForgeGraphChangeType : uint8;
class UDungeonForgeGraphNode;

UCLASS()
class UDungeonForgeEditorGraphNodeBase : public UEdGraphNode {
	GENERATED_BODY()
public:
	void Construct(UDungeonForgeGraphNode* InForgeNode);
	
	// ~Begin UObject interface
	virtual void BeginDestroy() override;
	virtual void PostTransacted(const FTransactionObjectEvent& TransactionEvent) override;
	// ~End UObject interface
	
	// ~Begin UEdGraphNode interface
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	// ~End UEdGraphNode interface

	
	UDungeonForgeGraphNode* GetForgeNode() { return ForgeNode; }
	const UDungeonForgeGraphNode* GetForgeNode() const { return ForgeNode; }
	void PostCopy();
	void RebuildAfterPaste();
	virtual void PostPaste();
	virtual FLinearColor GetNodeTitleColor() const override;

	/** Whether this OutputPin can be connected to this InputPin */
	virtual bool IsCompatible(const UDungeonForgeGraphPin* InputPin, const UDungeonForgeGraphPin* OutputPin, FText& OutReason) const;

	
	DECLARE_DELEGATE(FOnDungeonForgeEditorGraphNodeChanged);
	FOnDungeonForgeEditorGraphNodeChanged OnNodeChangedDelegate;

	/** Pulls current errors/warnings state from Forge subsystem. */
	EDungeonForgeGraphChangeType UpdateErrorsAndWarnings();
	
	/** Increase deferred reconstruct counter, calls to ReconstructNode will flag reconstruct to happen when count hits zero */
	void EnableDeferredReconstruct();
	/** Decrease deferred reconstruct counter, ReconstructNode will be called if counter hits zero and the node is flagged for reconstruction  */
	void DisableDeferredReconstruct();
	
protected:
	void OnNodeChanged(UDungeonForgeGraphNode* InNode, EDungeonForgeGraphChangeType ChangeType);
	void UpdateCommentBubblePinned();
	void UpdatePosition();

	void CreatePins(const TArray<UDungeonForgeGraphPin*>& InInputPins, const TArray<UDungeonForgeGraphPin*>& InOutputPins);

	// Performs potentially custom logic when there's a change that would trigger a reconstruct (needed for linked nodes like the named reroutes)
	virtual void ReconstructNodeOnChange() { ReconstructNode(); }

	// Custom logic to hide some pins to the user (by not creating a UI pin, even if the model pin exists).
	// Useful for deprecation
	virtual bool ShouldCreatePin(const UDungeonForgeGraphPin* InPin) const;

	// Returns the appropriate pin name to allow for some flexibility
	virtual FText GetPinFriendlyName(const UDungeonForgeGraphPin* InPin) const;

	static FEdGraphPinType GetPinType(const UDungeonForgeGraphPin* InPin);
	
protected:
	UPROPERTY()
	TObjectPtr<UDungeonForgeGraphNode> ForgeNode = nullptr;
	
	int32 DeferredReconstructCounter = 0;
	bool bDeferredReconstruct = false;
};



/** Disables reconstruct on nodes (or from a pin) and re-enables in destructor. */
struct FDungeonForgeDeferNodeReconstructScope
{
	explicit FDungeonForgeDeferNodeReconstructScope(UEdGraphPin* FromPin)
		: Node(FromPin ? Cast<UDungeonForgeEditorGraphNodeBase>(FromPin->GetOwningNode()) : nullptr)
	{
		if (Node)
		{
			Node->EnableDeferredReconstruct();
		}
	}

	explicit FDungeonForgeDeferNodeReconstructScope(UDungeonForgeEditorGraphNodeBase* InNode)
		: Node(InNode)
	{
		if (Node)
		{
			Node->EnableDeferredReconstruct();
		}
	}

	FDungeonForgeDeferNodeReconstructScope(const FDungeonForgeDeferNodeReconstructScope&) = delete;
	FDungeonForgeDeferNodeReconstructScope(FDungeonForgeDeferNodeReconstructScope&& Other)
	{
		Node = Other.Node;
		Other.Node = nullptr;
	}

	FDungeonForgeDeferNodeReconstructScope& operator=(const FDungeonForgeDeferNodeReconstructScope&) = delete;
	FDungeonForgeDeferNodeReconstructScope& operator=(FDungeonForgeDeferNodeReconstructScope&& Other)
	{
		Swap(Node, Other.Node);
		return *this;
	}

	~FDungeonForgeDeferNodeReconstructScope()
	{
		if (Node)
		{
			Node->DisableDeferredReconstruct();
			Node = nullptr;
		}
	}

private:
	UDungeonForgeEditorGraphNodeBase* Node = nullptr;
};

