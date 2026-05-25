//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_DungeonThemeBase.generated.h"

class DUNGEONARCHITECTEDITOR_API FNodePropertyObserver {
public:
    virtual void OnPropertyChanged(class UEdGraphNode_DungeonThemeBase* Sender, const FName& PropertyName) = 0;
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeBase : public UEdGraphNode {
    GENERATED_UCLASS_BODY()

public:
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
    virtual UEdGraphPin* GetInputPin() const { return nullptr; }
    virtual UEdGraphPin* GetOutputPin() const { return nullptr; }
    virtual void NodeConnectionListChanged() override;

    void UpdateChildExecutionOrder();

#if WITH_EDITOR
    virtual void OnThemeEditorLoaded() {
    }
#endif // WITH_EDITOR

public:
    TWeakPtr<FNodePropertyObserver> PropertyObserver;
    bool bHighlightChildNodeIndices;
};

