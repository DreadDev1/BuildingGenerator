//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonNoiseTexture3D.h"


FDungeonNoiseTexture3D& FDungeonNoiseTexture3D::Get() {
	static FDungeonNoiseTexture3D Instance;
	return Instance;
}

void FDungeonNoiseTexture3D::Initialize(int32 InSize, int32 InNumOctaves) {
	FScopeLock Lock(&InitializationLock);
	
	if (bInitialized) {
		return; // Already initialized
	}
	
	TextureSize = InSize;
	NumOctaves = InNumOctaves;
	
	GenerateNoiseTexture();
	bInitialized = true;
	
	UE_LOG(LogTemp, Log, TEXT("DungeonNoiseTexture3D initialized with size %dx%dx%d (%.2f MB)"), 
		TextureSize, TextureSize, TextureSize, 
		GetMemoryUsage() / (1024.0f * 1024.0f));
}

void FDungeonNoiseTexture3D::GenerateNoiseTexture() {
	const int32 Size = TextureSize;
	const int32 TotalSize = Size * Size * Size;
	TextureData.SetNum(TotalSize);
	
	// Generate 3D noise texture
	const float InvSize = 1.0f / Size;
	const float NoiseScale = 4.0f; // Scale for noise frequency
	
	for (int32 Z = 0; Z < Size; Z++) {
		for (int32 Y = 0; Y < Size; Y++) {
			for (int32 X = 0; X < Size; X++) {
				// Generate smooth noise using multiple octaves
				FVector Pos(X * InvSize * NoiseScale, 
						   Y * InvSize * NoiseScale, 
						   Z * InvSize * NoiseScale);
				
				float Value = 0.0f;
				float Amplitude = 1.0f;
				float FreqScale = 1.0f;
				
				// 4 octaves of noise for good detail
				for (int32 Octave = 0; Octave < 6; Octave++) {
					Value += FMath::PerlinNoise3D(Pos * FreqScale) * Amplitude;
					Amplitude *= 0.5f;
					FreqScale *= 2.0f;
				}
				
				const int32 Index = X + Y * Size + Z * Size * Size;
				TextureData[Index] = Value;
			}
		}
	}
}

float FDungeonNoiseTexture3D::SampleNoise(const FVector& Position, float Scale) const {
	if (!bInitialized) {
		return 0.0f; // Return 0 if not initialized
	}
	
	const int32 Size = TextureSize;
	
	// Scale and wrap coordinates
	float X = FMath::Fmod(Position.X * Scale, Size);
	float Y = FMath::Fmod(Position.Y * Scale, Size);
	float Z = FMath::Fmod(Position.Z * Scale, Size);
	
	// Handle negative wrapping
	if (X < 0) X += Size;
	if (Y < 0) Y += Size;
	if (Z < 0) Z += Size;
	
	// Get integer coordinates
	int32 X0 = FMath::FloorToInt(X);
	int32 Y0 = FMath::FloorToInt(Y);
	int32 Z0 = FMath::FloorToInt(Z);
	
	// Wrap indices
	X0 = X0 % Size;
	Y0 = Y0 % Size;
	Z0 = Z0 % Size;
	
	int32 X1 = (X0 + 1) % Size;
	int32 Y1 = (Y0 + 1) % Size;
	int32 Z1 = (Z0 + 1) % Size;
	
	// Get fractional parts
	float FracX = X - FMath::Floor(X);
	float FracY = Y - FMath::Floor(Y);
	float FracZ = Z - FMath::Floor(Z);
	
	// Sample 8 corners of the cube
	float V000 = TextureData[X0 + Y0 * Size + Z0 * Size * Size];
	float V100 = TextureData[X1 + Y0 * Size + Z0 * Size * Size];
	float V010 = TextureData[X0 + Y1 * Size + Z0 * Size * Size];
	float V110 = TextureData[X1 + Y1 * Size + Z0 * Size * Size];
	float V001 = TextureData[X0 + Y0 * Size + Z1 * Size * Size];
	float V101 = TextureData[X1 + Y0 * Size + Z1 * Size * Size];
	float V011 = TextureData[X0 + Y1 * Size + Z1 * Size * Size];
	float V111 = TextureData[X1 + Y1 * Size + Z1 * Size * Size];
	
	// Trilinear interpolation
	float V00 = FMath::Lerp(V000, V100, FracX);
	float V10 = FMath::Lerp(V010, V110, FracX);
	float V01 = FMath::Lerp(V001, V101, FracX);
	float V11 = FMath::Lerp(V011, V111, FracX);
	float V0 = FMath::Lerp(V00, V10, FracY);
	float V1 = FMath::Lerp(V01, V11, FracY);
	
	return FMath::Lerp(V0, V1, FracZ);
}

FVector FDungeonNoiseTexture3D::SampleNoiseVector(const FVector& Position, float Scale, float Separation) const {
	// Sample noise at different offsets for each component
	FVector Result;
	Result.X = SampleNoise(Position + FVector(Separation, 0, 0), Scale);
	Result.Y = SampleNoise(Position + FVector(0, Separation, 0), Scale);
	Result.Z = SampleNoise(Position + FVector(0, 0, Separation), Scale);
	return Result;
}

