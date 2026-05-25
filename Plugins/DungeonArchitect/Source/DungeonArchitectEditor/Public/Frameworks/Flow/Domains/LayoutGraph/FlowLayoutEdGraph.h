//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Common/Widgets/FlowItemOverlayDelegates.h"

#include "EdGraph/EdGraph.h"
#include "FlowLayoutEdGraph.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UFlowLayoutEdGraph : public UEdGraph {
	GENERATED_UCLASS_BODY()

public:
	TWeakObjectPtr<class UFlowAbstractGraphBase> ScriptGraph;

	FFlowItemWidgetEvent OnItemWidgetClicked;
	FGuid SelectedItemId;
};

