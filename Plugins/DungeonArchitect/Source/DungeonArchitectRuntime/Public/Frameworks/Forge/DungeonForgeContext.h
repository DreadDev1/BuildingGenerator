//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "DungeonForgeContext.generated.h"

class ADungeon;
class UDungeonForgeGraphNode;
class UDungeonForgeResourceManager;
struct FDungeonForgeGraphExecutionState;

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonForgeContext {
	GENERATED_BODY()

	virtual ~FDungeonForgeContext() = default;

	UPROPERTY()
	FDungeonForgeDataCollection InputData;
	
	UPROPERTY()
	FDungeonForgeDataCollection OutputData;
	
	TWeakObjectPtr<const UDungeonForgeGraphNode> Node = nullptr;
	TWeakPtr<const FDungeonForgeGraphExecutionState> ExecutionState;
};
typedef TSharedPtr<FDungeonForgeContext, ESPMode::ThreadSafe> FDungeonForgeContextPtr;

