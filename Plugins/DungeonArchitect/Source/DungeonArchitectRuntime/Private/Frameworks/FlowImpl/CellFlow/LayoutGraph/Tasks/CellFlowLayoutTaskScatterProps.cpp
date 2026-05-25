//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskScatterProps.h"

#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"

void UCellFlowLayoutTaskScatterProps::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
	check(Input.IncomingNodeOutputs.Num() == 1);

	Output.State = Input.IncomingNodeOutputs[0].State->Clone();

	UDAFlowCellGraph* CellGraph = Output.State->GetState<UDAFlowCellGraph>(UDAFlowCellGraph::StateTypeID);
	CellGraph->ScatterSettings.Add(Settings);
	
	Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

