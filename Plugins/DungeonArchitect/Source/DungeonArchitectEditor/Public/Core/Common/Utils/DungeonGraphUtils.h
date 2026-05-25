//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "DungeonGraphUtils.generated.h"

class UEdGraphNode;

/** Action to add a node to the graph */
USTRUCT()
struct DUNGEONARCHITECTEDITOR_API FDungeonSchemaAction_NewNode : public FEdGraphSchemaAction {
    GENERATED_USTRUCT_BODY();

    /** Template of node we want to create */
    UPROPERTY()
    TObjectPtr<UEdGraphNode> NodeTemplate;


    FDungeonSchemaAction_NewNode()
        : FEdGraphSchemaAction()
          , NodeTemplate(nullptr) {
    }

    FDungeonSchemaAction_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip,
                                 const int32 InGrouping)
        : FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
          , NodeTemplate(nullptr) {
    }

    // FEdGraphSchemaAction interface
    virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
    virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, FVector2D Location, bool bSelectNewNode = true) override;
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    // End of FEdGraphSchemaAction interface

    template <typename NodeType>
    static NodeType* SpawnNodeFromTemplate(class UEdGraph* ParentGraph, NodeType* InTemplateNode,
                                           const FVector2D Location, bool bSelectNewNode = true) {
        FDungeonSchemaAction_NewNode Action;
        Action.NodeTemplate = InTemplateNode;

        return Cast<NodeType>(Action.PerformAction(ParentGraph, nullptr, Location, bSelectNewNode));
    }
};

class DUNGEONARCHITECTEDITOR_API FDungeonGraphUtils {
public:
    template <typename T>
    static TArray<T*> GetConnectedNodes(UEdGraphPin* Pin) {
        TArray<T*> Result;
        for (UEdGraphPin* OtherPin : Pin->LinkedTo) {
            T* OtherNode = Cast<T>(OtherPin->GetOwningNode());
            if (OtherNode) {
                Result.Add(OtherNode);
            }
        }
        return Result;
    }

    template <typename T>
    static const T* FindInHierarchy(const UObject* Object) {
        while (Object != nullptr) {
            if (Object->IsA<T>()) {
                return Cast<const T>(Object);
            }
            Object = Object->GetOuter();
        }
        return nullptr;
    }

    static TArray<UEdGraphNode*> GetAllNodesInHierarchy(const UEdGraphNode* InRootNode) {
        TArray<UEdGraphNode*> Result;
        TSet<const UEdGraphNode*> Visited;
            
        if (!InRootNode) {
            return Result;
        }

        struct Local {
            static void TraverseNodes(const UEdGraphNode* Node, TArray<UEdGraphNode*>& OutNodes, TSet<const UEdGraphNode*>& VisitedNodes) {
                if (!Node || VisitedNodes.Contains(Node)) {
                    return;
                }

                // Mark as visited to prevent cycles
                VisitedNodes.Add(Node);
                OutNodes.Add(const_cast<UEdGraphNode*>(Node));

                for (UEdGraphPin* Pin : Node->Pins) {
                    if (Pin && Pin->Direction == EGPD_Output) {
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo) {
                            if (LinkedPin) {
                                UEdGraphNode* ConnectedNode = LinkedPin->GetOwningNode();
                                TraverseNodes(ConnectedNode, OutNodes, VisitedNodes);
                            }
                        }
                    }
                }
            }
        };

        Local::TraverseNodes(InRootNode, Result, Visited);
        return Result;
    }

    template <typename T>
    static T* FindInHierarchy(UObject* Object) {
        while (Object != nullptr) {
            if (Object->IsA<T>()) {
                return Cast<T>(Object);
            }
            Object = Object->GetOuter();
        }
        return nullptr;
    }

    static UEdGraphPin* GetFirstOutputPin(UEdGraphNode* InNode) {
        if (InNode) {
            for (UEdGraphPin* Pin : InNode->Pins) {
                if (Pin && Pin->Direction == EGPD_Output) {
                    return Pin;
                }
            }
        }
        return nullptr;
    }
    static UEdGraphPin* GetFirstInputPin(UEdGraphNode* InNode) {
        if (InNode) {
            for (UEdGraphPin* Pin : InNode->Pins) {
                if (Pin && Pin->Direction == EGPD_Input) {
                    return Pin;
                }
            }
        }
        return nullptr;
    }

};

class DUNGEONARCHITECTEDITOR_API FDungeonSchemaUtils {
public:
    template <typename T>
    static void AddAction(FString Title, FString Tooltip, TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions,
                          UEdGraph* OwnerOfTemporaries, int32 Priority = 0) {
        const FText MenuDesc = FText::FromString(Title);
        const FText Category = FText::FromString(TEXT("Dungeon"));
        const FText TooltipText = FText::FromString(Tooltip);
        TSharedPtr<FDungeonSchemaAction_NewNode> NewActorNodeAction = AddNewNodeAction(OutActions, Category, MenuDesc, TooltipText, Priority);
        T* ActorNode = NewObject<T>(OwnerOfTemporaries);
        NewActorNodeAction->NodeTemplate = ActorNode;
    }

    static TSharedPtr<FDungeonSchemaAction_NewNode> AddNewNodeAction(
            TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, const FText& Category, const FText& MenuDesc, const FText& Tooltip, int32 Priority = 0);
    
    template <typename TAction>
    static void AddCustomAction(FString Title, FString Tooltip, TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions) {
        const FText MenuDesc = FText::FromString(Title);
        const FText Category = FText::FromString(TEXT("Dungeon"));
        const FText TooltipText = FText::FromString(Tooltip);
        TSharedPtr<TAction> NewAction = MakeShared<TAction>(Category, MenuDesc, TooltipText, 0);
        OutActions.Add(NewAction);
    }

};

