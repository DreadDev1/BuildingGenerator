// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawners/Rooms/UniformRoomSpawner.h"
#include "Generators/Rooms/UniformRoomGenerator.h"


bool AUniformRoomSpawner::EnsureGeneratorReady()
{
	// Validate RoomData
	if (!RoomData)
	{ DebugHelpers->LogCritical(TEXT("RoomData is not assigned!")); return false; }

	if (RoomGridSize.X < 4 || RoomGridSize.Y < 4)
	{ DebugHelpers->LogCritical(TEXT("GridSize is too small (min 4x4)!")); return false; }

	// Create UniformRoomGenerator if needed
	if (!RoomGenerator)
	{
		DebugHelpers->LogVerbose(TEXT("Creating UniformRoomGenerator..."));
		RoomGenerator = NewObject<UUniformRoomGenerator>(this, TEXT("UniformRoomGenerator"));

		if (!RoomGenerator)
		{ DebugHelpers->LogCritical(TEXT("Failed to create UniformRoomGenerator!")); return false; }
	}

	// Initialize if needed
	if (!RoomGenerator->IsInitialized())
	{
		DebugHelpers->LogVerbose(TEXT("Initializing UniformRoomGenerator..."));

		if (!RoomGenerator->Initialize(RoomData, RoomGridSize))
		{ DebugHelpers->LogCritical(TEXT("Failed to initialize UniformRoomGenerator!")); return false; }

		DebugHelpers->LogVerbose(TEXT("Creating grid cells..."));
		RoomGenerator->CreateGrid();
	}

	return true;
}
