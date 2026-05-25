//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsBase.h"
#include "CellFlowLayoutTaskCreateCellsGrid.generated.h"

UCLASS(Abstract)
class UCellFlowLayoutTaskCreateCellsGridFilter : public UObject {
	GENERATED_BODY()
public:
	virtual void Filter(const FTransform& InTransform, const FVector& InGridSize, const FIntPoint& InTilemapSize, TArray<int>& OutCellTileMap) const PURE_VIRTUAL(, );
};

UCLASS(Meta = (AbstractTask, Title="Create Cells (Grid)", Tooltip="Create the initial grid cell graph to work on", MenuPriority = 1000))
class UCellFlowLayoutTaskCreateCellsGrid : public UCellFlowLayoutTaskCreateCellsBase {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int MinCellSize = 1;
	
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int MaxCellSize = 6;
	
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int FitIterations = 20;
	
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	float MinAspectRatio = 0.4f;
	
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	float MaxAspectRatio = 1.5f;

	
private:
	virtual void GenerateCellsImpl(UDAFlowCellGraph* InCellGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const override;
};

