//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Widgets/GraphPanelNodeFactory_DungeonTheme.h"

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarkerEmitter.h"
#include "Core/Editors/ThemeEditor/Widgets/SGraphNode_DungeonActor.h"
#include "Core/Editors/ThemeEditor/Widgets/SGraphNode_DungeonMarker.h"
#include "Core/Editors/ThemeEditor/Widgets/SGraphNode_DungeonMarkerEmitter.h"

FGraphPanelNodeFactory_DungeonTheme::FGraphPanelNodeFactory_DungeonTheme() {
}

TSharedPtr<class SGraphNode> FGraphPanelNodeFactory_DungeonTheme::CreateNode(UEdGraphNode* Node) const {
    if (UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(Node)) {
        TSharedPtr<SGraphNode_DungeonActor> SNode = SNew(SGraphNode_DungeonActor, ActorNode);
        ActorNode->PropertyObserver = SNode;
        return SNode;
    }
    if (UEdGraphNode_DungeonThemeMarker* MarkerNode = Cast<UEdGraphNode_DungeonThemeMarker>(Node)) {
        TSharedPtr<SGraphNode_DungeonMarker> SNode = SNew(SGraphNode_DungeonMarker, MarkerNode);
        MarkerNode->PropertyObserver = SNode;
        return SNode;
    }
    if (UEdGraphNode_DungeonThemeMarkerEmitter* MarkerEmitterNode = Cast<UEdGraphNode_DungeonThemeMarkerEmitter>(Node)) {
        TSharedPtr<SGraphNode_DungeonMarkerEmitter> SNode = SNew(SGraphNode_DungeonMarkerEmitter, MarkerEmitterNode);
        MarkerEmitterNode->PropertyObserver = SNode;
        return SNode;
    }

    return nullptr;
}

