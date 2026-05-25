//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Lib/Shaders/ReadbackRTShader.h"

#include "Core/Dungeon.h"

#include "Async/Async.h"
#include "GlobalShader.h"
#include "RHIGPUReadback.h"
#include "RenderGraphUtils.h"

#define LOCTEXT_NAMESPACE "DATextureReadbackCompute"
#define DA_NUM_THREADS_PER_GROUP_DIMENSION 8

class DUNGEONARCHITECTRUNTIME_API FDATextureReadbackChannelRCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDATextureReadbackChannelRCS);
	SHADER_USE_PARAMETER_STRUCT(FDATextureReadbackChannelRCS, FGlobalShader);
 
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D<float>, SourceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
		SHADER_PARAMETER(FVector2f, SourceDimensions)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
 
public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), DA_NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), DA_NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FDATextureReadbackChannelRCS, "/Plugin/DungeonArchitect/Common/TextureReadback.usf", "Main_CS", SF_Compute);

class DUNGEONARCHITECTRUNTIME_API FDATextureReadbackChannelRGBACS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDATextureReadbackChannelRGBACS);
	SHADER_USE_PARAMETER_STRUCT(FDATextureReadbackChannelRGBACS, FGlobalShader);
 
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D<float4>, SourceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
		SHADER_PARAMETER(FVector2f, SourceDimensions)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
 
public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), DA_NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), DA_NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FDATextureReadbackChannelRGBACS, "/Plugin/DungeonArchitect/Common/TextureReadbackRGBA.usf", "Main_CS", SF_Compute);

namespace DATextureReadbackInterface {
	FRHITexture* DispatchReadbackShaderRFloat(FRHICommandListImmediate& RHICmdList, const FDATextureReadbackDispatchParams& Params, const FTextureRHIRef& OutputTexture) {
		FDATextureReadbackChannelRCS::FParameters PassParameters;
		PassParameters.SourceTexture = Params.SourceTexture;
		PassParameters.SourceSampler = Params.SourceSampler;
		PassParameters.SourceDimensions = { (float)Params.SourceDimensions.X, (float)Params.SourceDimensions.Y };
		PassParameters.OutputTexture = RHICmdList.CreateUnorderedAccessView(OutputTexture,
			FRHIViewDesc::CreateTextureUAV()
				.SetDimensionFromTexture(OutputTexture)
				.SetMipLevel(0)
				.SetFormat(Params.PixelFormat)
				.SetArrayRange(0, 0)
			);

		TShaderMapRef<FDATextureReadbackChannelRCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters,
			FIntVector(FMath::DivideAndRoundUp(Params.SourceDimensions.X, DA_NUM_THREADS_PER_GROUP_DIMENSION),
				FMath::DivideAndRoundUp(Params.SourceDimensions.Y, DA_NUM_THREADS_PER_GROUP_DIMENSION), 1));


		return PassParameters.OutputTexture->GetTexture();
	}
	
	FRHITexture* DispatchReadbackShaderColor(FRHICommandListImmediate& RHICmdList, const FDATextureReadbackDispatchParams& Params, const FTextureRHIRef& OutputTexture) {
		FDATextureReadbackChannelRGBACS::FParameters PassParameters;
		PassParameters.SourceTexture = Params.SourceTexture;
		PassParameters.SourceSampler = Params.SourceSampler;
		PassParameters.SourceDimensions = { (float)Params.SourceDimensions.X, (float)Params.SourceDimensions.Y };
		PassParameters.OutputTexture = RHICmdList.CreateUnorderedAccessView(OutputTexture,
			FRHIViewDesc::CreateTextureUAV()
				.SetDimensionFromTexture(OutputTexture)
				.SetMipLevel(0)
				.SetFormat(Params.PixelFormat)
				.SetArrayRange(0, 0)
			);

		TShaderMapRef<FDATextureReadbackChannelRGBACS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters,
			FIntVector(FMath::DivideAndRoundUp(Params.SourceDimensions.X, DA_NUM_THREADS_PER_GROUP_DIMENSION),
				FMath::DivideAndRoundUp(Params.SourceDimensions.Y, DA_NUM_THREADS_PER_GROUP_DIMENSION), 1));


		return PassParameters.OutputTexture->GetTexture();
	}
} 

