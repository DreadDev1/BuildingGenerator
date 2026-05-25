//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DungeonForgeRandomizeToolHandler.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UDungeonForgeRandomizeToolHandler : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONARCHITECTRUNTIME_API IDungeonForgeRandomizeToolHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	void RandomizeSeed() const;
	virtual void RandomizeSeed_Implementation() const = 0;
};

