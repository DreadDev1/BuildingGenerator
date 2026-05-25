//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DungeonWorldSubsystem.generated.h"

class ADungeon;

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonWorldSubsystem : public UWorldSubsystem {
	GENERATED_BODY()
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	ADungeon* GetFirstActiveDungeon() const;
	TArray<ADungeon*> GetAllActiveDungeons() const;
	
private:
	TArray<TWeakObjectPtr<ADungeon>> Dungeons;
};

