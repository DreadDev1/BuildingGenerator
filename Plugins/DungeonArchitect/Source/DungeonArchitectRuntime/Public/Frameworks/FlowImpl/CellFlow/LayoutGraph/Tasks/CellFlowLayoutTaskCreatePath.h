//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskCreatePath.h"
#include "CellFlowLayoutTaskCreatePath.generated.h"

UCLASS(Meta = (AbstractTask, Title = "Create Path", Tooltip = "Create a path on an existing network", MenuPriority = 1200))
class UCellFlowLayoutTaskCreatePath  : public UBaseFlowLayoutTaskCreatePath {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "CellFlow", meta=(UIMin=1, UIMax=10))
	int32 CeilingHeightMin = 1;
	
	UPROPERTY(EditAnywhere, Category = "CellFlow", meta=(UIMin=1, UIMax=10))
	int32 CeilingHeightMax = 4;

public:
	virtual void ExtendPathNodes(FFlowExecNodeStatePtr InNodeState, const FFlowAGGrowthState& InPathState, const FRandomStream& InRandom, const FFlowAbstractGraphQuery& InGraphQuery) const override;
	
protected:
	virtual UFlowLayoutNodeCreationConstraint* GetNodeCreationConstraintLogic(ADungeon* InDungeon) const override;
	
};

