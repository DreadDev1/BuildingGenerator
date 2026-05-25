//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "Components/PointLightComponent.h"
#include "EdGraphNode_DungeonThemePointLight.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemePointLight : public UEdGraphNode_DungeonThemeActorBase {
    GENERATED_UCLASS_BODY()

public:
    virtual UObject* GetNodeAssetObject(UObject* Outer) const override;
    virtual TArray<UObject*> GetThumbnailAssetObjects() const override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<UPointLightComponent> PointLight;
};

