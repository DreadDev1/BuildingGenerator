//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Actors/DungeonMesh.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "PhysicsEngine/BodyInstance.h"
#include "EdGraphNode_DungeonThemeMesh.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeMesh : public UEdGraphNode_DungeonThemeActorBase {
    GENERATED_UCLASS_BODY()

public:
    virtual UObject* GetNodeAssetObject(UObject* Outer) const override;
    virtual TArray<UObject*> GetThumbnailAssetObjects() const override;

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<FMaterialOverride> MaterialOverrides;

    UPROPERTY(EditAnywhere, Category = Dungeon)
    bool bCanEverAffectNavigation = true;
    
    /** Physics scene information for this component, holds a single rigid body with multiple shapes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision)
    bool bUseCustomCollision = false;
    
    /** Physics scene information for this component, holds a single rigid body with multiple shapes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision, meta=(ShowOnlyInnerProperties, SkipUCSModifiedProperties, EditCondition="bUseCustomCollision"))
    FBodyInstance BodyInstance;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Details")
    TObjectPtr<UStaticMeshComponent> AdvancedOptions;
};

