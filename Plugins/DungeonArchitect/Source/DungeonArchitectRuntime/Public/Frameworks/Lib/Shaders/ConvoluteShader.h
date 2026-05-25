//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "GlobalShader.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"

BEGIN_SHADER_PARAMETER_STRUCT(FDAConvoluteShaderParameters,)
	SHADER_PARAMETER(int, TextureWidth)
	SHADER_PARAMETER(int, TextureHeight)
	SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InTexture) 
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutTexture)
END_SHADER_PARAMETER_STRUCT()

struct FDAConvKernel {
	bool bUseCoefficient{};
	float Coefficient{};
	int KernelSize{};
	const TCHAR* KernelValues{};
};

struct FDAConvKernels {
	static const FDAConvKernel Edge;
	static const FDAConvKernel Sharpen;
	static const FDAConvKernel BoxBlur;
	static const FDAConvKernel GaussBlur3x3;
	static const FDAConvKernel GaussBlur5x5;
};

#define DEFINE_DA_CONV_SHADER(Name)		\
class FDAConv##Name##Shader : public FGlobalShader {		\
public:		\
	DECLARE_GLOBAL_SHADER(FDAConv##Name##Shader);		\
	SHADER_USE_PARAMETER_STRUCT(FDAConv##Name##Shader, FGlobalShader);	\
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )		\
		SHADER_PARAMETER(int, TextureWidth)		\
		SHADER_PARAMETER(int, TextureHeight)		\
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InTexture)		\
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutTexture)		\
	END_SHADER_PARAMETER_STRUCT()		\
	static bool ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {		\
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);		\
	}		\
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {		\
		static const FDAConvKernel& KernelInfo = FDAConvKernels::Name;		\
		const int32 KernelBoxSize = FMath::RoundToInt(FMath::Sqrt(static_cast<float>(KernelInfo.KernelSize)));	\
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);		\
		OutEnvironment.SetDefine(TEXT("KERNEL_BUF_SIZE"), KernelInfo.KernelSize);		\
		OutEnvironment.SetDefine(TEXT("KERNEL_VALUES"), KernelInfo.KernelValues);		\
		OutEnvironment.SetDefine(TEXT("USE_COEFFICIENT"), KernelInfo.bUseCoefficient ? 1 : 0);		\
		OutEnvironment.SetDefine(TEXT("Coefficient"), KernelInfo.Coefficient);		\
		OutEnvironment.SetDefine(TEXT("KernelExtent"), KernelBoxSize / 2);	\
	}		\
};

DEFINE_DA_CONV_SHADER(Edge)
DEFINE_DA_CONV_SHADER(Sharpen)
DEFINE_DA_CONV_SHADER(BoxBlur)
DEFINE_DA_CONV_SHADER(GaussBlur3x3)
DEFINE_DA_CONV_SHADER(GaussBlur5x5)

