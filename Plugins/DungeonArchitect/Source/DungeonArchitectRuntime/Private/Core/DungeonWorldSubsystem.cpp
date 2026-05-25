//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/DungeonWorldSubsystem.h"

#include "Core/Dungeon.h"

#include "EngineUtils.h"

void UDungeonWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld) {
	Super::OnWorldBeginPlay(InWorld);

	Dungeons.Reset();
	for (TActorIterator<ADungeon> It(&InWorld); It; ++It) {
		Dungeons.Add(*It);
	}
}

ADungeon* UDungeonWorldSubsystem::GetFirstActiveDungeon() const {
	for (TWeakObjectPtr<ADungeon> DungeonPtr : Dungeons) {
		if (DungeonPtr.IsValid()) {
			return DungeonPtr.Get();
		}
	}
	return nullptr;
}

TArray<ADungeon*> UDungeonWorldSubsystem::GetAllActiveDungeons() const {
	TArray<ADungeon*> ActiveDungeons;
	for (TWeakObjectPtr<ADungeon> DungeonPtr : Dungeons) {
		if (DungeonPtr.IsValid()) {
			ActiveDungeons.Add(DungeonPtr.Get());
		}
	}
	return ActiveDungeons;
}

