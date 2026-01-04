// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawners/Rooms/UniformRoomSpawner.h"


// Sets default values
AUniformRoomSpawner::AUniformRoomSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AUniformRoomSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUniformRoomSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

