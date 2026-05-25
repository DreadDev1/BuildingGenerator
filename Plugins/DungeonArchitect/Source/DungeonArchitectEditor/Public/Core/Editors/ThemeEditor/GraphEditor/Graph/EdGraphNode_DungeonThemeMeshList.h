//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Actors/DungeonMeshList.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "EdGraphNode_DungeonThemeMeshList.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeMeshList : public UEdGraphNode_DungeonThemeActorBase {
	GENERATED_BODY()

public:
	virtual UObject* GetNodeAssetObject(UObject* Outer) const override;
	virtual TArray<UObject*> GetThumbnailAssetObjects() const override;
	virtual FLinearColor GetBorderColor() override;
	
public:
	UPROPERTY(EditAnywhere, Category = Dungeon)
	TArray<FDungeonMeshListItem> Meshes;
	
	UPROPERTY(EditAnywhere, Category = Dungeon)
	TArray<FDungeonActorTemplateListItem> Blueprints;
	
	UPROPERTY(EditAnywhere, Category = Dungeon)
	bool bCanEverAffectNavigation = true;
};

