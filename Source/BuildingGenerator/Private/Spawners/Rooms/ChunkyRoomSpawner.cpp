// Fill out your copyright notice in the Description page of Project Settings.

#include "Spawners/Rooms/ChunkyRoomSpawner.h"
#include "Generators/Rooms/ChunkyRoomGenerator.h"
#include "DrawDebugHelpers.h"



bool AChunkyRoomSpawner::EnsureGeneratorReady()
{
	// Validate RoomData
	if (!RoomData) { /* error */ return false; }
	if (RoomGridSize.X < 4 || RoomGridSize.Y < 4) { /* error */ return false; }
    
	// Create ChunkyRoomGenerator
	if (!RoomGenerator)
	{
		RoomGenerator = NewObject<UChunkyRoomGenerator>(this, TEXT("ChunkyRoomGenerator"));
		if (!RoomGenerator) { /* error */ return false; }
	}
    
	// Cast to chunky type
	UChunkyRoomGenerator* ChunkyGen = Cast<UChunkyRoomGenerator>(RoomGenerator);
    
	// Initialize if needed
	if (!ChunkyGen->IsInitialized())
	{
		if (!ChunkyGen->Initialize(RoomData, RoomGridSize)) { /* error */ return false; }        
		ChunkyGen->CreateGrid();
	}
    
	return true;
}

