// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FloorData.generated.h"

struct FMeshPlacementInfo;

UCLASS()
class BUILDINGGENERATOR_API UFloorData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* Pool of floor tiles with placement weights and footprints */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Tiles")
	TArray<FMeshPlacementInfo> FloorTilePool;

	// --- Floor Clutter / Detail Meshes ---
	
	// A separate pool for smaller details or clutter placed randomly on top of the main tiles.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Clutter")
	TArray<FMeshPlacementInfo> ClutterMeshPool;

	// The likelihood (0.0 to 1.0) of attempting to place a clutter mesh in an empty floor cell.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Clutter")
	float ClutterPlacementChance = 0.25f;

};
