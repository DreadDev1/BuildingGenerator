//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphSchema_DungeonTheme.h"

#include "Core/Common/Utils/DungeonGraphUtils.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/DungeonThemeConnectionDrawingPolicy.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorTemplate.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMesh.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMeshList.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeParticleSystem.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemePointLight.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeSpotLight.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphSchema_DungeonThemeExtensions.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"

#define LOCTEXT_NAMESPACE "EdGraphSchema_DungeonTheme"

/////////////////////////////////////////////////////////

int32 UEdGraphSchema_DungeonTheme::CurrentCacheRefreshID = 0;

UEdGraphSchema_DungeonTheme::UEdGraphSchema_DungeonTheme(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
}

void UEdGraphSchema_DungeonTheme::GetActionList(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions,
                                               const UEdGraph* Graph, UEdGraph* OwnerOfTemporaries, bool bShowNewMesh,
                                               bool bShowNewMarker, bool bShowMarkerEmitters) const {
    // Add mesh node
    if (bShowNewMesh) {
        FDungeonSchemaUtils::AddAction<UEdGraphNode_DungeonThemeMesh>(
            TEXT("Add Mesh Node"), TEXT("Add a static mesh node to the theme graph"), OutActions, OwnerOfTemporaries);
        FDungeonSchemaUtils::AddAction<UEdGraphNode_DungeonThemeActorTemplate>(
            TEXT("Add Blueprint Node"), TEXT("Add an actor blueprint node to the theme graph"), OutActions, OwnerOfTemporaries);
        FDungeonSchemaUtils::AddAction<UEdGraphNode_DungeonThemeMeshList>(
            TEXT("Add Mesh/BP Collection Node"), TEXT("Add a list of static meshes and blueprints to have one of them selected at random"), OutActions, OwnerOfTemporaries);
        FDungeonSchemaUtils::AddAction<UEdGraphNode_DungeonThemePointLight>(
            TEXT("Add Point Light Node"), TEXT("Add a point light node to the theme graph"), OutActions,
            OwnerOfTemporaries);
        FDungeonSchemaUtils::AddAction<UEdGraphNode_DungeonThemeSpotLight>(
            TEXT("Add Spot Light Node"), TEXT("Add a spot light node to the theme graph"), OutActions,
            OwnerOfTemporaries);
        FDungeonSchemaUtils::AddAction<UEdGraphNode_DungeonThemeParticleSystem>(
            TEXT("Add Particle System Node"), TEXT("Add a particle system node to the theme graph"), OutActions,
            OwnerOfTemporaries);
    }
    // Add marker node
    if (bShowNewMarker) {
        const FText MenuDesc = FText::FromString("Add Marker Node");
        TSharedPtr<FDungeonSchemaAction_NewNode> NewMarkerNodeAction = FDungeonSchemaUtils::AddNewNodeAction(
            OutActions, FText::FromString(TEXT("Dungeon")), MenuDesc,
            FText::FromString(TEXT("Add a marker reference")));
        UEdGraphNode_DungeonThemeMarker* MarkerNode = NewObject<UEdGraphNode_DungeonThemeMarker>(OwnerOfTemporaries);
        NewMarkerNodeAction->NodeTemplate = MarkerNode;
    }

    // Add comment node
    FDungeonSchemaUtils::AddCustomAction<FDungeonThemeSchemaAction_NewComment>(TEXT("Add Comment Node"), TEXT("Add a comment node"), OutActions);
    
    // Add marker emitter node
    if (bShowMarkerEmitters) {
        TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
        Graph->GetNodesOfClass<UEdGraphNode_DungeonThemeMarker>(MarkerNodes);

        for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerNodes) {
            const FText MenuDesc = FText::FromString("Add Marker Emitter Node: " + MarkerNode->MarkerName);
            TSharedPtr<FDungeonSchemaAction_NewNode> NewMarkerEmitterNodeAction = FDungeonSchemaUtils::AddNewNodeAction(
                OutActions, FText::FromString(TEXT("Marker Emitters")), MenuDesc,
                FText::FromString(TEXT("Add a marker emitter")));
            UEdGraphNode_DungeonThemeMarkerEmitter* MarkerEmitterNode = NewObject<UEdGraphNode_DungeonThemeMarkerEmitter>(
                OwnerOfTemporaries);
            MarkerEmitterNode->ParentMarker = MarkerNode;
            NewMarkerEmitterNodeAction->NodeTemplate = MarkerEmitterNode;
        }
    }

    UEdGraphSchema_DungeonThemeExtensions::Get().CreateCustomActions(OutActions, Graph, OwnerOfTemporaries, bShowNewMesh,
                                                         bShowNewMarker, bShowMarkerEmitters);
}


