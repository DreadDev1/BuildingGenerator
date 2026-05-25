//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FDAVoxelShape;

// GPU-optimized voxel shape structure matching shader layout
struct FDAVoxelShapeGPU {
	FVector4f Transform0;
	FVector4f Transform1;
	FVector4f Transform2;
	
	FVector4f ShapeData1; // x=Type, y=Height, z=Operation, w=Layer
	FVector4f ShapeData2; // x=Radius, y=LineStartX, z=LineStartY, w=LineEndX
	FVector4f ShapeData3; // x=LineEndY, y=Thickness, z=Width, w=DoorOcclusionThickness
	FVector4f ShapeData4; // x=HoleMinHeight, y=Eccentricity, z=EllipseRotation, w=MaterialID
	
	FVector4f NoiseOverride1; // x=bOverrideNoise, y=NoiseAmplitude, z=NoiseScale, w=NoiseChannelSeparation
	FVector4f NoiseOverride2; // x=FloorCeilingTransitionHeight, y=NoiseFloorScale, z=NoiseCeilingScale, w=NoiseOctaves
	FVector4f NoiseOverride3;
	
	FVector4f PolygonPoints[8];
	int32 NumPolygonPoints;
	int32 Padding[3];
	
	static FDAVoxelShapeGPU ConvertToGPU(const FDAVoxelShape& CPUShape);
};

// GPU-optimized noise influence zone structure matching shader layout
struct FNoiseInfluenceZoneGPU {
	FVector4f Transform0;  // Transform matrix row 0 + w=Translation.X
	FVector4f Transform1;  // Transform matrix row 1 + w=Translation.Y
	FVector4f Transform2;  // Transform matrix row 2 + w=Translation.Z
	FVector4f InnerExtent; // xyz=inner box half-extent, w=bEnabled
	FVector4f OuterExtent; // xyz=outer box half-extent, w=bUseEllipsoidFalloff
	FVector4f Overrides;   // x=OverrideNoiseAmplitude, y=OverrideNoiseFloorScale, z=OverrideNoiseScale, w=unused
	FVector4f OverrideFlags; // x=bOverrideAmplitude, y=bOverrideFloorScale, z=bOverrideScale, w=bOverrideScaleVector
	FVector4f OverrideNoiseScaleVector; // xyz=scale vector for directional stretching, w=unused
	
	static FNoiseInfluenceZoneGPU ConvertToGPU(const struct FDANoiseInfluenceZone& CPUZone);
};

// GPU voxel generation parameters
struct FDAVoxelGPUParams {
	FIntVector ChunkCoord;
	FIntVector VoxelMin;
	FIntVector VoxelMax;
	float VoxelSize;
	float CeilingHeightOffset;
	
	FVector NoiseOffset;
	float NoiseAmplitude;
	float NoiseScale;
	int32 NoiseOctaves;
	
	int32 NumPrimaryCarveShapes;
	int32 NumAddGeometryShapes;
};
