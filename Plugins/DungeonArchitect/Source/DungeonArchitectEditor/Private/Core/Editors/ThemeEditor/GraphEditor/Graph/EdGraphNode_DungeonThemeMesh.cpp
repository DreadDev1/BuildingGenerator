//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMesh.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

UEdGraphNode_DungeonThemeMesh::UEdGraphNode_DungeonThemeMesh(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    AdvancedOptions = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, "Template");
    AdvancedOptions->SetMobility(EComponentMobility::Static);
    AdvancedOptions->SetCanEverAffectNavigation(true);
    AdvancedOptions->bCastStaticShadow = true;
}


UObject* UEdGraphNode_DungeonThemeMesh::GetNodeAssetObject(UObject* Outer) const {

    UDungeonMesh* AssetObject = NewObject<UDungeonMesh>(Outer);
    AssetObject->StaticMesh = Mesh;

    UStaticMeshComponent* AdvancedOptionsCopy = NewObject<UStaticMeshComponent>(
        Outer, NAME_None, RF_NoFlags, AdvancedOptions);
    AssetObject->Template = AdvancedOptionsCopy;
    AssetObject->MaterialOverrides = MaterialOverrides;
    AssetObject->bCanEverAffectNavigation = bCanEverAffectNavigation;
    AssetObject->BodyInstance = BodyInstance;
    AssetObject->bUseCustomCollision = bUseCustomCollision;
    
    AssetObject->CalculateHashCode();
    return AssetObject;
}

TArray<UObject*> UEdGraphNode_DungeonThemeMesh::GetThumbnailAssetObjects() const {
    return { Mesh };
}

void UEdGraphNode_DungeonThemeMesh::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    UEdGraphNode_DungeonThemeActorBase::PostEditChangeProperty(e);

    if (!e.Property) return;

    const FName PropertyName = e.Property->GetFName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UEdGraphNode_DungeonThemeMesh, Mesh)) {
        AdvancedOptions->SetStaticMesh(Mesh);
        BodyInstance = AdvancedOptions->BodyInstance;
    }
}

