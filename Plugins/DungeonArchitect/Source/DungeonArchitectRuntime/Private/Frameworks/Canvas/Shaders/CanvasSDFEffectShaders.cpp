//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Shaders/CanvasSDFEffectShaders.h"

#include "DataDrivenShaderPlatformInfo.h"

inline bool FDACanvasVoronoiSDFShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

IMPLEMENT_GLOBAL_SHADER(FDACanvasVoronoiSDFShader, "/Plugin/DungeonArchitect/Canvas/Effects/VoronoiSDF.usf", "MainCS", SF_Compute);

