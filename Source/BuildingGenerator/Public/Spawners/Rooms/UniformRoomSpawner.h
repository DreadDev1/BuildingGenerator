// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spawners/Rooms/RoomSpawner.h"
#include "UniformRoomSpawner.generated.h"

UCLASS()
class BUILDINGGENERATOR_API AUniformRoomSpawner : public ARoomSpawner
{
	GENERATED_BODY()

protected:
	/** Override to create UniformRoomGenerator specifically */
	virtual bool EnsureGeneratorReady() override;
};