void UEdGraphSchema_DungeonTheme::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const {
    FFormatNamedArguments Args;
    const FName AttrName("Attributes");
    Args.Add(TEXT("Attribute"), FText::FromName(AttrName));
    const UEdGraphPin* FromPin = ContextMenuBuilder.FromPin;
    bool bShowNewMesh = true;
    bool bShowNewMarker = true;
    bool bShowMarkerEmitters = true;
    if (FromPin) {

        if (FromPin->Direction == EGPD_Output) {
            if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Mesh) {
                // Show mesh, since this is coming out from a mesh pin
                bShowNewMesh = true;
                bShowNewMarker = false;
                bShowMarkerEmitters = false;
            }
            else if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Marker) {
                // Show markers emitters, since the output type is a marker
                bShowNewMesh = false;
                bShowNewMarker = false;
                bShowMarkerEmitters = true;
            }
        }
        else {
            // FromPin->Direction == EEdGraphPinDirection::EGPD_Input
            if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Mesh) {
                // Show marker, since this is coming up from an mesh node
                bShowNewMesh = false;
                bShowNewMarker = true;
                bShowMarkerEmitters = false;
            }
            else if (FromPin->PinType.PinCategory == FDungeonThemePinDataTypes::PinType_Marker) {
                // Show only new mesh creation option, since this is coming up from a marker emitter
                bShowNewMesh = true;
                bShowNewMarker = false;
                bShowMarkerEmitters = false;
            }
        }
    }

    const UEdGraph* Graph = ContextMenuBuilder.CurrentGraph;
    TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
    GetActionList(Actions, Graph, ContextMenuBuilder.OwnerOfTemporaries, bShowNewMesh, bShowNewMarker,
                  bShowMarkerEmitters);

    for (TSharedPtr<FEdGraphSchemaAction> Action : Actions) {
        ContextMenuBuilder.AddAction(Action);
    }
}

namespace DA::Private::ThemeSchema {
    TArray<UEdGraphNode_DungeonThemeBase*> GetOutgoingNodes(const UEdGraphPin* A, const UEdGraphPin* B, const UEdGraphNode_DungeonThemeBase* Node) {
        TArray<UEdGraphNode_DungeonThemeBase*> Result;
        UEdGraphPin* OutPin = Node->GetOutputPin();
        if (OutPin) {
            for (const UEdGraphPin* InputPin : OutPin->LinkedTo) {
                UEdGraphNode_DungeonThemeBase* NextNode = Cast<UEdGraphNode_DungeonThemeBase>(InputPin->GetOwningNode());
                if (NextNode) {
                    Result.Add(NextNode);
                }
            }
        }

        if (OutPin == A) {
            UEdGraphNode_DungeonThemeBase* NextNode = Cast<UEdGraphNode_DungeonThemeBase>(B->GetOwningNode());
            if (NextNode) Result.Add(NextNode);
        }
        if (OutPin == B) {
            UEdGraphNode_DungeonThemeBase* NextNode = Cast<UEdGraphNode_DungeonThemeBase>(A->GetOwningNode());
            if (NextNode) Result.Add(NextNode);
        }
        return Result;
    }
}

UEdGraphNode* FDungeonThemeSchemaAction_NewComment::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode) {
    UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();
    FVector2D SpawnLocation = Location;
    FSlateRect Bounds;
    if (FDungeonArchitectThemeEditorUtils::GetBoundsForSelectedNodes(ParentGraph, Bounds, 50.0f)) {
        CommentTemplate->SetBounds(Bounds);
        SpawnLocation.X = CommentTemplate->NodePosX;
        SpawnLocation.Y = CommentTemplate->NodePosY;
    }
    else {
        const FVector2D Size(250, 200);
        CommentTemplate->SetBounds(FSlateRect(Location, Location + Size));
        SpawnLocation.X = CommentTemplate->NodePosX;
        SpawnLocation.Y = CommentTemplate->NodePosY;
    }
    

    return FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation);
}

bool FDungeonThemeGraphCycleDetector::HasCycles(const UEdGraphPin* A, const UEdGraphPin* B, TArray<UEdGraphNode_DungeonThemeBase*>& OutCyclePath) {
    if (!A || !B) return false;
    TSet<UEdGraphNode_DungeonThemeMarker*> MarkerTreesToCheck;
    MarkerTreesToCheck.Append(FDungeonArchitectThemeEditorUtils::FindParentMarkers(A->GetOwningNode()));
    MarkerTreesToCheck.Append(FDungeonArchitectThemeEditorUtils::FindParentMarkers(B->GetOwningNode()));

    for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerTreesToCheck) {
        OutCyclePath = {};
        if (FindCycle(A, B, MarkerNode, OutCyclePath)) {
            return true;
        }
    }
    return false;
}

