//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Shaders/CanvasFogOfWarShader.h"

#include "DataDrivenShaderPlatformInfo.h"

bool FDACanvasFogOfWarShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

bool FDACanvasFogOfWarQueryStateShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

bool FDACanvasFogOfWarInitFrameShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

IMPLEMENT_GLOBAL_SHADER(FDACanvasFogOfWarInitFrameShader, "/Plugin/DungeonArchitect/Canvas/FogOfWar/CanvasFogOfWarInit.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDACanvasFogOfWarShader, "/Plugin/DungeonArchitect/Canvas/FogOfWar/CanvasFogOfWar.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDACanvasFogOfWarQueryStateShader, "/Plugin/DungeonArchitect/Canvas/FogOfWar/CanvasFogOfWarQuery.usf", "MainCS", SF_Compute);

