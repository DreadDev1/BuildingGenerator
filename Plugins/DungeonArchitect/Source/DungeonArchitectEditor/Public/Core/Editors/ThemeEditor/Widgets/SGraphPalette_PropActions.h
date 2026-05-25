//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SGraphPalette.h"

class UEdGraph;

class DUNGEONARCHITECTEDITOR_API SGraphPalette_PropActions : public SGraphPalette {
public:
    SLATE_BEGIN_ARGS(SGraphPalette_PropActions) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TWeakObjectPtr<UEdGraph>& InGraph);
    virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;
    virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* InCreateData) override;
    virtual FReply OnActionDragged(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions,
                                   const FPointerEvent& MouseEvent) override;

    void Refresh();

protected:
    TWeakObjectPtr<UEdGraph> GraphPtr;
};

