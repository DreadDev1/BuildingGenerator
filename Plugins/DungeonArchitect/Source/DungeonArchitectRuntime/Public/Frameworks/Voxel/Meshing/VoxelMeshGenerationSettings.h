//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "VoxelMeshGenerationSettings.generated.h"

class UDungeonThemeAsset;
class UDungeonVoxelSDFModel;
class UDAChunkMeshCollection;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAVoxelMeshGenerationSettings {
    GENERATED_BODY()

	FDAVoxelMeshGenerationSettings();
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
    float VoxelSize = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings", meta=(UIMin = "8", UIMax = "128"))
    int32 VoxelChunkSize = 32;

    /** Default material (deprecated - use material settings below) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
    TObjectPtr<UMaterialInterface> Material = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
	TSoftObjectPtr<UDungeonThemeAsset> VoxelShapeTheme;

    /** UV scale for texture mapping (smaller values = larger texture) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Materials", meta=(UIMin = "10", UIMax = "1000"))
    float UVScale = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
    bool bEnableCollision = true;

    /** Thickness of walls generated from outline data */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
    float WallThickness = 200.0f;
    
    /** Use GPU compute shaders for voxel density generation (faster but requires GPU support) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
    bool bUseGPU = true;

    /** Optional: assign a collection asset to bake voxel chunks into static meshes in editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Mesh Settings")
    TObjectPtr<UDAChunkMeshCollection> BakeCollectionAsset = {};
};

// Noise settings for voxel generation
USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAVoxelNoiseSettings {
    GENERATED_BODY()
	
    /** Scale of the noise pattern */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", DisplayName="Noise Scale")
    float NoiseScaleNormalized = 1.0f;
	
    /** Maximum displacement amplitude */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
    float NoiseAmplitude = 350.0f;
	
    /** Number of octaves for fractal noise */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta=(UIMin = "1", UIMax = "8"))
    int32 NoiseOctaves = 4;
	
    /** Base offset for noise sampling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
    FVector NoiseOffset = FVector::ZeroVector;
	
    /** Scale multiplier for floor Z-axis noise (0=flat floors, 1=same as walls) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta=(UIMin = "0", UIMax = "1"))
    float NoiseFloorScale = 0.3f;
    
    /** Scale multiplier for ceiling Z-axis noise (0=flat ceilings, 1=same as walls) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta=(UIMin = "0", UIMax = "1"))
    float NoiseCeilingScale = 1.0f;
    
    /** Scaling factor for the noise pattern (creates elongated/stretched features) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Scaling")
    FVector NoiseScaleVector = FVector(1.0f, 1.0f, 1.0f);

    /** Enable domain warping for more organic, eroded-looking patterns (uses IQ's nested FBM formula) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Domain Warp", meta=(HideInDetailPanel))
    bool bEnableDomainWarp = false;
    
    /** Strength of the domain warp distortion (0 = no warp, higher = more distortion) 
     * Automatically scales with noise scale to maintain proportional distortion */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Domain Warp", meta=(EditCondition="bEnableDomainWarp", UIMin = "0", UIMax = "1.0", HideInDetailPanel))
    float DomainWarpStrength = 0.3f;
    
    /** Height at which noise transitions from floor to ceiling scaling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Noise")
    float FloorCeilingTransitionHeight = 400.0f;
	
	/** Separation between X, Y, Z noise channels to avoid correlation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Noise")
	float NoiseChannelSeparation = 10000.0f;
	
	/** Whether to use different seeds for each axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Noise")
	bool bUsePerAxisSeeds = true;

	float GetNoiseScale(bool bIsGPU) const;
};

