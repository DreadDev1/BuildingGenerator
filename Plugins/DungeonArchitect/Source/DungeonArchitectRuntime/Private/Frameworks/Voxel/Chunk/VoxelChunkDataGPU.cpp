//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Chunk/VoxelChunkDataGPU.h"

#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

#include "Engine/World.h"
#include "ShaderParameterUtils.h"
#include "Stats/Stats.h"

FDAVoxelShapeGPU FDAVoxelShapeGPU::ConvertToGPU(const FDAVoxelShape& CPUShape) {
	FDAVoxelShapeGPU GPUShape;
	
	// Convert transform to 3x4 matrix
	FMatrix44f Matrix = FMatrix44f(CPUShape.Transform.ToMatrixWithScale());
	GPUShape.Transform0 = FVector4f(Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2], Matrix.M[3][0]);
	GPUShape.Transform1 = FVector4f(Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2], Matrix.M[3][1]);
	GPUShape.Transform2 = FVector4f(Matrix.M[2][0], Matrix.M[2][1], Matrix.M[2][2], Matrix.M[3][2]);
	
	// Pack shape data
	GPUShape.ShapeData1 = FVector4f(
		(float)CPUShape.Type,
		CPUShape.Height,
		(float)CPUShape.Operation,
		(float)CPUShape.Layer
	);
	
	GPUShape.ShapeData2 = FVector4f(
		CPUShape.Radius,
		CPUShape.LineStart.X,
		CPUShape.LineStart.Y,
		CPUShape.LineEnd.X
	);
	
	GPUShape.ShapeData3 = FVector4f(
		CPUShape.LineEnd.Y,
		CPUShape.Thickness,
		CPUShape.Width,
		CPUShape.DoorOcclusionThickness
	);
	
	GPUShape.ShapeData4 = FVector4f(
		CPUShape.HoleMinHeight,
		CPUShape.Eccentricity,
		CPUShape.EllipseRotation,
		(float)CPUShape.MaterialID
	);
	
	// Pack noise override settings
	GPUShape.NoiseOverride1 = FVector4f(
		CPUShape.bOverrideNoiseSettings ? 1.0f : 0.0f,
		CPUShape.NoiseSettingsOverride.NoiseAmplitude,
		CPUShape.NoiseSettingsOverride.GetNoiseScale(true),
		CPUShape.NoiseSettingsOverride.NoiseChannelSeparation
	);
	
	GPUShape.NoiseOverride2 = FVector4f(
		CPUShape.NoiseSettingsOverride.FloorCeilingTransitionHeight,
		CPUShape.NoiseSettingsOverride.NoiseFloorScale,
		CPUShape.NoiseSettingsOverride.NoiseCeilingScale,
		(float)CPUShape.NoiseSettingsOverride.NoiseOctaves
	);
	
	GPUShape.NoiseOverride3 = FVector4f(
		CPUShape.NoiseSettingsOverride.NoiseOffset.X,
		CPUShape.NoiseSettingsOverride.NoiseOffset.Y,
		CPUShape.NoiseSettingsOverride.NoiseOffset.Z,
		0.0f
	);
	
	// Copy polygon points
	GPUShape.NumPolygonPoints = FMath::Min(CPUShape.PolygonPoints.Num(), 8);
	for (int32 i = 0; i < 8; i++) {
		if (i < GPUShape.NumPolygonPoints) {
			GPUShape.PolygonPoints[i] = FVector4f(
				CPUShape.PolygonPoints[i].X,
				CPUShape.PolygonPoints[i].Y,
				0.0f,
				0.0f
			);
		} else {
			GPUShape.PolygonPoints[i] = FVector4f::Zero();
		}
	}
	
	return GPUShape;
}

FNoiseInfluenceZoneGPU FNoiseInfluenceZoneGPU::ConvertToGPU(const FDANoiseInfluenceZone& CPUZone) {
	FNoiseInfluenceZoneGPU GPUZone;
	
	// Convert transform to 3x4 matrix
	FMatrix44f Matrix = FMatrix44f(CPUZone.Transform.ToMatrixWithScale());
	GPUZone.Transform0 = FVector4f(Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2], Matrix.M[3][0]);
	GPUZone.Transform1 = FVector4f(Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2], Matrix.M[3][1]);
	GPUZone.Transform2 = FVector4f(Matrix.M[2][0], Matrix.M[2][1], Matrix.M[2][2], Matrix.M[3][2]);
	
	// Pack extent data
	GPUZone.InnerExtent = FVector4f(
		CPUZone.InnerExtent.X,
		CPUZone.InnerExtent.Y,
		CPUZone.InnerExtent.Z,
		CPUZone.bEnabled ? 1.0f : 0.0f
	);
	
	GPUZone.OuterExtent = FVector4f(
		CPUZone.OuterExtent.X,
		CPUZone.OuterExtent.Y,
		CPUZone.OuterExtent.Z,
		CPUZone.bUseEllipsoidFalloff ? 1.0f : 0.0f
	);
	
	// Pack override values
	GPUZone.Overrides = FVector4f(
		CPUZone.OverrideNoiseAmplitude,
		CPUZone.OverrideNoiseFloorScale,
		CPUZone.OverrideNoiseScale,
		0.0f  // Reserved for future use
	);
	
	// Pack override flags
	GPUZone.OverrideFlags = FVector4f(
		CPUZone.bOverrideNoiseAmplitude ? 1.0f : 0.0f,
		CPUZone.bOverrideNoiseFloorScale ? 1.0f : 0.0f,
		CPUZone.bOverrideNoiseScale ? 1.0f : 0.0f,
		CPUZone.bOverrideNoiseScaleVector ? 1.0f : 0.0f
	);
	
	// Pack noise scale vector override
	GPUZone.OverrideNoiseScaleVector = FVector4f(
		CPUZone.OverrideNoiseScaleVector.X,
		CPUZone.OverrideNoiseScaleVector.Y,
		CPUZone.OverrideNoiseScaleVector.Z,
		0.0f  // Reserved for future use
	);
	
	return GPUZone;
}
