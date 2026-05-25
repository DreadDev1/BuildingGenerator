//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"

#include "Core/DungeonBuilder.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorTemplate.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMesh.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeParticleSystem.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemePointLight.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeSpotLight.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphSchema_DungeonTheme.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/ProceduralDungeonTransformLogic.h"

#include "ActorFactories/ActorFactory.h"
#include "AssetSelection.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMesh.h"
#include "Kismet2/BlueprintEditorUtils.h"

const int32 UEdGraph_DungeonTheme::SNAP_GRID = 16;

#define LOCTEXT_NAMESPACE "DungeonPropGraph"

const FName FDungeonThemePinDataTypes::PinType_Mesh = "mesh";
const FName FDungeonThemePinDataTypes::PinType_Marker = "marker";

UEdGraph_DungeonTheme::UEdGraph_DungeonTheme(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    Schema = UEdGraphSchema_DungeonTheme::StaticClass();
}

namespace DALib {
}



void UEdGraph_DungeonTheme::RecreateDefaultMarkerNodes(TSubclassOf<UDungeonBuilder> BuilderClass) {
    TArray<FString> DefaultMarkerNames;
    if (BuilderClass) {
        UDungeonBuilder* Builder = NewObject<UDungeonBuilder>(static_cast<UObject*>(GetTransientPackage()), BuilderClass);
        if (Builder) {
            Builder->GetDefaultMarkerNames(DefaultMarkerNames);
        }
    }
    RecreateDefaultMarkerNodes(DefaultMarkerNames);
}

void UEdGraph_DungeonTheme::RecreateDefaultMarkerNodes(const TArray<FString> InMarkerNames) {
    // Remove all the unused builder marker nodes 
    {
        TSet<UEdGraphNode_DungeonThemeMarker*> ReferencedNodes;

        // First cache all the marker emitter nodes' references
        TArray<UEdGraphNode_DungeonThemeMarkerEmitter*> EmitterNodes;
        GetNodesOfClass<UEdGraphNode_DungeonThemeMarkerEmitter>(EmitterNodes);
        for (UEdGraphNode_DungeonThemeMarkerEmitter* EmitterNode : EmitterNodes) {
            if (EmitterNode && EmitterNode->ParentMarker) {
                ReferencedNodes.Add(EmitterNode->ParentMarker);
            }
        }

        // Now grab all the marker nodes, and remove the builder created nodes that are not referenced by emitters
        TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
        GetNodesOfClass<UEdGraphNode_DungeonThemeMarker>(MarkerNodes);
        for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerNodes) {
            if (MarkerNode->bBuilderEmittedMarker && !ReferencedNodes.Contains(MarkerNode)) {
                if (MarkerNode->GetOutputPin()->LinkedTo.Num() > 0) {
                    // This node has children
                    continue;
                }

                // Remove this node from the graph
                // Break all node links first so that we don't update the material before deleting
                MarkerNode->BreakAllNodeLinks();
                FBlueprintEditorUtils::RemoveNode(nullptr, MarkerNode, true);
            }
        }
    }

    TSet<FString> ExistingMarkerNames;
    {
        TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
        GetNodesOfClass<UEdGraphNode_DungeonThemeMarker>(MarkerNodes);
        for (UEdGraphNode_DungeonThemeMarker* MarkerNode : MarkerNodes) {
            ExistingMarkerNames.Add(MarkerNode->MarkerName);
        }
    }


    int32 NumItemsPerRow = 5;
    int32 XDelta = 200;
    int32 YDelta = 400;

    int PositionIndex = 0;

    for (int i = 0; i < InMarkerNames.Num(); i++) {
        FString MarkerName = InMarkerNames[i];
        if (ExistingMarkerNames.Contains(MarkerName)) {
            continue;
        }
        ExistingMarkerNames.Add(MarkerName);

        UEdGraphNode_DungeonThemeMarker* MarkerNode = NewObject<UEdGraphNode_DungeonThemeMarker>(this);
        MarkerNode->bUserDefined = false;
        MarkerNode->bBuilderEmittedMarker = true;
        MarkerNode->Rename(nullptr, this, REN_NonTransactional);
        MarkerNode->MarkerName = MarkerName;
        this->AddNode(MarkerNode, true, false);

        MarkerNode->CreateNewGuid();
        MarkerNode->PostPlacedNewNode();
        MarkerNode->AllocateDefaultPins();
        MarkerNode->NodePosY = -1000;

        bool bOccupied = false;
        int32 X, Y;
        do {
            X = (PositionIndex % NumItemsPerRow) * XDelta;
            Y = (PositionIndex / NumItemsPerRow) * YDelta;
            PositionIndex++;
        }
        while (!ContainsFreeSpace(X, Y, 100));

        MarkerNode->NodePosX = X;
        MarkerNode->NodePosY = Y;
        MarkerNode->SnapToGrid(SNAP_GRID);
    }

}

float GetDistanceSq(int32 X0, int32 Y0, int32 X1, int32 Y1) {
    return (X0 - X1) * (X0 - X1) + (Y0 - Y1) * (Y0 - Y1);
}

