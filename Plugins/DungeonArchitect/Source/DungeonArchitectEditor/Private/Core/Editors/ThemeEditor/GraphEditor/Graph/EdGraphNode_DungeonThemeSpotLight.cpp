//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeSpotLight.h"

#include "Core/Utils/AssetUtils.h"

#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonLight"

UEdGraphNode_DungeonThemeSpotLight::UEdGraphNode_DungeonThemeSpotLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    SpotLight = ObjectInitializer.CreateDefaultSubobject<USpotLightComponent>(this, "SpotLight");

    SpotLight->CastShadows = false;
    SpotLight->CastDynamicShadows = false;
    SpotLight->CastStaticShadows = false;
}

UObject* UEdGraphNode_DungeonThemeSpotLight::GetNodeAssetObject(UObject* Outer) const {
    UObject* AssetObject = NewObject<USpotLightComponent>(Outer, NAME_None, RF_NoFlags, SpotLight);
    return AssetObject;
}

TArray<UObject*> UEdGraphNode_DungeonThemeSpotLight::GetThumbnailAssetObjects() const {
    return { FAssetUtils::GetSpotLightSprite() };
}

#undef LOCTEXT_NAMESPACE

