//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DeferredExecutor.h"


void FDADeferredExecutor::EnqueueExecutionForNextTick(FExecutableFunction&& Function) {
	PendingExecutions.Add(MoveTemp(Function));
}

void FDADeferredExecutor::Tick() {
	// Execute all pending functions
	TArray<FExecutableFunction> FrameExecutions = MoveTemp(PendingExecutions);

	// Clear the queue, as the current frame executions might add more to this list
	PendingExecutions.Empty();
	
	for (auto& Function : FrameExecutions) {
		Function();
	}
       
}

