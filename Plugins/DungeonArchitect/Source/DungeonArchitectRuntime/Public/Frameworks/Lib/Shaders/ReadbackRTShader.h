//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "PixelFormat.h"
#include "RHIFwd.h"

/**
 * Parameters to drive execution of a GPU texture readback. These mirror the fields in the texture readback compute shader (PCGTextureReadback.usf).
 */
struct DUNGEONARCHITECTRUNTIME_API FDATextureReadbackDispatchParams
{
	/** Source texture to sample from. Can be a UTexture2D or UTexture2DArray. */
	FTextureRHIRef SourceTexture;

	/** Sampler used to sample the SourceTexture. Should use an SF_Point filter for precise per-pixel readback. */
	FSamplerStateRHIRef SourceSampler;

	EPixelFormat PixelFormat = EPixelFormat::PF_R16F;
	
	/** Width and Height of the SourceTexture. Should match the underlying dimensions exactly for precise per-pixel readback. */
	FIntPoint SourceDimensions;
};
 
/**
 * API for dispatching TextureReadback operations to the GPU.
 */
class DUNGEONARCHITECTRUNTIME_API FDATextureReadbackInterface
{
public:
	static void Dispatch_RenderThread(FRHICommandListImmediate& RHICmdList, const FDATextureReadbackDispatchParams& Params, const TFunction<void(void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight)>& AsyncCallback);
	static void Dispatch_GameThread(const FDATextureReadbackDispatchParams& Params, const TFunction<void(void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight)>& AsyncCallback);

	/** Dispatches the texture readback compute shader. Can be called from any thread. */
	static void Dispatch(const FDATextureReadbackDispatchParams& Params, const TFunction<void(void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight)>& AsyncCallback);
};

