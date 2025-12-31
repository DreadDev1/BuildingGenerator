// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoomPreset.generated.h"

/* Preset region definition - defines a sub-area within a room with its own generation rules */
USTRUCT(BlueprintType)
struct FPresetRegion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
	FString RegionName = "Unnamed Region";

	/* Top-left corner of region (inclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
	FIntPoint StartCell = FIntPoint(0, 0);

	/* Bottom-right corner of region (inclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
	FIntPoint EndCell = FIntPoint(4, 4);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Styles")
	TSoftObjectPtr<class UFloorData> RegionFloorStyle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Styles")
	TSoftObjectPtr<class UCeilingData> RegionCeilingStyle;

	/* Fill priority (higher values = filled first, useful for overlapping regions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Advanced", meta=(ClampMin="0", ClampMax="100"))
	int32 FillPriority = 0;

	/* Should this region generate internal walls around its perimeter? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Walls")
	bool bGenerateInternalWalls = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Walls", meta=(EditCondition="bGenerateInternalWalls"))
	TSoftObjectPtr<class UWallData> RegionWallStyle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Walls", meta=(EditCondition="bGenerateInternalWalls"))
	bool bRequiresDoorway = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region|Debug")
	FColor DebugColor = FColor:: Cyan;
};

/* Preset room layout definition */

UCLASS()
class BUILDINGGENERATOR_API URoomPreset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/* List of regions that make up this preset room */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Layout")
	TArray<FPresetRegion> Regions;

	/* Default floor style (used for cells not in any region) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Layout|Defaults")
	TSoftObjectPtr<class UFloorData> DefaultFloorStyle;

	/* Default ceiling style (used for cells not in any region) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Layout|Defaults")
	TSoftObjectPtr<class UCeilingData> DefaultCeilingStyle;

	/* Should regions allow overlap? (if true, FillPriority determines which region wins) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Layout|Advanced")
	bool bAllowRegionOverlap = false;

	/* Validate regions (check bounds, overlaps, etc.) */
	UFUNCTION(CallInEditor, Category = "Preset Layout|Validation")
	void ValidateRegions();

	/* Get region at specific grid coordinate (returns nullptr if no region found) */
	const FPresetRegion* GetRegionAtCoordinate(FIntPoint GridCoordinate, FIntPoint GridSize) const;
};
