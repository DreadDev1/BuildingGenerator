//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemePointLight.h"

#include "Core/Utils/AssetUtils.h"

#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonPointLight"

UEdGraphNode_DungeonThemePointLight::UEdGraphNode_DungeonThemePointLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    PointLight = ObjectInitializer.CreateDefaultSubobject<UPointLightComponent>(this, "PointLight");

    PointLight->CastShadows = false;
    PointLight->CastDynamicShadows = false;
    PointLight->CastStaticShadows = false;
}

UObject* UEdGraphNode_DungeonThemePointLight::GetNodeAssetObject(UObject* Outer) const {
    UObject* AssetObject = NewObject<UPointLightComponent>(Outer, NAME_None, RF_NoFlags, PointLight);
    return AssetObject;
}

TArray<UObject*> UEdGraphNode_DungeonThemePointLight::GetThumbnailAssetObjects() const {
    return { FAssetUtils::GetPointLightSprite() };
}

#undef LOCTEXT_NAMESPACE

