//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonLayoutSegmentActor.generated.h"

class UDungeonLayoutComponent;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API ADungeonLayoutSegmentActor : public AActor {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Layout")
	TArray<TObjectPtr<UDungeonLayoutComponent>> LayoutComponents;
};

