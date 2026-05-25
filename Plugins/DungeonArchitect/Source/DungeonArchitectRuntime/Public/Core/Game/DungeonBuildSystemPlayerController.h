//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "DungeonBuildSystemPlayerController.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UDungeonBuildSystemPlayerControllerComponent : public UControllerComponent {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Server, Reliable, Category=Dungeon)
	void ServerRequestBuildDungeonOnClient(int32 Seed);
	
	UFUNCTION(BlueprintCallable, Client, Reliable, Category=Dungeon)
	void ClientBuildDungeonOnClient(int32 Seed);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerNotifyClientDungeonBuildComplete(UDungeonBuildSystemDungeonComponent* BuildSystem, int32 Seed);
};

