//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

enum class EDABlurShaderOp : uint8 {
	Add,
	Multiply,
	Copy
};

template<EDABlurShaderOp ShaderOp>
class TDABlitShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(TDABlitShader);
	SHADER_USE_PARAMETER_STRUCT(TDABlitShader, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, TexSource) 
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, TexDest)
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& Environment) {
		if (ShaderOp == EDABlurShaderOp::Add) {
			Environment.SetDefine(TEXT("BLIT_OP_ADD"), 1);
		}
		else if (ShaderOp == EDABlurShaderOp::Multiply) {
			Environment.SetDefine(TEXT("BLIT_OP_MULTIPLY"), 1);
		}
		else if (ShaderOp == EDABlurShaderOp::Copy) {
			Environment.SetDefine(TEXT("BLIT_OP_COPY"), 1);
		}
	}
};

