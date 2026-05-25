//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTask.h"
#include "CellFlowLayoutTaskCreateCellsBase.generated.h"

class UDAFlowCellGenerator;
class UDAFlowCellGraph;
class UCellFlowLayoutGraph;

UCLASS(Abstract)
class UCellFlowLayoutTaskCreateCellsBase : public UBaseFlowLayoutTask {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	FIntPoint WorldSize = { 30, 30 };

	UPROPERTY(EditAnywhere, Category = "CellFlow")
	float MinGroupArea = 20;
	
	/** Should the chunk shape be irregular or blobby */
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	float ClusterBlobbiness{0.5f};
	
public:
	UCellFlowLayoutTaskCreateCellsBase();
	virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const override;
	virtual bool GetParameter(const FString& InParameterName, FDAAttribute& OutValue) override;
	virtual bool SetParameter(const FString& InParameterName, const FDAAttribute& InValue) override;
	virtual bool SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) override;

private:
	void GenerateCells(UDAFlowCellGraph* InCellGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const;
	virtual void GenerateCellsImpl(UDAFlowCellGraph* InCellGraph, const FFlowExecutionInput& Input, FFlowExecNodeStatePtr OutputState) const {}

	void FinalizeCells(UDAFlowCellGraph* InCellGraph) const;
	virtual void FinalizeCellsImpl(UDAFlowCellGraph* InCellGraph) const;
	
	static void CreateGraphs(UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph, const FRandomStream& InRandom);
};