bool UEdGraph_DungeonTheme::ContainsFreeSpace(int32 X, int32 Y, float Distance) {
    TArray<UEdGraphNode_DungeonThemeBase*> DungeonNodes;
    GetNodesOfClass<UEdGraphNode_DungeonThemeBase>(DungeonNodes);

    for (UEdGraphNode_DungeonThemeBase* Node : DungeonNodes) {
        float DistSq = GetDistanceSq(X, Y, Node->NodePosX, Node->NodePosY);
        if (DistSq < Distance * Distance) {
            return false;
        }
    }
    return true;
}

bool UEdGraph_DungeonTheme::IsAssetAcceptableForDrop(const UObject* AssetObject) const {
    if (!AssetObject) return false;

    const UClass* AssetClass = AssetObject->GetClass();

    bool bValidClass = AssetClass->IsChildOf<UStaticMesh>()
        || AssetClass->IsChildOf<UParticleSystem>()
        || AssetClass->IsChildOf<UClass>()
        || AssetClass->IsChildOf<AActor>()
        || AssetClass->IsChildOf<UBlueprint>();

    if (bValidClass) {
        if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject)) {
            bValidClass &= Blueprint->ParentClass->IsChildOf<AActor>();
        }
    }
    return bValidClass;
}

UEdGraphNode_DungeonThemeBase* UEdGraph_DungeonTheme::CreateNewNode(UObject* AssetObject, const FVector2D& Location) {
    UEdGraphNode_DungeonThemeBase* Node = nullptr;

    UClass* AssetClass = AssetObject->GetClass();

    if (AssetClass->IsChildOf<UStaticMesh>()) {
        UEdGraphNode_DungeonThemeMesh* MeshNode = CreateNewNode<UEdGraphNode_DungeonThemeMesh>(Location);
        MeshNode->Mesh = Cast<UStaticMesh>(AssetObject);

        Node = MeshNode;
    }
    else if (AssetClass->IsChildOf<UParticleSystem>()) {
        UEdGraphNode_DungeonThemeParticleSystem* ParticleNode = CreateNewNode<UEdGraphNode_DungeonThemeParticleSystem>(Location);
        ParticleNode->ParticleSystem = Cast<UParticleSystem>(AssetObject);

        Node = ParticleNode;
    }
    else if (AssetClass->IsChildOf<UBlueprint>()) {
        UEdGraphNode_DungeonThemeActorTemplate* ActorNode = CreateNewNode<UEdGraphNode_DungeonThemeActorTemplate>(Location);
        UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject);
        if (Blueprint) {
            ActorNode->SetTemplateClass(Cast<UClass>(Blueprint->GeneratedClass));
        }

        Node = ActorNode;
    }
    else if (AssetClass->IsChildOf<AActor>()) {
        UEdGraphNode_DungeonThemeActorTemplate* ActorNode = CreateNewNode<UEdGraphNode_DungeonThemeActorTemplate>(Location);
        ActorNode->SetTemplateClass(AssetClass);

        Node = ActorNode;
    }
    else if (AssetClass->IsChildOf<UClass>()) {
        UClass* ActorClass = Cast<UClass>(AssetObject);
        if (ActorClass->IsChildOf(ASpotLight::StaticClass())) {
            Node = CreateNewNode<UEdGraphNode_DungeonThemeSpotLight>(Location);
        }
        else if (ActorClass->IsChildOf(APointLight::StaticClass())) {
            Node = CreateNewNode<UEdGraphNode_DungeonThemePointLight>(Location);
        }
        else {
            UEdGraphNode_DungeonThemeActorTemplate* ActorNode = CreateNewNode<UEdGraphNode_DungeonThemeActorTemplate>(Location);
            ActorNode->SetTemplateClass(ActorClass);
            Node = ActorNode;
        }
    }
    else {
        // Try to create an actor from the asset factory
        const FAssetData AssetData(AssetObject);
        if (UActorFactory* ActorFactory = FActorFactoryAssetProxy::GetFactoryForAsset(AssetData)) {
            FText ErrorMessage;
            if (ActorFactory->CanCreateActorFrom(AssetData, ErrorMessage)) {
                // Create a temporary CDO of the actor class
                if (UClass* ActorClass = ActorFactory->GetDefaultActorClass(AssetData)) {
                    if (AActor* DefaultActor = ActorClass->GetDefaultObject<AActor>()) {
                        // Create an actor node using the class information
                        UEdGraphNode_DungeonThemeActorTemplate* ActorNode = CreateNewNode<UEdGraphNode_DungeonThemeActorTemplate>(Location);
                        ActorNode->SetTemplateClass(ActorClass, DefaultActor);
                        Node = ActorNode;
                    }
                }
            }
        }
    }

    return Node;
}

FDelegateHandle UEdGraph_DungeonTheme::AddOnNodePropertyChangedHandler(const FOnThemeGraphPropertyChanged::FDelegate& InHandler) {
    return OnNodePropertyChanged.Add(InHandler);
}

void UEdGraph_DungeonTheme::RemoveOnNodePropertyChangedHandler(FDelegateHandle Handle) {
    OnNodePropertyChanged.Remove(Handle);
}

void UEdGraph_DungeonTheme::NotifyNodePropertyChanged(const FEdGraphEditAction& InAction, const FPropertyChangedEvent& InChangeEvent) const {
    OnNodePropertyChanged.Broadcast(InAction, InChangeEvent);
}

#undef LOCTEXT_NAMESPACE

