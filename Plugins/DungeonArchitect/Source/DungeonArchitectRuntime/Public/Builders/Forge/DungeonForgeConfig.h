//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonConfig.h"
#include "Frameworks/Forge/DungeonForgeAsset.h"
#include "DungeonForgeConfig.generated.h"

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeConfig : public UDungeonConfig {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Dungeon")
	TSoftObjectPtr<UDungeonForgeAsset> DungeonForgeAsset;
};

