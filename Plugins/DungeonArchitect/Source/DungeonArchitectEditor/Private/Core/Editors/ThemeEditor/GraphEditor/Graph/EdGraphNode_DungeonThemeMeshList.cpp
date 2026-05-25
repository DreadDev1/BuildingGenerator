//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMeshList.h"

#include "Engine/StaticMesh.h"

UObject* UEdGraphNode_DungeonThemeMeshList::GetNodeAssetObject(UObject* Outer) const {
	UDungeonMeshList* AssetObject = NewObject<UDungeonMeshList>(Outer);
	AssetObject->StaticMeshes = Meshes;
	AssetObject->ActorTemplates = Blueprints;
	AssetObject->bCanEverAffectNavigation = bCanEverAffectNavigation;
	AssetObject->CalculateHashCode();
	return AssetObject;
}

TArray<UObject*> UEdGraphNode_DungeonThemeMeshList::GetThumbnailAssetObjects() const {
	TArray<UObject*> ThumbnailObjects;
	for (const FDungeonMeshListItem& MeshInfo : Meshes) {
		ThumbnailObjects.Add(MeshInfo.StaticMesh);
	}
	for (const FDungeonActorTemplateListItem& Blueprint : Blueprints) {
		ThumbnailObjects.Add(Blueprint.ClassTemplate);
	}
	return ThumbnailObjects;
}

FLinearColor UEdGraphNode_DungeonThemeMeshList::GetBorderColor() {
	static constexpr FLinearColor BorderColor(0.2f, 0.12f, 0.08f);
	return BorderColor;
}

