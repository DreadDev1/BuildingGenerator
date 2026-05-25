//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarker.h"

class UDungeonThemeAsset;
class UEdGraphNode_DungeonThemeMarker;
class UEdGraphNode_DungeonThemeActorBase;

class FDungeonThemeEdModeActorSelection {
public:
	void Initialize(AActor* InSelectedActor, const UDungeonThemeAsset* InThemeAsset, const ADungeon* InDungeon);
	void Initialize(AActor* InSelectedActor, UEdGraphNode_DungeonThemeActorBase* InActorNode, const ADungeon* InDungeon);
	void Initialize(UEdGraphNode_DungeonThemeMarker* InMarkerNode, const FDungeonMarkerInstance& InMarkerInstance);
	
	AActor* GetSelectedActor() const {
		return SelectedActor.Get();
	}

	UEdGraphNode_DungeonThemeActorBase* GetSelectedActorThemeNode() const {
		return SelectedActorThemeNode.Get();
	}

	UEdGraphNode_DungeonThemeMarker* GetParentMarkerNode() const {
		return ParentMarkerNode.Get();
	}

	bool IsCachedWorldMarkerValid() const {
		return bCachedWorldMarkerValid;
	}

	FDungeonMarkerInstance GetCachedWorldMarker() const {
		return CachedWorldMarker;
	}

private:
	TWeakObjectPtr<AActor> SelectedActor;
	TWeakObjectPtr<UEdGraphNode_DungeonThemeActorBase> SelectedActorThemeNode;
	TWeakObjectPtr<UEdGraphNode_DungeonThemeMarker> ParentMarkerNode;
	bool bCachedWorldMarkerValid = false;
	FDungeonMarkerInstance CachedWorldMarker{};
};

