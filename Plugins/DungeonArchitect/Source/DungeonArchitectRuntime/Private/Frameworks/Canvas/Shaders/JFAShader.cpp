//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Shaders/JFAShader.h"

#include "DataDrivenShaderPlatformInfo.h"

bool FDAJFAInitShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

bool FDAJFAFindNearestPointShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

bool FDAJFAWriteSDFShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

bool FDAJFAWriteSDFEdgeShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

IMPLEMENT_GLOBAL_SHADER(FDAJFAInitShader, "/Plugin/DungeonArchitect/Canvas/JFA/JFA_Init.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAJFAFindNearestPointShader, "/Plugin/DungeonArchitect/Canvas/JFA/JFA_NearestPoint.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAJFAWriteSDFShader, "/Plugin/DungeonArchitect/Canvas/JFA/JFA_WriteSDF.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAJFAWriteSDFEdgeShader, "/Plugin/DungeonArchitect/Canvas/JFA/JFA_WriteSDFEdge.usf", "MainCS", SF_Compute);

