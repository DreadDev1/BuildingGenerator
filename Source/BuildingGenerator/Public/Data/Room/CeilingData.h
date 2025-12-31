// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CeilingData.generated.h"

struct FMeshPlacementInfo;

UCLASS()
class BUILDINGGENERATOR_API UCeilingData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ceiling Tiles")
	TArray<FMeshPlacementInfo> CeilingTilePool;
	
	// Height of the ceiling above the floor (Z offset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ceiling Settings")
	float CeilingHeight = 500.0f;

	// Rotation offset for all ceiling tiles (0, 180, 0) to flip floor tiles upside down for ceiling
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ceiling Settings")
	FRotator CeilingRotation = FRotator(0.0f, 0.0f, 0.0f);
};
