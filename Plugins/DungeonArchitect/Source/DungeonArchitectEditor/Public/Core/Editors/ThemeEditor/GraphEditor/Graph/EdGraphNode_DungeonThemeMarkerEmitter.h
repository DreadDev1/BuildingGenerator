//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeBase.h"
#include "EdGraphNode_DungeonThemeMarkerEmitter.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeMarkerEmitter : public UEdGraphNode_DungeonThemeBase {
    GENERATED_UCLASS_BODY()

    // Begin UEdGraphNode interface.
    virtual void AllocateDefaultPins() override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    // End UEdGraphNode interface.

public:
    virtual UEdGraphPin* GetInputPin() const override { return Pins[0]; }
    virtual UEdGraphPin* GetOutputPin() const override { return nullptr; }
    virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

public:
    UPROPERTY()
    TObjectPtr<class UEdGraphNode_DungeonThemeMarker> ParentMarker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    FTransform Offset;
};

