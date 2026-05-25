//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FDACanvasVoronoiSDFShader : public FGlobalShader {
public:
	DECLARE_GLOBAL_SHADER(FDACanvasVoronoiSDFShader);
	SHADER_USE_PARAMETER_STRUCT(FDACanvasVoronoiSDFShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(int, TextureWidth)
		SHADER_PARAMETER(int, TextureHeight)
		SHADER_PARAMETER(int, CellSize)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, TexSDF) 
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, TexVoronoi)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters);
};

