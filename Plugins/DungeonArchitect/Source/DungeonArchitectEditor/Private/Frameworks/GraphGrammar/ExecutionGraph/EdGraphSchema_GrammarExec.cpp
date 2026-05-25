//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/GraphGrammar/ExecutionGraph/EdGraphSchema_GrammarExec.h"

#include "Core/Common/Utils/DungeonGraphUtils.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/EdGraph_GrammarExec.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecEntryNode.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecNodeBase.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecRuleNode.h"
#include "Frameworks/GraphGrammar/GraphGrammar.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "EdGraphSchema_GrammarExec"

FGrammarExecGraphSupport* UEdGraphSchema_GrammarExec::ExecGraphSupport = nullptr;

UEdGraphSchema_GrammarExec::UEdGraphSchema_GrammarExec(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
}

void UEdGraphSchema_GrammarExec::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const {
    const UEdGraph_GrammarExec* Graph = Cast<UEdGraph_GrammarExec>(ContextMenuBuilder.CurrentGraph);

    // Add the node actions
    {
        TArray<TSharedPtr<FEdGraphSchemaAction>> NodeActions;
        GetNodeActionList(NodeActions, ContextMenuBuilder.OwnerOfTemporaries, Graph);

        for (TSharedPtr<FEdGraphSchemaAction> Action : NodeActions) {
            ContextMenuBuilder.AddAction(Action);
        }
    }
}

void UEdGraphSchema_GrammarExec::GetContextMenuActions(class UToolMenu* Menu,
                                                    class UGraphNodeContextMenuContext* Context) const {
    if (ExecGraphSupport) {
        ExecGraphSupport->GetContextMenuActions(Menu, Context);
    }
}

class FGrammarExecSchemaUtils {
public:
    template <typename T>
    static void AddNodeAction(const FText& InMenuDesc, TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions,
                              UEdGraph* OwnerOfTemporaries, TFunction<void(T*)> InitTemplate = TFunction<void(T*)>()) {
        const FText Category = LOCTEXT("CategoryLabel", "Rules");
        const FText Tooltip = LOCTEXT("TooltipLabel", "Add Rule node");

        T* Node = NewObject<T>(OwnerOfTemporaries);
        if (InitTemplate) {
            InitTemplate(Node);
        }

        TSharedPtr<FGrammarExecSchemaAction_NewNode> NewAction = MakeShared<FGrammarExecSchemaAction_NewNode>(
            Category, InMenuDesc, Tooltip, 0);
        NewAction->NodeTemplate = Node;
        OutActions.Add(NewAction);
    }
};


void UEdGraphSchema_GrammarExec::GetNodeActionList(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions,
                                                UEdGraph* OwnerOfTemporaries, const UEdGraph_GrammarExec* Graph) const {
    const UGraphGrammar* Grammar = FDungeonGraphUtils::FindInHierarchy<UGraphGrammar>(Graph);
    if (Grammar) {
        for (UGraphGrammarProduction* Rule : Grammar->ProductionRules) {

            FText MenuDesc;
            FFormatNamedArguments Args;
            Args.Add(TEXT("NodeName"), Rule->RuleName);
            MenuDesc = FText::Format(LOCTEXT("AddNodeTextPattern", "Add Rule: {NodeName}"), Args);

            FGrammarExecSchemaUtils::AddNodeAction<UEdGraphNode_GrammarExecRuleNode>(MenuDesc, OutActions, OwnerOfTemporaries,
                                                                           [Rule](UEdGraphNode_GrammarExecRuleNode* Node) {
                                                                               Node->Rule = Rule;
                                                                           }
            );
        }
    }
}

const FPinConnectionResponse UEdGraphSchema_GrammarExec::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const {
    // Make sure the data types match
    if (A->PinType.PinCategory != B->PinType.PinCategory) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }
    // Make sure they are not the same pins
    if (A->GetOwningNode() == B->GetOwningNode()) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }

    // Disallow connection on same direction
    if (A->Direction == EGPD_Output && B->GetOwningNode()->IsA<UEdGraphNode_GrammarExecEntryNode>()) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }

    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

FLinearColor UEdGraphSchema_GrammarExec::GetPinTypeColor(const FEdGraphPinType& PinType) const {
    return FColor::Yellow;
}

bool UEdGraphSchema_GrammarExec::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const {
    return false;
}

FConnectionDrawingPolicy* UEdGraphSchema_GrammarExec::CreateConnectionDrawingPolicy(
    int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
    class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const {
    if (ExecGraphSupport) {
        return ExecGraphSupport->CreateDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect,
                                                     InDrawElements, InGraphObj);
    }
    return UEdGraphSchema::CreateConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect,
                                                         InDrawElements, InGraphObj);
}

#if WITH_EDITOR
bool UEdGraphSchema_GrammarExec::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const {
    UEdGraphNode_GrammarExecNodeBase* NodeA = Cast<UEdGraphNode_GrammarExecNodeBase>(A->GetOwningNode());
    UEdGraphNode_GrammarExecNodeBase* NodeB = Cast<UEdGraphNode_GrammarExecNodeBase>(B->GetOwningNode());
    UEdGraphPin* OutputA = NodeA->GetOutputPin();
    UEdGraphPin* InputB = NodeB->GetInputPin();
    if (!OutputA || !InputB) {
        return false;
    }

    bool bConnectionMade = UEdGraphSchema::TryCreateConnection(OutputA, InputB);
    if (bConnectionMade && OutputA && InputB) {
        // Allow only one outgoing link
        TArray<UEdGraphPin*> LinkedPins = A->LinkedTo;
        for (UEdGraphPin* LinkedPin : LinkedPins) {
            if (LinkedPin != InputB) {
                // Break this pin
                OutputA->BreakLinkTo(LinkedPin);
            }
        }
        // Break a reverse link, if it exists
        {
            UEdGraphPin* InputA = NodeA->GetInputPin();
            UEdGraphPin* OutputB = NodeB->GetOutputPin();
            if (InputA && OutputB) {
                OutputB->BreakLinkTo(InputA);
            }
        }

        UEdGraph* Graph = A->GetOwningNode()->GetGraph();
        Graph->NotifyGraphChanged();
    }
    return bConnectionMade;
}

void UEdGraphSchema_GrammarExec::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const {
    UEdGraphSchema::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
    TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
}

void UEdGraphSchema_GrammarExec::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const {
    UEdGraphSchema::BreakSinglePinLink(SourcePin, TargetPin);
    SourcePin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
}

void UEdGraphSchema_GrammarExec::BreakNodeLinks(UEdGraphNode& TargetNode) const {
    UEdGraphSchema::BreakNodeLinks(TargetNode);
    TargetNode.GetGraph()->NotifyGraphChanged();
}
#endif // WITH_EDITOR

//////////////////////////////////////// FGrammarExecSchemaAction_NewNode ////////////////////////////////////////
UEdGraphNode* FGrammarExecSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin,
                                                           const FVector2D Location, bool bSelectNewNode /*= true*/) {
    UEdGraphNode* NewNode = FDungeonSchemaAction_NewNode::PerformAction(ParentGraph, FromPin, Location, bSelectNewNode);
    UEdGraphNode_GrammarExecRuleNode* NewRuleNode = Cast<UEdGraphNode_GrammarExecRuleNode>(NewNode);

    return NewNode;
}

#undef LOCTEXT_NAMESPACE

