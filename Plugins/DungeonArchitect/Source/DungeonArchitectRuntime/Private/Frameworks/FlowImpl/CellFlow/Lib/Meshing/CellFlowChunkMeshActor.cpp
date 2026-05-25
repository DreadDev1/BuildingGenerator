//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/CellFlowChunkMeshActor.h"

#include "Frameworks/Meshing/Geometry/DungeonProceduralMesh.h"

ACellFlowChunkMesh::ACellFlowChunkMesh() {
	MeshComponent = CreateDefaultSubobject<UDAProcMeshComponent>("Mesh");
	SetRootComponent(MeshComponent);
}

void ACellFlowChunkMesh::UploadGeometry(int32 LODIndex, int32 SectionIndex, const FFlowVisLib::FGeometry& SurfaceGeometry, bool bEnableCollision) const {
	auto ConvertToProcVert = [](const TArray<FFlowVisLib::FDAVertexInfo>& InVertices, TArray<FDAProcMeshVertex>& OutVertices) {
		OutVertices.SetNum(InVertices.Num());

		for (int i = 0; i < InVertices.Num(); i++) {
			const FFlowVisLib::FDAVertexInfo& InVert = InVertices[i];
			FDAProcMeshVertex& OutVert = OutVertices[i];
			OutVert.Position = FVector(InVert.Position);
			OutVert.Normal = FVector(InVert.Normal);
			OutVert.Tangent.TangentX = FVector::UnitX(); //-FVector::CrossProduct(FVector(InVert.Normal), -FVector::UnitY());
			OutVert.Tangent.bFlipTangentY = false;
			OutVert.Color = FLinearColor(InVert.Color).ToFColor(true);
			OutVert.UV0 = FVector2D(InVert.UV0);
			OutVert.UV1 = FVector2D(InVert.UV1);
		}
	};

	auto ConvertToIndices = [](const TArray<UE::Geometry::FIndex3i>& InTriangles, TArray<int32>& OutIndices) {
		OutIndices.SetNum(InTriangles.Num() * 3);
		for (int i = 0; i < InTriangles.Num(); i++) {
			const int32 BaseIdx = i * 3;
			OutIndices[BaseIdx] = InTriangles[i].A;
			OutIndices[BaseIdx + 1] = InTriangles[i].B;
			OutIndices[BaseIdx + 2] = InTriangles[i].C;
		}
	};
	
	TArray<FDAProcMeshVertex> Vertices;
	TArray<int32> Indices;

	ConvertToProcVert(SurfaceGeometry.Vertices, Vertices);
	ConvertToIndices(SurfaceGeometry.Triangles, Indices);
	
	MeshComponent->CreateMeshSection(LODIndex, SectionIndex, Vertices, Indices, PT_TriangleList, bEnableCollision);
}

