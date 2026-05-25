//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DungeonDoorInterface.generated.h"

UINTERFACE(MinimalAPI)
class UDungeonDoorInterface : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONARCHITECTRUNTIME_API IDungeonDoorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent , Category="Dungeon Architect Door")
	bool IsPassageOpen() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent , Category="Dungeon Architect Door")
	bool SupportsLocking() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent , Category="Dungeon Architect Door")
	bool IsOneWayPassage() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent , Category="Dungeon Architect Door")
	float GetDoorWidth() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent , Category="Dungeon Architect Door")
	float GetDoorBaseRotation() const;
	
};

