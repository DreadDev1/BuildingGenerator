//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct DUNGEONARCHITECTRUNTIME_API FGridBuilderMarkers {
    static const FString FENCE;
    static const FString FENCE_SEPARATOR;
    static const FString DOOR;
    static const FString WALL;
    static const FString WALL_SEPARATOR;
    static const FString GROUND;
    static const FString STAIR;
    static const FString STAIR2X;
    static const FString WALLHALF;
    static const FString WALLHALF_SEPARATOR;
    static const FString NONE;

    static const FString ROOMWALL;
    static const FString ROOMWALL_SEPARATOR;
    static const FString ROOMOPENSPACE;
    static const FString LIGHT;
};

struct DUNGEONARCHITECTRUNTIME_API FGridFlowBuilderMarkers {
    static const FString GROUND;
    static const FString WALL;
    static const FString WALL_L;
    static const FString WALL_T;
    static const FString WALL_X;
    static const FString DOOR;
    static const FString DOOR_ONEWAY;
    static const FString CAVE_FENCE;
    static const FString CAVE_FENCE_SEPARATOR;
    static const FString WALL_SEPARATOR;
    static const FString FENCE;
    static const FString FENCE_SEPARATOR;
};

struct DUNGEONARCHITECTRUNTIME_API FVoxelBuilderMarkers {
    static const FString INTERNAL_VOXEL_WALL;
    static const FString INTERNAL_VOXEL_STAIR;
    static const FString INTERNAL_VOXEL_DOOR;
};

struct DUNGEONARCHITECTRUNTIME_API FCellFlowBuilderMarkers {
    static const FString GROUND;
    static const FString CEILING;
    static const FString CEILING_EDGE_TRIM;
    static const FString WALL;
    static const FString WALL_HALF;
    static const FString DOOR_WALL_HALF;
    static const FString WALL_SEPARATOR;
    static const FString STAIR;
    static const FString DOOR;
    static const FString DOOR_ONEWAY;
    static const FString SITE;
    static const FString SITE_GROUP;
};

struct DUNGEONARCHITECTRUNTIME_API FSimpleCityBuilderMarkers {
    static const FString HOUSE;
    static const FString PARK;
    static const FString ROAD_X;
    static const FString ROAD_T;
    static const FString ROAD_S;
    static const FString ROAD_CORNER;
    static const FString ROAD_END;
    static const FString ROAD;
};

struct DUNGEONARCHITECTRUNTIME_API FFloorPlanBuilderMarkers {
    static const FString GROUND;
    static const FString CEILING;
    static const FString WALL;
    static const FString WALL_SEPARATOR;
    static const FString DOOR;
    static const FString BUILDING_WALL;
    static const FString BUILDING_WALL_SEPARATOR;
};

struct DUNGEONARCHITECTRUNTIME_API FIsaacBuilderMarkers {
    static const FString FENCE;
    static const FString FENCE_SEPARATOR;
    static const FString DOOR;
    static const FString WALL;
    static const FString WALL_SEPARATOR;
    static const FString GROUND;
};

