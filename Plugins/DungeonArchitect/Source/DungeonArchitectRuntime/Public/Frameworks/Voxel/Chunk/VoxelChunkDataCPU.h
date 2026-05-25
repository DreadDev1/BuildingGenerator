//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FDAVoxelChunkDataCPU {
	TArray<float> DensityData;
	TArray<uint8> MaterialData;
	FIntVector ChunkCoord;
	FIntVector VoxelMin;
	FIntVector VoxelMax;
	
	void Initialize(const FIntVector& InChunkCoord, const FIntVector& InVoxelMin, const FIntVector& InVoxelMax);
	
	FORCEINLINE int32 GetLinearIndex(int32 X, int32 Y, int32 Z) const {
		const FIntVector Coord(X, Y, Z);
		return GetLinearIndex(Coord);
	}

	FORCEINLINE int32 GetLinearIndex(const FIntVector& Coord) const {
		const FIntVector Size = VoxelMax - VoxelMin + FIntVector(1, 1, 1);
		const FIntVector Local = Coord - VoxelMin;
		return Local.Z + Local.Y * Size.Z + Local.X * Size.Y * Size.Z;
	}
	
	FORCEINLINE float GetDensity(int32 X, int32 Y, int32 Z) const {
		const int32 Index = GetLinearIndex(X, Y, Z);
		check(DensityData.IsValidIndex(Index));
		return DensityData[Index];
	}
	
	FORCEINLINE void SetDensity(int32 X, int32 Y, int32 Z, float Value) {
		const int32 Index = GetLinearIndex(X, Y, Z);
		check(DensityData.IsValidIndex(Index));
		DensityData[Index] = Value;
	}
	
	FORCEINLINE uint8 GetMaterial(int32 X, int32 Y, int32 Z) const {
		const int32 Index = GetLinearIndex(X, Y, Z);
		check(MaterialData.IsValidIndex(Index));
		return MaterialData[Index];
	}
	
	FORCEINLINE void SetMaterial(int32 X, int32 Y, int32 Z, uint8 Value) {
		const int32 Index = GetLinearIndex(X, Y, Z);
		check(MaterialData.IsValidIndex(Index));
		MaterialData[Index] = Value;
	}
	
};

