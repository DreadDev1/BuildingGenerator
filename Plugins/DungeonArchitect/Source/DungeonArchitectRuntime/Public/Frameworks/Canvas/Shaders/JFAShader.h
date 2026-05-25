//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"

class FDAJFAInitShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDAJFAInitShader);
	SHADER_USE_PARAMETER_STRUCT(FDAJFAInitShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, MaskTexture) 
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, BorderTexture) 
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, OcclusionTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, SDFTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float2>, NearestPoint)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};


class FDAJFAFindNearestPointShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDAJFAFindNearestPointShader);
	SHADER_USE_PARAMETER_STRUCT(FDAJFAFindNearestPointShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(int, JumpDistance)
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, MaskTexture) 
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float2>, NearestPoint)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};


class FDAJFAWriteSDFShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDAJFAWriteSDFShader);
	SHADER_USE_PARAMETER_STRUCT(FDAJFAWriteSDFShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight)
		SHADER_PARAMETER(float, MaxSDFValue)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, MaskTexture) 
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float2>, NearestPoint)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, SDFTexture)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};


class FDAJFAWriteSDFEdgeShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDAJFAWriteSDFEdgeShader);
	SHADER_USE_PARAMETER_STRUCT(FDAJFAWriteSDFEdgeShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight) 
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float2>, NearestPoint)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, SDFTexture)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};



