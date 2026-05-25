//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Rules/Spawn/DungeonSpawnLogic.h"

#include "Engine/World.h"

UWorld* UDungeonSpawnLogic::GetWorld() const {
	return WeakCachedWorld.Get();
}

float UDungeonSpawnLogic::GetRandomFloat() {
	return CachedRandom ? CachedRandom->FRand() : 0;
}

int UDungeonSpawnLogic::GetRandomInt() {
	return CachedRandom ? CachedRandom->RandRange(0, MAX_int32) : 0;
}

void UDungeonSpawnLogic::OnItemSpawn_Implementation(AActor* Actor, const ADungeon* Dungeon) {
}

void UDungeonSpawnLogic::SetExecState(UWorld* InWorld, const FRandomStream* InRandom) {
	WeakCachedWorld = InWorld;
	CachedRandom = InRandom;
}

void UDungeonSpawnLogic::ClearExecState() {
	WeakCachedWorld = nullptr;
	CachedRandom = nullptr;
}

