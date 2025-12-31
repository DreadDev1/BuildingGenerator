// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawners/Rooms/RandWalkSpawner.h"


// Sets default values
ARandWalkSpawner::ARandWalkSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ARandWalkSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARandWalkSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

