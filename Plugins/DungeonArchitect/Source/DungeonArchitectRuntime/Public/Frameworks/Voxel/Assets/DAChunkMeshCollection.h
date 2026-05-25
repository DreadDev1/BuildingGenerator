//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DAChunkMeshCollection.generated.h"

class UStaticMesh;

/**
 * Bake settings applied when generating static meshes for voxel chunks.
 * Extended settings (collision proxies, LOD generation, etc) can be added later.
 */
USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDAVoxelStaticBakeSettings {
    GENERATED_BODY()

    /** Enable Nanite on baked static meshes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    bool bEnableNanite = true;

    /** Generate lightmap UVs during build */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    bool bGenerateLightmapUVs = true;

    /** Target lightmap resolution for baked meshes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake", meta=(ClampMin="1", UIMin="16", UIMax="1024"))
    int32 LightmapResolution = 64;

    /** Use complex-as-simple collision on baked meshes (fast path to start with) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake|Collision")
    bool bUseComplexAsSimpleCollision = true;
};

/**
 * A collection asset that holds baked static meshes for voxel chunks.
 * Stores a map of ChunkCoord -> UStaticMesh created as sub-assets of this asset.
 */
UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UDAChunkMeshCollection : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** Map of chunk coordinates to baked static meshes (stored as sub-assets of this asset) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
    TMap<FIntVector, TObjectPtr<UStaticMesh>> StaticMeshes;

    /** Default bake settings used when creating/updating baked chunk meshes */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
    FDAVoxelStaticBakeSettings BakeSettings;
};

