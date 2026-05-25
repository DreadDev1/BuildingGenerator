//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshLib.h"

#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "Frameworks/Lib/Geometry/DCEL.h"

#include "Components/DynamicMeshComponent.h"

void FCellFlowLevelMeshLib::UploadGeometry(const FFlowVisLib::FGeometry& InGeometry,
                                           TObjectPtr<UDynamicMeshComponent> InComponent) {
	FDynamicMesh3 Mesh(true, true, true, false);;
	Mesh.EnableTriangleGroups();
	Mesh.EnableAttributes();
	Mesh.Attributes()->EnablePrimaryColors();
	FFlowVisLib::AddGeometry(Mesh, InGeometry);
	InComponent->SetMesh(MoveTemp(Mesh));
	
	InComponent->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	InComponent->EnableComplexAsSimpleCollision();
	InComponent->UpdateCollision(false);
	
}

int FCellFlowLevelMeshLib::GetGlobalCliffDepth(const UCellFlowLayoutGraph* LayoutGraph,
													  const UDAFlowCellGraph* CellGraph,
													  const TMap<FGuid, const UFlowAbstractNode*>& LayoutNodes) {
	int GlobalCliffDepth{};
	bool bFirstEntry{true};
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		if (GroupNode.LeafNodes.Num() > 0 && GroupNode.LayoutNodeID.IsValid()) {
			const UFlowAbstractNode* const* LayoutNodePtr = LayoutNodes.Find(GroupNode.LayoutNodeID);
			if (!LayoutNodePtr) continue;
			const UFlowAbstractNode* LayoutNode = *LayoutNodePtr;
			if (!LayoutNode->bActive) continue;

			constexpr int CliffDepth = 5;
			const UDAFlowCellLeafNode* LeafNode = CellGraph->LeafNodes[*GroupNode.LeafNodes.begin()];
			if (bFirstEntry) {
				bFirstEntry = false;
				GlobalCliffDepth = LeafNode->LogicalZ - CliffDepth;
			}
			else {
				GlobalCliffDepth = FMath::Min(GlobalCliffDepth, LeafNode->LogicalZ - CliffDepth);
			}
		}
	};

	return GlobalCliffDepth;
}

void FCellFlowLevelMeshLib::RenderStairs(const FVector3f& InLocation, const UE::Math::TQuat<float>& InRotation,
			const FVector3f& InScale, const FVector2f& InUVScale, const FVector4f& InColor,
			TFunction<int(const FFlowVisLib::FDAVertexInfo& InVertexInfo)> CreateVertex,
			TFunction<void(const UE::Geometry::FIndex3i& InTriangle)> CreateTriangle)
{
	static const TArray<FVector3f> LocalStairVerts = {
		{-.5f, -.5f, 0},
		{.5f, .5f, 1},
		{.5f, -.5f, 0},

		{-.5f, -.5f, 0},
		{-.5f, .5f, 1},
		{.5f, .5f, 1},

		{-.5f, -.5f, 0},
		{-.5f, .5f, 0},
		{-.5f, .5f, 1},

		{.5f, -.5f, 0},
		{.5f, .5f, 1},
		{.5f, .5f, 0},
	};

	static const TArray<FVector2f> LocalStairUV = {
		{0, 0},
		{1, 1},
		{1, 0},

		{0, 0},
		{0, 1},
		{1, 1},

		{0, 0},
		{0, 1},
		{0.5f, 0.5f},

		{0, 0},
		{0, 1},
		{0.5f, 0.5f},
	};

	static const TArray<int32> LocalStairIndices = {
		0, 1, 2,
		3, 4, 5,

		6, 7, 8,
		9, 10, 11,
	};
	using namespace UE::Geometry;

	TArray<int32> StairIndices;
	TArray<FVector3f> VertexLocations;
	for (int i = 0; i < LocalStairVerts.Num(); i++) {
		const FVector3f VertLoc = InLocation + InRotation.RotateVector(LocalStairVerts[i] * InScale);
		VertexLocations.Add(VertLoc);
	}

	TArray<FVector3f> TriNormals;
	for (int i = 0; i + 2 < LocalStairVerts.Num(); i += 3) {
		FVector3f TriNormal = FVector3f::CrossProduct((VertexLocations[i + 0] - VertexLocations[i + 1]).GetSafeNormal(),
													  (VertexLocations[i + 2] - VertexLocations[i + 1]).GetSafeNormal())
			.GetSafeNormal();
		TriNormals.Add(TriNormal);
	}

	for (int i = 0; i < LocalStairVerts.Num(); i++) {
		const int32 VertIndex = CreateVertex(FFlowVisLib::CreateVertex(VertexLocations[i], LocalStairUV[i], InUVScale, InColor, TriNormals[i / 3]));
		StairIndices.Add(VertIndex);
	}

	for (int i = 0; i + 2 < LocalStairIndices.Num(); i += 3) {
		CreateTriangle(FIndex3i(
			StairIndices[LocalStairIndices[i]],
			StairIndices[LocalStairIndices[i + 1]],
			StairIndices[LocalStairIndices[i + 2]]));
	}
}

void FCellFlowLevelMeshLib::RenderStairs(const FVector3f& InLocation, const UE::Math::TQuat<float>& InRotation,
                                         const FVector3f& InScale,
                                         const FVector4f& InColor, FFlowVisLib::FGeometry& OutSurfaceGeometry,
                                         const FVector2f& InUVScale) {
	RenderStairs(InLocation, InRotation, InScale, InUVScale, InColor,
		[&OutSurfaceGeometry](const FFlowVisLib::FDAVertexInfo& InVertexInfo) {
			return OutSurfaceGeometry.Vertices.Add(InVertexInfo);
		},
		[&OutSurfaceGeometry](const UE::Geometry::FIndex3i& InTriangle) {
			OutSurfaceGeometry.Triangles.Add(InTriangle);
		});
}

void FCellFlowLevelMeshLib::FUniqueLineCollection::Add(const DA::DCEL::FEdge* InEdge, int LogicalZ,
	const FVector4f& InColor) {
	FFlowVisLib::FLineInfo& Line = Lines.AddDefaulted_GetRef();
	const FVector2d& Start2D = InEdge->Origin->Location;
	const FVector2d& End2D = InEdge->Twin->Origin->Location;
	Line.Start = FVector3f(Start2D.X, Start2D.Y, LogicalZ) * GridSize + FVector3f(0, 0, OffsetZ);
	Line.End = FVector3f(End2D.X, End2D.Y, LogicalZ) * GridSize + FVector3f(0, 0, OffsetZ);
	Line.Color = InColor;
}

