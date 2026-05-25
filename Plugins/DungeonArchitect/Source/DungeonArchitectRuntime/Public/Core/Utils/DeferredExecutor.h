//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class DUNGEONARCHITECTRUNTIME_API FDADeferredExecutor {
public:
	using FExecutableFunction = TFunction<void()>;

	void EnqueueExecutionForNextTick(FExecutableFunction&& Function);
	void Tick();

private:
	TArray<FExecutableFunction> PendingExecutions;
};

