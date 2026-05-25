//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeBase.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMesh.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonBase"

UEdGraphNode_DungeonThemeBase::UEdGraphNode_DungeonThemeBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), bHighlightChildNodeIndices(false) {
}

void UEdGraphNode_DungeonThemeBase::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    const TSharedPtr<FNodePropertyObserver> PropertyObserverPtr = PropertyObserver.Pin();
    if (PropertyObserverPtr.IsValid()) {
        const FName PropertyName = (e.Property != nullptr) ? e.Property->GetFName() : NAME_None;
        PropertyObserverPtr->OnPropertyChanged(this, PropertyName);
    }

    Super::PostEditChangeProperty(e);
}

struct Compare_UEdGraphNode_DungeonBase {
    bool operator()(const UEdGraphNode_DungeonThemeBase& A, const UEdGraphNode_DungeonThemeBase& B) const {
        return A.NodePosX < B.NodePosX;
    }
};

void UEdGraphNode_DungeonThemeBase::NodeConnectionListChanged() {
    UEdGraphNode::NodeConnectionListChanged();
    UpdateChildExecutionOrder();
    GetGraph()->NotifyGraphChanged();
}

void UEdGraphNode_DungeonThemeBase::UpdateChildExecutionOrder() {
    UEdGraphPin* OutputPin = GetOutputPin();
    if (!OutputPin) return;

    TArray<UEdGraphNode_DungeonThemeActorBase*> Children;

    for (UEdGraphPin* ChildPin : OutputPin->LinkedTo) {
        UEdGraphNode_DungeonThemeActorBase* ChildNode = Cast<UEdGraphNode_DungeonThemeActorBase>(ChildPin->GetOwningNode());
        if (ChildNode) {
            Children.Add(ChildNode);
        }
    }

    // Sort the sibling nodes based on the X axis
    Children.Sort(Compare_UEdGraphNode_DungeonBase());

    int32 ExecutionOrder = 1;
    for (UEdGraphNode_DungeonThemeActorBase* Child : Children) {
        Child->ExecutionOrder = ExecutionOrder;
        ExecutionOrder++;
    }
}

#undef LOCTEXT_NAMESPACE

