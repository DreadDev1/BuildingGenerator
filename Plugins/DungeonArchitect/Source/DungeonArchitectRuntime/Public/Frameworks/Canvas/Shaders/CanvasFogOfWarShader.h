//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FDACanvasFogOfWarInitFrameShader : public FGlobalShader {
public:
	DECLARE_GLOBAL_SHADER(FDACanvasFogOfWarInitFrameShader);
	SHADER_USE_PARAMETER_STRUCT(FDACanvasFogOfWarInitFrameShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, TexFogOfWarVisibility)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};



class FDACanvasFogOfWarShader : public FGlobalShader {
public:
	DECLARE_GLOBAL_SHADER(FDACanvasFogOfWarShader);
	SHADER_USE_PARAMETER_STRUCT(FDACanvasFogOfWarShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(FIntPoint, FoWSourcePixel)		// The visibility point, e.g. player location
		SHADER_PARAMETER(FIntPoint, BaseOffset)			// The offset to apply to the pixel
		SHADER_PARAMETER(float, RadiusPixels)			// Radius of the fog of war, in pixels

		SHADER_PARAMETER(float, SoftShadowRadius)
		SHADER_PARAMETER(int, SoftShadowSamples)
		
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, TexSDF) 
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, TexFogOfWarExplored)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, TexFogOfWarVisibility)
	END_SHADER_PARAMETER_STRUCT()
		
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};


class FDACanvasFogOfWarQueryStateShader : public FGlobalShader {
public:
	DECLARE_GLOBAL_SHADER(FDACanvasFogOfWarQueryStateShader);
	SHADER_USE_PARAMETER_STRUCT(FDACanvasFogOfWarQueryStateShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, TexFogOfWarExplored)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, TexFogOfWarVisibility)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float2>, QueryLocations)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float2>, ExploreVisibleStates)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};

