//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FDACanvasShapeBorderGenShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDACanvasShapeBorderGenShader);
	SHADER_USE_PARAMETER_STRUCT(FDACanvasShapeBorderGenShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight)
		SHADER_PARAMETER(int, OffsetX)
		SHADER_PARAMETER(int, OffsetY)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, FillTexture) 
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, BorderTexture)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};

