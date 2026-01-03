// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "UniformRoomGenerator.generated.h"

/**
 * 
 */
UCLASS()
class BUILDINGGENERATOR_API UUniformRoomGenerator : public URoomGenerator
{
	GENERATED_BODY()

public:
	// Generation interface (will implement these one by one)
	virtual void CreateGrid() override;
	virtual bool GenerateFloor() override;
	virtual bool GenerateWalls() override;
	virtual bool GenerateCorners() override;
	virtual bool GenerateDoorways() override;
	virtual bool GenerateCeiling() override;

private:
	// Private helper methods will be moved here as needed
};
