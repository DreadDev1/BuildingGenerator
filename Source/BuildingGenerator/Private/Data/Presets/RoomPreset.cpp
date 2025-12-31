// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/Presets/RoomPreset.h"

void URoomPreset::ValidateRegions()
{
	if (Regions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PresetRoomLayout '%s':  No regions defined"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PresetRoomLayout '%s':  Validating %d regions... "), *GetName(), Regions.Num());

	int32 ErrorCount = 0;
	int32 WarningCount = 0;

	// Validate each region
	for (int32 i = 0; i < Regions.Num(); ++i)
	{
		const FPresetRegion& Region = Regions[i];

		// Check if region has valid bounds
		if (Region. EndCell.X < Region.StartCell.X || Region.EndCell.Y < Region. StartCell.Y)
		{
			UE_LOG(LogTemp, Error, TEXT("  Region[%d] '%s':  Invalid bounds (Start=%d,%d, End=%d,%d)"),
				i, *Region.RegionName, Region.StartCell.X, Region.StartCell. Y, Region.EndCell.X, Region.EndCell.Y);
			ErrorCount++;
		}

		// Check if region has any styles assigned
		if (Region.RegionFloorStyle.IsNull() && DefaultFloorStyle.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("  Region[%d] '%s': No floor style assigned (and no default)"),
				i, *Region.RegionName);
			WarningCount++;
		}

		// Check for overlaps with other regions (if not allowed)
		if (!bAllowRegionOverlap)
		{
			for (int32 j = i + 1; j < Regions. Num(); ++j)
			{
				const FPresetRegion& OtherRegion = Regions[j];

				// Check if regions overlap
				bool bOverlaps = !(Region.EndCell.X < OtherRegion.StartCell.X || 
				                   Region.StartCell.X > OtherRegion.EndCell.X ||
				                   Region.EndCell. Y < OtherRegion. StartCell.Y || 
				                   Region.StartCell.Y > OtherRegion.EndCell.Y);

				if (bOverlaps)
				{
					UE_LOG(LogTemp, Error, TEXT("  Region[%d] '%s' overlaps with Region[%d] '%s' (overlaps not allowed)"),
						i, *Region.RegionName, j, *OtherRegion.RegionName);
					ErrorCount++;
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("  Region[%d] '%s': Start=(%d,%d), End=(%d,%d), Size=(%dx%d), Priority=%d"),
			i, *Region.RegionName, 
			Region.StartCell.X, Region.StartCell.Y,
			Region.EndCell.X, Region.EndCell.Y,
			Region.EndCell.X - Region.StartCell.X + 1,
			Region.EndCell.Y - Region.StartCell.Y + 1,
			Region.FillPriority);
	}

	// Summary
	if (ErrorCount > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("PresetRoomLayout '%s':  Validation FAILED (%d errors, %d warnings)"), 
			*GetName(), ErrorCount, WarningCount);
	}
	else if (WarningCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PresetRoomLayout '%s': Validation passed with warnings (%d warnings)"), 
			*GetName(), WarningCount);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("PresetRoomLayout '%s': Validation PASSED ✓"), *GetName());
	}
}

const FPresetRegion* URoomPreset::GetRegionAtCoordinate(FIntPoint GridCoordinate, FIntPoint GridSize) const
{
	// Bounds check
	if (GridCoordinate.X < 0 || GridCoordinate.X >= GridSize.X || 
	    GridCoordinate.Y < 0 || GridCoordinate.Y >= GridSize.Y)
	{
		return nullptr;
	}

	// Find highest priority region containing this coordinate
	const FPresetRegion* FoundRegion = nullptr;
	int32 HighestPriority = -1;

	for (const FPresetRegion& Region :  Regions)
	{
		// Check if coordinate is within region bounds
		if (GridCoordinate.X >= Region. StartCell.X && GridCoordinate.X <= Region.EndCell.X &&
		    GridCoordinate.Y >= Region.StartCell.Y && GridCoordinate.Y <= Region. EndCell.Y)
		{
			// If this region has higher priority, use it
			if (Region.FillPriority > HighestPriority)
			{
				FoundRegion = &Region;
				HighestPriority = Region.FillPriority;
			}
		}
	}

	return FoundRegion;
}