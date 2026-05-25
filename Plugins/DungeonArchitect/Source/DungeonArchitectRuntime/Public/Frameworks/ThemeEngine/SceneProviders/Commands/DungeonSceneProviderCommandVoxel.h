//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDataGPU.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

#include "RHIGPUReadback.h"
#include "RenderGraphResources.h"
#include <atomic>

struct FDAVoxelChunkDataCPU;
struct FDAVoxelChunkInfo;

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_GenerateVoxelMasterWorld : public FDungeonSceneProviderCommand {
public:
	FDungeonSceneProviderCommand_GenerateVoxelMasterWorld(
		const FDungeonSceneProviderContext& InContext,
		const TSharedPtr<FDAVoxelChunkDescriptor>& InDescriptor)
		: FDungeonSceneProviderCommand(InContext)
		, Descriptor(InDescriptor)
	{
	}

	virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
		ExecutionPriority = MAX_int32;	// Run in the end after all the meshes have been spawned in
	}
	
protected:
	virtual void ExecuteImpl(UWorld* World) override;
	
private:
	TSharedPtr<FDAVoxelChunkDescriptor> Descriptor;
};


////////////////// Chunk Generation Command (CPU) //////////////////
class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_GenerateVoxelChunkBase : public FDungeonSceneProviderCommand {
public:
	FDungeonSceneProviderCommand_GenerateVoxelChunkBase(
		const FDungeonSceneProviderContext& InContext,
		const TSharedPtr<FDAVoxelChunkDescriptor>& InDescriptor,
		const FIntVector& InChunkCoord)
		: FDungeonSceneProviderCommand(InContext)
		, Descriptor(InDescriptor)
		, ChunkCoord(InChunkCoord) 
	{
	}
	
	virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;

protected:
	bool CanGenerateGeometry(const TArray<float>& DensityArray) const;
	void GenerateMesh(UWorld* InWorld, const FDAVoxelChunkDataCPU& ChunkData);
	
protected:
	TSharedPtr<FDAVoxelChunkDescriptor> Descriptor;
	FIntVector ChunkCoord;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_GenerateVoxelChunkCPU : public FDungeonSceneProviderCommand_GenerateVoxelChunkBase {
public:
	FDungeonSceneProviderCommand_GenerateVoxelChunkCPU(
		const FDungeonSceneProviderContext& InContext,
		const TSharedPtr<FDAVoxelChunkDescriptor>& InDescriptor,
		const FIntVector& InChunkCoord)
		: FDungeonSceneProviderCommand_GenerateVoxelChunkBase(InContext, InDescriptor, InChunkCoord)
	{
	}
	
protected:
	virtual void ExecuteImpl(UWorld* World) override;
	
private:
	void GenerateDensity(const FDAVoxelChunkInfo& ChunkInfo, FDAVoxelChunkDataCPU& ChunkData) const;
	void GenerateDensity_SimpleSlow(const FDAVoxelChunkInfo& ChunkInfo, FDAVoxelChunkDataCPU& ChunkData) const;
	
	float CalculateNoise(const FVector& WorldPos) const;
};

////////////////// Chunk Generation Command (GPU) //////////////////

// GPU command for async voxel generation
class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_GenerateVoxelChunkGPU
	: public FDungeonSceneProviderCommand_GenerateVoxelChunkBase
	, public TSharedFromThis<FDungeonSceneProviderCommand_GenerateVoxelChunkGPU>
{
public:
	FDungeonSceneProviderCommand_GenerateVoxelChunkGPU(
		const FDungeonSceneProviderContext& InContext,
		const TSharedPtr<FDAVoxelChunkDescriptor>& InDescriptor,
		const FIntVector& InChunkCoord);

	virtual ~FDungeonSceneProviderCommand_GenerateVoxelChunkGPU();

	virtual void Tick(UWorld* World, float DeltaTime) override;
	
protected:
	virtual void ExecuteImpl(UWorld* World) override;
	
private:
	enum class EGPUState {
		NotStarted,
		ComputeDispatched,
		ReadbackInitiated,
		ReadbackPending,
		DataProcessing,
		MeshGenerating,
		Completed
	};
	
	void PrepareShapeData();
	void DispatchComputeShader();
	void InitiateReadback();
	bool IsReadbackReady();
	void ProcessReadbackData();
	void ProcessPhaseGenerateMesh(UWorld* World);
	void ReleaseGPUResources();
	bool IsDataProcessed() const;
	
private:
	EGPUState GPUState = EGPUState::NotStarted;
	
	// Shape data separated by layer
	TArray<FDAVoxelShapeGPU> PrimaryCarveShapes;
	TArray<FDAVoxelShapeGPU> AddGeometryShapes;
	
	// GPU resources (temporary per chunk)
	TRefCountPtr<FRDGPooledBuffer> PrimaryCarveBuffer;
	TRefCountPtr<FRDGPooledBuffer> AddGeometryBuffer;
	TRefCountPtr<FRDGPooledBuffer> DensityBuffer;
	TRefCountPtr<FRDGPooledBuffer> MaterialBuffer;
	
	// Readback objects
	FRHIGPUBufferReadback* DensityReadback = nullptr;
	FRHIGPUBufferReadback* MaterialReadback = nullptr;
	
	// Chunk data for result storage
	TSharedPtr<struct FDAVoxelChunkDataCPU> ChunkData;
	
	// Thread-safe flag to track if data has been copied from GPU
	mutable std::atomic<bool> bDataCopied{false};
};