bool FDungeonThemeGraphCycleDetector::FindCycle(const UEdGraphPin* A, const UEdGraphPin* B, UEdGraphNode_DungeonThemeBase* InNode, TArray<UEdGraphNode_DungeonThemeBase*>& OutPath) {
    if (OutPath.Contains(InNode)) {
        return true;
    }

    OutPath.Add(InNode);
    TArray<UEdGraphNode_DungeonThemeBase*> OutgoingNodes = DA::Private::ThemeSchema::GetOutgoingNodes(A, B, InNode);
    for (UEdGraphNode_DungeonThemeBase* OutgoingNode : OutgoingNodes) {
        if (FindCycle(A, B, OutgoingNode, OutPath)) {
            return true;
        }
    }

    if (UEdGraphNode_DungeonThemeMarkerEmitter* EmitterNode = Cast<UEdGraphNode_DungeonThemeMarkerEmitter>(InNode)) {
        // Traverse the emitted marker node tree
        if (EmitterNode->ParentMarker) {
            if (FindCycle(A, B, EmitterNode->ParentMarker, OutPath)) {
                return true;
            }
        }
    }

    // Didn't find a cycle along this subtree, try another path
    OutPath.Remove(InNode);
    return false;
}

const FPinConnectionResponse UEdGraphSchema_DungeonTheme::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const {
    // Make sure the input is connecting to an output
    if (A->Direction == B->Direction) {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
    }
    
    auto CombineNodeText = [](const TArray<UEdGraphNode_DungeonThemeBase*>& InNodes, const FString& Separator) {
        auto GetNodeText = [](const UEdGraphNode_DungeonThemeBase* InNode) {
            FString NodeText;
            if (const UEdGraphNode_DungeonThemeMarker* MarkerNode = Cast<UEdGraphNode_DungeonThemeMarker>(InNode)) {
                NodeText = "Marker:" + MarkerNode->MarkerName;
            }
            else if (const UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(InNode)) {
                if (ActorNode->GetThumbnailAssetObjects().Num() > 0) {
                    NodeText = "Asset:" + ActorNode->GetThumbnailAssetObjects()[0]->GetName();
                }
                else {
                    NodeText = TEXT("ActorNode");
                }
            }
            else {
                NodeText = "Node";
            }
            
            NodeText = "[" + NodeText + "]";
            return NodeText;
        };

        FString Result;
        TArray<UEdGraphNode_DungeonThemeBase*> Nodes = InNodes.FilterByPredicate([](const UEdGraphNode_DungeonThemeBase* InNode) { return IsValid(InNode); });
        if (Nodes.Num() > 0) {
            UEdGraphNode_DungeonThemeBase* FirstNode = Nodes[0];
            Nodes.Add(FirstNode);
            for (const UEdGraphNode_DungeonThemeBase* Node : Nodes) {
                if (!Node || Node->IsA<UEdGraphNode_DungeonThemeMarkerEmitter>()) {
                    continue;
                }
            
                FString NodeText = GetNodeText(Node);
                if (Result.Len() > 0) {
                    Result += Separator;
                }
                Result += NodeText;
            }
        }
        else {
            Result = "Invalid";
        }
        
        return Result;
    };
    
    // Make sure we don't have a cycle formed by this link
    TArray<UEdGraphNode_DungeonThemeBase*> CyclePath;
    if (FDungeonThemeGraphCycleDetector::HasCycles(A, B, CyclePath)) {
        FString CycleString = CombineNodeText(CyclePath, " > ");
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed. Contains a cycle: " + CycleString));
    }
    
    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

class FConnectionDrawingPolicy* UEdGraphSchema_DungeonTheme::CreateConnectionDrawingPolicy(
        int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
        class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
    return new FDungeonThemeConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UEdGraphSchema_DungeonTheme::GetPinTypeColor(const FEdGraphPinType& PinType) const {
    return FColor::Yellow;
}

bool UEdGraphSchema_DungeonTheme::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const {
    return false;
}


bool UEdGraphSchema_DungeonTheme::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const {
    bool ConnectionMade = UEdGraphSchema::TryCreateConnection(A, B);
    if (ConnectionMade) {
        UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;
        UEdGraphNode_DungeonThemeBase* OutputNode = Cast<UEdGraphNode_DungeonThemeBase>(OutputPin->GetOwningNode());
        if (OutputNode) {
            OutputNode->UpdateChildExecutionOrder();
            OutputNode->GetGraph()->NotifyGraphChanged();
        }

    }

    return ConnectionMade;
}

TSharedPtr<FEdGraphSchemaAction> UEdGraphSchema_DungeonTheme::GetCreateCommentAction() const {
    return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FDungeonThemeSchemaAction_NewComment));
}

bool UEdGraphSchema_DungeonTheme::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const {
    return InVisualizationCacheID != CurrentCacheRefreshID;
}

int32 UEdGraphSchema_DungeonTheme::GetCurrentVisualizationCacheID() const {
    return CurrentCacheRefreshID;
}

void UEdGraphSchema_DungeonTheme::ForceVisualizationCacheClear() const {
    CurrentCacheRefreshID++;
}


#undef LOCTEXT_NAMESPACE

