//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

/**
 * 3D Noise texture for fast noise sampling
 * Thread-safe singleton implementation
 */
class DUNGEONARCHITECTRUNTIME_API FDungeonNoiseTexture3D {
public:
	static FDungeonNoiseTexture3D& Get();
	
	// Initialize the texture (thread-safe, can be called multiple times)
	void Initialize(int32 InSize = 128, int32 InNumOctaves = 4);
	
	// Sample the noise texture with trilinear interpolation
	float SampleNoise(const FVector& Position, float Scale = 0.1f) const;
	
	// Sample noise with offset (for multi-channel noise)
	FVector SampleNoiseVector(const FVector& Position, float Scale = 0.1f, float Separation = 1000.0f) const;
	
	// Check if initialized
	bool IsInitialized() const { return bInitialized; }
	
	// Get memory usage in bytes
	int32 GetMemoryUsage() const { return TextureData.Num() * sizeof(float); }
	
private:
	FDungeonNoiseTexture3D() = default;
	~FDungeonNoiseTexture3D() = default;
	
	// Delete copy/move constructors
	FDungeonNoiseTexture3D(const FDungeonNoiseTexture3D&) = delete;
	FDungeonNoiseTexture3D& operator=(const FDungeonNoiseTexture3D&) = delete;
	
	void GenerateNoiseTexture();
	
private:
	TArray<float> TextureData;
	int32 TextureSize = 128;
	int32 NumOctaves = 4;
	bool bInitialized = false;
	
	// Thread safety
	mutable FCriticalSection InitializationLock;
};