//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UDAChunkMeshCollection;
class UStaticMesh;
class UMaterialInterface;

namespace DA { struct FDungeonMeshGeometry; }

namespace DA { namespace Voxel {

/**
 * Create or update a baked static mesh for the given chunk inside the collection asset.
 * Returns the baked UStaticMesh (sub-asset of the collection) or nullptr on failure.
 * No-op at runtime (non-editor builds).
 */
UStaticMesh* BakeStaticMeshForChunk(UDAChunkMeshCollection* Collection, const FIntVector& ChunkCoord, const DA::FDungeonMeshGeometry& Geometry, UMaterialInterface* Material, const FVector& ChunkWorldPosition, float UVScale);

}} // namespace DA::Voxel

