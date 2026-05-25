//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsBase.h"
#include "Frameworks/Lib/Geometry/DCEL.h"
#include "Frameworks/Lib/Voronoi/VoronoiBeachLine.h"
#include "CellFlowLayoutTaskCreateCellsVoronoi.generated.h"

UCLASS(Meta = (AbstractTask, Title="Create Cells (Voronoi)", Tooltip="Create the initial voronoi cell graph to work on", MenuPriority = 1001))
class DUNGEONARCHITECTRUNTIME_API UCellFlowLayoutTaskCreateCellsVoronoi : public UCellFlowLayoutTaskCreateCellsBase {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int NumPoints{100};

	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int NumRelaxIterations{5}; 

	UPROPERTY(EditAnywhere, Category = "CellFlow")
	bool bDisableBoundaryCells{true};
	
	/** We don't want to connect to an adjacent cell if the edge connecting to it is too small.  Control how small you want the ratio to be from here. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow")
	float EdgeConnectionThreshold{0.2f}; 
	
private:
	virtual void GenerateCellsImpl(UDAFlowCellGraph* InCellGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const override;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UCellFlowVoronoiGraph : public UObject, public IFlowExecCloneableState {
	GENERATED_BODY()
public:
	virtual void CloneFromStateObject(const UObject* SourceObject) override;
	
public:
	TArray<FVector2d> Sites;
	DA::FVoronoiGraph VoronoiGraph;
	DA::DCELGraph DGraph;
	
    static const FName StateTypeID;
};

