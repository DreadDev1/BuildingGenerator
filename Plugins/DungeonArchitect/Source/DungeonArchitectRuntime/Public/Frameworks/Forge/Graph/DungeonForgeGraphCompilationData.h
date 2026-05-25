//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgeGraphCompilationData.generated.h"

struct FDungeonForgeGraphTask;

/** Holds compiled data for serialization during cook. */
UCLASS()
class UDungeonForgeGraphCompilationData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FDungeonForgeGraphTask> Tasks;
};

