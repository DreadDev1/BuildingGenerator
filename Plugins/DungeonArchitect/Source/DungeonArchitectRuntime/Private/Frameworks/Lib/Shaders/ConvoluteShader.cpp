//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Lib/Shaders/ConvoluteShader.h"


// Kernel values here: https://en.wikipedia.org/wiki/Kernel_(image_processing)

const FDAConvKernel FDAConvKernels::Edge = {
	false, 1, 9, TEXT(
		"-1, -1, -1, "
		"-1,  8, -1, "
		"-1, -1, -1")
};

const FDAConvKernel FDAConvKernels::Sharpen = {
	false, 1, 9, TEXT(
		" 0, -1,  0, "
		"-1,  5, -1, "
		" 0, -1,  0")
};


const FDAConvKernel FDAConvKernels::BoxBlur = {
	true, 1.f/9, 9, TEXT(
		" 1,  1,  1, "
		" 1,  1,  1, "
		" 1,  1,  1")
};

const FDAConvKernel FDAConvKernels::GaussBlur3x3 = {
	true, 1.f/16, 9, TEXT(
	" 1,  2,  1, "
	" 2,  4,  2, "
	" 1,  2,  1")
};


const FDAConvKernel FDAConvKernels::GaussBlur5x5 = {
	true, 1.f/256, 25, TEXT(
	" 1,  4,  6,  4,  1, "
	" 4, 16, 24, 16,  4, "
	" 6, 24, 36, 24,  6, "
	" 4, 16, 24, 16,  4, "
	" 1,  4,  6,  4,  1, ")
};

IMPLEMENT_GLOBAL_SHADER(FDAConvEdgeShader, "/Plugin/DungeonArchitect/Canvas/Convolution/Convolution2D.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAConvSharpenShader, "/Plugin/DungeonArchitect/Canvas/Convolution/Convolution2D.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAConvBoxBlurShader, "/Plugin/DungeonArchitect/Canvas/Convolution/Convolution2D.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAConvGaussBlur3x3Shader, "/Plugin/DungeonArchitect/Canvas/Convolution/Convolution2D.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDAConvGaussBlur5x5Shader, "/Plugin/DungeonArchitect/Canvas/Convolution/Convolution2D.usf", "MainCS", SF_Compute);

