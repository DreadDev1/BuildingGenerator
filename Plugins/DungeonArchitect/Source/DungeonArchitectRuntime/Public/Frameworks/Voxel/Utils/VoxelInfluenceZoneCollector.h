//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

class ADAVoxelNoiseInfluenceVolume;
class UWorld;

/**
 * Helper class to collect noise influence zones from the world
 */
class DUNGEONARCHITECTRUNTIME_API FDAVoxelInfluenceZoneCollector
{
public:
	/**
	 * Collects all active influence zones from influence volume actors in the world
	 * @param World The world to search for influence volumes
	 * @param OutInfluenceZones Array to fill with collected influence zones
	 */
	static void CollectInfluenceZones(UWorld* World, TArray<FDANoiseInfluenceZone>& OutInfluenceZones);
	
	/**
	 * Collects influence zones within a specific bounding box
	 * @param World The world to search for influence volumes
	 * @param BoundingBox The area to search within
	 * @param OutInfluenceZones Array to fill with collected influence zones
	 */
	static void CollectInfluenceZonesInBounds(UWorld* World, const FBox& BoundingBox, TArray<FDANoiseInfluenceZone>& OutInfluenceZones);
};