// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spawners/Rooms/RoomSpawner.h"
#include "ChunkyRoomSpawner.generated.h"

class UChunkyRoomGenerator;

/**
 * AChunkyRoomSpawner - Spawner for chunky room generation
 * Creates rooms by combining rectangular chunks in irregular patterns
 */
UCLASS()
class BUILDINGGENERATOR_API AChunkyRoomSpawner : public ARoomSpawner
{
	GENERATED_BODY()
protected:
	/** Override to create UniformRoomGenerator specifically */
	virtual bool EnsureGeneratorReady() override;
};