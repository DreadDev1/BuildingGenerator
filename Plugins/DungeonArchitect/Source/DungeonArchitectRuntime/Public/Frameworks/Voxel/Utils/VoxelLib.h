//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UDungeonVoxelSDFModel;
struct FDAVoxelNoiseSettings;
struct FDAVoxelShape;
struct FDANoiseInfluenceZone;
struct FDAVoxelChunkDescriptor;

namespace DA {
	class DUNGEONARCHITECTRUNTIME_API FVoxelLib {
	public:
		// E: With SDF model, influence zones and descriptor support (for cached data)
		static float CalculateBaseSDF(const FVector& WorldPos, const TArray<FDAVoxelShape>& InShapes,
		                              float InVoxelSize, const FDAVoxelNoiseSettings& InGlobalNoiseSettings,
		                              const TArray<FDANoiseInfluenceZone>& InInfluenceZones,
		                              UDungeonVoxelSDFModel* SDFModel, const FDAVoxelChunkDescriptor* Descriptor,
		                              const FIntVector& InChunkCoord, const FBox& ChunkExpandedBounds, const FIntVector& VoxelCoord, uint8& OutMaterialID);

		static float CalculateShapeSDF(const FDAVoxelShape& Shape, const FVector& InWorldPos, float InVoxelSize,
			const FDAVoxelNoiseSettings& InGlobalNoiseSettings, const TArray<FDANoiseInfluenceZone>& InInfluenceZones, uint8& OutMaterialID);

		static FVector CalculateNoiseVector(const FVector& WorldPos, const FDAVoxelNoiseSettings& InNoiseSettings);

		static FVector CalculateNoiseVector(const FVector& WorldPos, const FDAVoxelShape& InShape, 
			const FDAVoxelNoiseSettings& InGlobalNoiseSettings, const TArray<FDANoiseInfluenceZone>& InInfluenceZones);
		static bool InvertNoiseDisplacement_Picard(const FVector& P0, FVector& OutP1, const FDAVoxelNoiseSettings& NoiseSettings,
			int32 MaxIters = 10, float Tol = 1e-3f, float Alpha = 0.8f);
		
	private:
		static float CalculateInfluenceZoneFactor(const FVector& WorldPos, const TArray<FDANoiseInfluenceZone>& InInfluenceZones,
			float& OutNoiseAmplitude, float& OutNoiseFloorScale, float& OutNoiseScaleNormalized, FVector& OutNoiseScaleVector);
		
	};

	class DUNGEONARCHITECTRUNTIME_API FVoxelMathUtils {
	public:
		static bool IsPointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon);
		static float DistanceToLineSegment2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B);
	};
	
	class DUNGEONARCHITECTRUNTIME_API FVoxelConstants {
	public:
		static const FName VoxelChunkActorTag;
		static const float VoxelScaleMultiplierGPU;
		static const float VoxelScaleMultiplierCPU;
	};

	class DUNGEONARCHITECTRUNTIME_API FVoxelLibSSE {
	public:
		static VectorRegister CalculateSphereDistanceVectorized(const VectorRegister& VecPosX, const VectorRegister& VecPosY, const VectorRegister& VecPosZ, const FVector& Center, float Radius); 
	};
}

