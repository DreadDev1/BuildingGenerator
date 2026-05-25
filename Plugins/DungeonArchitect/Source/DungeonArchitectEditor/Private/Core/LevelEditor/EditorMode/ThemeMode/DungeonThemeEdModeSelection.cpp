//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeSelection.h"

#include "Core/Dungeon.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

void FDungeonThemeEdModeActorSelection::Initialize(AActor* InSelectedActor, const UDungeonThemeAsset* InThemeAsset, const ADungeon* InDungeon) {
	UEdGraphNode_DungeonThemeActorBase* ThemeNode = InThemeAsset
		? FDungeonArchitectThemeEditorUtils::FindThemeNodeFromSpawnedActor(InSelectedActor, InThemeAsset->UpdateGraph)
		: nullptr;
	
	Initialize(InSelectedActor, ThemeNode, InDungeon);
}

void FDungeonThemeEdModeActorSelection::Initialize(AActor* InSelectedActor, UEdGraphNode_DungeonThemeActorBase* InActorNode, const ADungeon* InDungeon) {
	SelectedActor = InSelectedActor;
	SelectedActorThemeNode = InActorNode;

	ParentMarkerNode = InActorNode
		? FDungeonArchitectThemeEditorUtils::FindParentMarker(InActorNode)
		: nullptr;

	UDungeonModel* DungeonModel = InDungeon ? InDungeon->GetModel() : nullptr;
	if (InSelectedActor && DungeonModel) {
		bCachedWorldMarkerValid = FDungeonUtils::GetSpawnedActorMarkerInfo(InSelectedActor, DungeonModel, CachedWorldMarker);
	}
	else {
		bCachedWorldMarkerValid = false;
	}
}

void FDungeonThemeEdModeActorSelection::Initialize(UEdGraphNode_DungeonThemeMarker* InMarkerNode, const FDungeonMarkerInstance& InMarkerInstance) {
	SelectedActor = nullptr;
	SelectedActorThemeNode = nullptr;
	ParentMarkerNode = InMarkerNode;
	bCachedWorldMarkerValid = true;
	CachedWorldMarker = InMarkerInstance;
}

