//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "Templates/SubclassOf.h"
#include "EdGraphNode_DungeonThemeActorTemplate.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeActorTemplate : public UEdGraphNode_DungeonThemeActorBase {
    GENERATED_BODY()

public:
    virtual UObject* GetNodeAssetObject(UObject* Outer) const override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon, meta = (OnlyPlaceable, AllowPrivateAccess = "true", ForceRebuildProperty = "ActorTemplate"))
    TSubclassOf<AActor> ClassTemplate;

    /** Property to point to the template child actor for details panel purposes */
    UPROPERTY(VisibleDefaultsOnly, Export, Category = Dungeon, meta = (ShowInnerProperties))
    TObjectPtr<AActor> ActorTemplate;

    virtual TArray<UObject*> GetThumbnailAssetObjects() const override;

public:
    //~ Begin Object Interface.
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
    virtual void OnThemeEditorLoaded() override;
#endif  // WITH_EDITOR
    //virtual void Serialize(FArchive& Ar) override;
    //~ End Object Interface.

    void SetTemplateClass(TSubclassOf<AActor> InClass, AActor* InTemplate = {});
    void SetTemplateFromAsset(UObject* AssetObject, class UActorFactory* ActorFactory);
};

