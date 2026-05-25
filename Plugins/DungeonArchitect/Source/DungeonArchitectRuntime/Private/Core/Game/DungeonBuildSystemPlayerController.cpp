//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Game/DungeonBuildSystemPlayerController.h"

#include "Core/Dungeon.h"
#include "Core/Game/DungeonBuildSystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UDungeonBuildSystemPlayerControllerComponent::ServerRequestBuildDungeonOnClient_Implementation(int32 Seed) {
	check(HasAuthority());
	ClientBuildDungeonOnClient(Seed);
}

void UDungeonBuildSystemPlayerControllerComponent::ClientBuildDungeonOnClient_Implementation(int32 Seed) {
	if (IsLocalController() && !HasAuthority()) {
		if (ADungeon* Dungeon = FDungeonBuildSystemUtils::FindDungeonActor(GetWorld())) {
			if (UDungeonBuildSystemDungeonComponent* BuildSystemComponent = Dungeon->GetBuildSystemComponent()) {
				BuildSystemComponent->ClientBuildDungeon(Seed);
			}
		}
	}
}

void UDungeonBuildSystemPlayerControllerComponent::ServerNotifyClientDungeonBuildComplete_Implementation(UDungeonBuildSystemDungeonComponent* BuildSystem, int32 Seed) {
	check(HasAuthority());
	if (BuildSystem) {
		if (APlayerController* PlayerController = GetController<APlayerController>()) {
			BuildSystem->ServerNotifyClientDungeonBuildComplete(PlayerController, Seed);
		}
	}
}

bool UDungeonBuildSystemPlayerControllerComponent::ServerNotifyClientDungeonBuildComplete_Validate(UDungeonBuildSystemDungeonComponent* BuildSystem, int32 Seed) {
	return true;
}

