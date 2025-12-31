// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RoomSpawner.h"
#include "RandWalkSpawner.generated.h"

UCLASS()
class BUILDINGGENERATOR_API ARandWalkSpawner : public ARoomSpawner
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARandWalkSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
