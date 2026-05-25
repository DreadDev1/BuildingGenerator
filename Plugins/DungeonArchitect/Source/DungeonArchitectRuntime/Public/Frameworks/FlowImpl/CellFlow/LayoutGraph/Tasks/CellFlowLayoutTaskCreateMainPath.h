//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskCreateMainPath.h"
#include "CellFlowLayoutTaskCreateMainPath.generated.h"

UCLASS(Meta = (AbstractTask, Title = "Create Main Path", Tooltip = "Create a main path with spawn and goal", MenuPriority = 1100))
class UCellFlowLayoutTaskCreateMainPath : public UBaseFlowLayoutTaskCreateMainPath {
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


