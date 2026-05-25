//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"

#include "Particles/ParticleSystem.h"
#include "EdGraphNode_DungeonThemeParticleSystem.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeParticleSystem : public UEdGraphNode_DungeonThemeActorBase {
    GENERATED_BODY()

public:
    virtual UObject* GetNodeAssetObject(UObject* Outer) const override { return ParticleSystem; }

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<UParticleSystem> ParticleSystem;
};

