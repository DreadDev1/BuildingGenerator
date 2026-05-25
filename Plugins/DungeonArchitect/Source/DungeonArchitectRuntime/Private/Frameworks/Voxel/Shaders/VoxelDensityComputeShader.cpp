//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Shaders/VoxelDensityComputeShader.h"

#include "ShaderCore.h"

IMPLEMENT_GLOBAL_SHADER(FVoxelDensityComputeShader, "/Plugin/DungeonArchitect/Voxel/VoxelDensityCompute.usf", "VoxelDensityCS", SF_Compute);

