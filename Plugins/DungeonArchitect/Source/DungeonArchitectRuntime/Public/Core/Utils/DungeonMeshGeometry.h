//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DynamicMesh/InfoTypes.h"

namespace DA {
	struct FDungeonMeshGeometry {
		TArray<UE::Geometry::FVertexInfo> Vertices;
		TArray<UE::Geometry::FIndex3i> Triangles;

		uint32 GetUsedMemory() const {
			return Vertices.Num() * sizeof(UE::Geometry::FVertexInfo)
				+ Triangles.Num() * sizeof(UE::Geometry::FIndex3i);
		}
	};
	typedef TSharedPtr<FDungeonMeshGeometry, ESPMode::ThreadSafe> FDungeonMeshGeometryPtr;
}