void FDATextureReadbackInterface::Dispatch_RenderThread(FRHICommandListImmediate& RHICmdList, const FDATextureReadbackDispatchParams& Params, const TFunction<void(void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight)>& AsyncCallback)
{
	check(Params.SourceTexture && Params.SourceSampler);

	FRHITextureCreateDesc TargetTextureDesc =
		FRHITextureCreateDesc::Create2D(TEXT("PCGTexture Readback Compute Target"), Params.SourceDimensions.X, Params.SourceDimensions.Y, Params.PixelFormat)
			.SetClearValue(FClearValueBinding::None)
			.SetFlags(ETextureCreateFlags::UAV | 
				ETextureCreateFlags::RenderTargetable |
				ETextureCreateFlags::ShaderResource |
				ETextureCreateFlags::NoTiling
			)
			.SetInitialState(ERHIAccess::UAVCompute)
			.DetermineInititialState();
	check(TargetTextureDesc.IsValid());
	
	// Create temporary output texture
	FTextureRHIRef OutputTexture = RHICreateTexture(TargetTextureDesc);

	if (Params.PixelFormat == PF_R8 || Params.PixelFormat == PF_R16F) {
		DATextureReadbackInterface::DispatchReadbackShaderRFloat(RHICmdList, Params, OutputTexture);
	}
	else if (Params.PixelFormat == PF_R8G8B8A8 || Params.PixelFormat == PF_B8G8R8A8) {
		DATextureReadbackInterface::DispatchReadbackShaderColor(RHICmdList, Params, OutputTexture);
	}
	else {
		UE_LOG(DungeonLog, Error, TEXT("FDATextureReadbackInterface: Unsupported readback texture format"));
		OutputTexture.SafeRelease();
		return;
	}

	// Prepare OutputTexture to be copied
	RHICmdList.Transition(FRHITransitionInfo(OutputTexture, ERHIAccess::UAVCompute, ERHIAccess::CopySrc));

	FRHIGPUTextureReadback* GPUTextureReadback = new FRHIGPUTextureReadback(TEXT("DATextureReadbackCopy"));
	GPUTextureReadback->EnqueueCopy(RHICmdList, OutputTexture->GetTexture2D());

	auto RunnerFunc = [GPUTextureReadback, AsyncCallback](auto&& RunnerFunc) -> void
	{
		if (GPUTextureReadback->IsReady())
		{
			int32 ReadbackWidth = 0, ReadbackHeight = 0;
			void* OutBuffer = GPUTextureReadback->Lock(ReadbackWidth, &ReadbackHeight);

			AsyncCallback(OutBuffer, ReadbackWidth, ReadbackHeight);

			GPUTextureReadback->Unlock();
			delete GPUTextureReadback;
		}
		else
		{
			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]()
			{
				RunnerFunc(RunnerFunc);
			});
		}
	};

	AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]()
	{
		RunnerFunc(RunnerFunc);
	});
}

void FDATextureReadbackInterface::Dispatch_GameThread(const FDATextureReadbackDispatchParams& Params, const TFunction<void(void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight)>& AsyncCallback)
{
	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			Dispatch_RenderThread(RHICmdList, Params, AsyncCallback);
		});
}

void FDATextureReadbackInterface::Dispatch(const FDATextureReadbackDispatchParams& Params, const TFunction<void(void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight)>& AsyncCallback)
{
	if (IsInRenderingThread())
	{
		Dispatch_RenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
	}
	else
	{
		Dispatch_GameThread(Params, AsyncCallback);
	}
}

#undef LOCTEXT_NAMESPACE
#undef DA_NUM_THREADS_PER_GROUP_DIMENSION

