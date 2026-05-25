//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Shaders/CanvasShapeBorderGenShader.h"

#include "DataDrivenShaderPlatformInfo.h"

bool FDACanvasShapeBorderGenShader::ShouldCompilePermutation(FGlobalShaderPermutationParameters const& Parameters) {
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

IMPLEMENT_GLOBAL_SHADER(FDACanvasShapeBorderGenShader, "/Plugin/DungeonArchitect/Canvas/Internal/CanvasShapeBorderGen.usf", "MainCS", SF_Compute);

