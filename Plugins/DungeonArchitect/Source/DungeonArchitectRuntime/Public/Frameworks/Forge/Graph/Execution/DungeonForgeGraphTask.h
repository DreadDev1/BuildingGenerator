//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "Frameworks/Forge/DungeonForgeCommon.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphExecutor.h"
#include "DungeonForgeGraphTask.generated.h"

class UDungeonForgeGraphNode;

USTRUCT()
struct FDungeonForgeGraphTaskInput {
	GENERATED_BODY()

	FDungeonForgeGraphTaskInput() = default;

	explicit FDungeonForgeGraphTaskInput(FDungeonForgeTaskId InTaskId, const TOptional<FDungeonForgePinProperties>& InUpstreamPin = NoPin, const TOptional<FDungeonForgePinProperties>& InDownstreamPin = NoPin, bool bInProvideData = true)
		: TaskId(InTaskId)
		, UpstreamPin(InUpstreamPin)
		, DownstreamPin(InDownstreamPin)
		, bProvideData(bInProvideData)
	{
	}

#if WITH_EDITOR
	bool operator==(const FDungeonForgeGraphTaskInput& Other) const;
#endif

	UPROPERTY()
	uint64 TaskId = InvalidDungeonForgeTaskId;

	/** The upstream output pin from which the input data comes. */
	UPROPERTY()
	TOptional<FDungeonForgePinProperties> UpstreamPin;

	/** The input pin on the task element. */
	UPROPERTY()
	TOptional<FDungeonForgePinProperties> DownstreamPin;

	/** Whether the input provides any data. For the post execute task, only the output node will provide data. */
	UPROPERTY()
	bool bProvideData = false;
	
	/** Whether the input is used multiple times (previous task has multiple successors). Useful to know if the data can be stolen or not. True by default.*/
	UPROPERTY()
	bool bIsUsedMultipleTimes = true;

	static inline const TOptional<FDungeonForgePinProperties> NoPin = TOptional<FDungeonForgePinProperties>();
};

USTRUCT()
struct FDungeonForgeGraphTask {
	GENERATED_BODY()
public:
#if WITH_EDITOR
	void PrepareForCook();
#else
	void LoadCookedData();
#endif
	
	bool CanExecuteOnlyOnMainThread() const;

	UPROPERTY()
	TArray<FDungeonForgeGraphTaskInput> Inputs;

	/** Serialized version of Node (see below) to avoid hard object references. Only used for cooking and standalone. */
	UPROPERTY()
	TSoftObjectPtr<const UDungeonForgeGraphNode> NodePtr = nullptr;
	
	UPROPERTY()
	uint64 NodeId = InvalidDungeonForgeTaskId;

	/** The task id as it exists when compiled. */
	UPROPERTY()
	uint64 CompiledTaskId = InvalidDungeonForgeTaskId;
	
	TObjectPtr<const UDungeonForgeGraphNode> Node;
};

