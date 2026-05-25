//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RenderGraphResources.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"

class DUNGEONARCHITECTRUNTIME_API FVoxelDensityComputeShader : public FGlobalShader {
public:
	DECLARE_GLOBAL_SHADER(FVoxelDensityComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVoxelDensityComputeShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		// Chunk parameters
		SHADER_PARAMETER(FIntVector, ChunkCoord)
		SHADER_PARAMETER(FIntVector, VoxelMin)
		SHADER_PARAMETER(FIntVector, VoxelMax)
		SHADER_PARAMETER(float, VoxelSize)
		SHADER_PARAMETER(float, CeilingHeightOffset)
		SHADER_PARAMETER(float, WallThickness)
		
		// Noise parameters
		SHADER_PARAMETER(FVector3f, NoiseOffset)
		SHADER_PARAMETER(float, NoiseAmplitude)
		SHADER_PARAMETER(float, NoiseScale)
		SHADER_PARAMETER(float, NoiseChannelSeparation)
		SHADER_PARAMETER(float, FloorCeilingTransitionHeight)
		SHADER_PARAMETER(float, NoiseFloorScale)
		SHADER_PARAMETER(float, NoiseCeilingScale)
		SHADER_PARAMETER(int32, NoiseOctaves)
		
		// Noise scaling parameters
		SHADER_PARAMETER(FVector3f, NoiseScaleVector)
		
		// Domain warp parameters
		SHADER_PARAMETER(uint32, bEnableDomainWarp)
		SHADER_PARAMETER(float, DomainWarpStrength)

		// SDF Model type (0=Cave, 1=Island)
		SHADER_PARAMETER(int32, SDFModelType)

		// Island-specific parameters
		SHADER_PARAMETER(float, WaterLevel)
		SHADER_PARAMETER(float, ShoreSlope)
		SHADER_PARAMETER(float, ShorelineFalloffDistance)
		SHADER_PARAMETER(float, MaxShoreDepth)
		SHADER_PARAMETER(float, ShoreDepthCutoff)

		// Floating Island-specific parameters
		SHADER_PARAMETER(float, FloatingIsland_PlatformDepth)
		SHADER_PARAMETER(float, FloatingIsland_SurfaceZ)
		SHADER_PARAMETER(float, FloatingIsland_ExpStrength)
		SHADER_PARAMETER(FVector3f, FloatingIsland_TipLocation)
		SHADER_PARAMETER(float, FloatingIsland_CurlAnglePerMeterRad)

		// 3D Noise texture
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, NoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, NoiseTextureSampler)
		
		// Single unified shape buffer (all shapes, sorted by layer)
		SHADER_PARAMETER(int32, NumShapes)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVoxelShapeGPU>, ShapeBuffer)
		
		// Influence zone buffer
		SHADER_PARAMETER(int32, NumInfluenceZones)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FNoiseInfluenceZoneGPU>, InfluenceZoneBuffer)
		
		// Output buffers
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, DensityBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, MaterialBuffer)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		// Use 8x8x8 thread groups for better occupancy
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Y"), 8);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Z"), 8);
	}
};