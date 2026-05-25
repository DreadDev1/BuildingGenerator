//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Utils/VoxelInfluenceZoneCollector.h"

#include "Frameworks/Voxel/Utils/VoxelNoiseInfluenceVolume.h"

#include "Engine/World.h"
#include "EngineUtils.h"

void FDAVoxelInfluenceZoneCollector::CollectInfluenceZones(UWorld* World, TArray<FDANoiseInfluenceZone>& OutInfluenceZones)
{
	OutInfluenceZones.Empty();
	
	if (!World)
	{
		return;
	}
	
	for (TActorIterator<ADAVoxelNoiseInfluenceVolume> It(World); It; ++It)
	{
		ADAVoxelNoiseInfluenceVolume* Volume = *It;
		if (Volume && Volume->InfluenceZoneData.bEnabled)
		{
			// Update the transform before collecting
			FDANoiseInfluenceZone Zone = Volume->InfluenceZoneData;
			Zone.Transform = Volume->GetActorTransform();
			OutInfluenceZones.Add(Zone);
		}
	}
}

void FDAVoxelInfluenceZoneCollector::CollectInfluenceZonesInBounds(UWorld* World, const FBox& BoundingBox, TArray<FDANoiseInfluenceZone>& OutInfluenceZones)
{
	OutInfluenceZones.Empty();
	
	if (!World)
	{
		return;
	}
	
	for (TActorIterator<ADAVoxelNoiseInfluenceVolume> It(World); It; ++It)
	{
		ADAVoxelNoiseInfluenceVolume* Volume = *It;
		if (Volume && Volume->InfluenceZoneData.bEnabled)
		{
			FVector VolumeLocation = Volume->GetActorLocation();
			FVector OuterExtent = Volume->InfluenceZoneData.OuterExtent;
			
			// Create a bounding box for the influence zone
			FBox InfluenceBox(VolumeLocation - OuterExtent, VolumeLocation + OuterExtent);
			
			// Check for overlap
			if (BoundingBox.Intersect(InfluenceBox))
			{
				FDANoiseInfluenceZone Zone = Volume->InfluenceZoneData;
				Zone.Transform = Volume->GetActorTransform();
				OutInfluenceZones.Add(Zone);
			}
		}
	}
}

