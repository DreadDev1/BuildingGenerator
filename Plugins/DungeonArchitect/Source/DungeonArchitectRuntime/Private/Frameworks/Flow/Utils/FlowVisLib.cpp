//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Utils/FlowVisLib.h"

#include "DynamicMesh/DynamicMeshAttributeSet.h"

////////////////////////////////////////// FFlowVisLib //////////////////////////////////////////
void FFlowVisLib::AddGeometry(UE::Geometry::FDynamicMesh3& InMesh, const FGeometry& InGeometry) {
	using namespace UE::Geometry;
	
	InMesh.Attributes()->SetNumUVLayers(2);
	FDynamicMeshUVOverlay* UV0Overlay = InMesh.Attributes()->GetUVLayer(0);
	FDynamicMeshUVOverlay* UV1Overlay = InMesh.Attributes()->GetUVLayer(1);
	FDynamicMeshNormalOverlay* NormalOverlay = InMesh.Attributes()->PrimaryNormals();
	FDynamicMeshColorOverlay* ColorOverlay = InMesh.Attributes()->PrimaryColors();
	
	auto AppendVertex = [&](const FDAVertexInfo& VertexInfo) {
		const int32 Index = InMesh.AppendVertex(VertexInfo.Position);
		UV0Overlay->AppendElement(VertexInfo.UV0);
		UV1Overlay->AppendElement(VertexInfo.UV1);
		NormalOverlay->AppendElement(VertexInfo.Normal);
		ColorOverlay->AppendElement(VertexInfo.Color);
		return Index;
	};

	constexpr int32 PolyID = 0;
	auto AppendTriangle = [&](const FIndex3i& Tri) {
		const int32 TriId = InMesh.AppendTriangle(Tri, PolyID);
		UV0Overlay->SetTriangle(TriId, Tri);
		UV1Overlay->SetTriangle(TriId, Tri);
		NormalOverlay->SetTriangle(TriId, Tri);
		ColorOverlay->SetTriangle(TriId, Tri);
		//return TriId;
	};

	TArray<int32> VertexLookup;
	VertexLookup.SetNum(InGeometry.Vertices.Num());
	for (int i = 0; i < InGeometry.Vertices.Num(); i++) {
		VertexLookup[i] = AppendVertex(InGeometry.Vertices[i]);
	}
	for (const FIndex3i& LocalTri : InGeometry.Triangles) {
		FIndex3i Tri(
			VertexLookup[LocalTri.A],
			VertexLookup[LocalTri.B],
			VertexLookup[LocalTri.C]);
		AppendTriangle(Tri);
	}
}

FFlowVisLib::FDAVertexInfo FFlowVisLib::CreateVertex(const FVector3f& InPosition, const FVector2f& InUV, const FVector2f& InUVScale, const FVector4f& InColor, const FVector3f& InNormal) {
	FDAVertexInfo Vertex;
	Vertex.Position = FVector3d(InPosition);
	Vertex.Normal = InNormal;
	Vertex.UV0 = InUV * InUVScale;
	Vertex.UV1 = InUVScale;
	Vertex.Color = InColor;
	return Vertex;
}

FBox FFlowVisLib::GetBounds(const FGeometry& InGeometry) {
	FBox Bounds;
	for (const FDAVertexInfo& Vertex : InGeometry.Vertices) {
		Bounds += Vertex.Position;
	}
	return Bounds;
}

void FFlowVisLib::TranslateGeometry(const FVector3d& Offset, FGeometry& Geometry) {
	for (FDAVertexInfo& Vertex : Geometry.Vertices) {
		Vertex.Position += Offset;
	}
}

void FFlowVisLib::CalculateNormals(const TArray<UE::Geometry::FIndex3i>& Triangles, TArray<FFlowVisLib::FDAVertexInfo>& Vertices) {
	for (FFlowVisLib::FDAVertexInfo& Vertex : Vertices) {
		Vertex.Normal = FVector3f::ZeroVector;
	}

	struct FDAVertSpatialNormalInfo {
		int32 Count{};
		FVector3f Normal{FVector3f::ZeroVector};
	};
		
	TArray<FDAVertSpatialNormalInfo> VertexNormalInfo;
	VertexNormalInfo.SetNumZeroed(Vertices.Num());
			
	for (const UE::Geometry::FIndex3i& Triangle : Triangles) {
		FVector3d V0 = Vertices[Triangle.A].Position;
		FVector3d V1 = Vertices[Triangle.B].Position;
		FVector3d V2 = Vertices[Triangle.C].Position;

		const FVector3f FaceNormal = FVector3f::CrossProduct(FVector3f(V2 - V0), FVector3f(V1 - V0)).GetSafeNormal();
		FDAVertSpatialNormalInfo& N0 = VertexNormalInfo[Triangle.A];
		FDAVertSpatialNormalInfo& N1 = VertexNormalInfo[Triangle.B];
		FDAVertSpatialNormalInfo& N2 = VertexNormalInfo[Triangle.C];

		N0.Normal += FaceNormal; N0.Count++;
		N1.Normal += FaceNormal; N1.Count++;
		N2.Normal += FaceNormal; N2.Count++;
	}

	for (int i = 0; i < Vertices.Num(); i++) {
		FDAVertSpatialNormalInfo& NormalInfo = VertexNormalInfo[i];
		if (NormalInfo.Count > 0) {
			Vertices[i].Normal = NormalInfo.Normal / NormalInfo.Count;
		}
		else {
			Vertices[i].Normal = {0, 0, 1};
		}
	}
}

void FFlowVisLib::MergeGeometries(const FGeometry& A, const FGeometry& B, FGeometry& OutMerged) {
	OutMerged = A;
	const int BaseIndex = A.Vertices.Num();
	OutMerged.Vertices.Append(B.Vertices);
	for (const UE::Geometry::FIndex3i& TriB : B.Triangles) {
		UE::Geometry::FIndex3i& OutTri = OutMerged.Triangles.AddDefaulted_GetRef();
		OutTri.A = BaseIndex + TriB.A;
		OutTri.B = BaseIndex + TriB.B;
		OutTri.C = BaseIndex + TriB.C;
	}
}

void FFlowVisLib::AppendGeometry(const FGeometry& InGeometryToAppend, FGeometry& OutTarget) {
	const int BaseIndex = OutTarget.Vertices.Num();
	OutTarget.Vertices.Append(InGeometryToAppend.Vertices);
	for (const UE::Geometry::FIndex3i& TriToAppend : InGeometryToAppend.Triangles) {
		UE::Geometry::FIndex3i& OutTri = OutTarget.Triangles.AddDefaulted_GetRef();
		OutTri.A = BaseIndex + TriToAppend.A;
		OutTri.B = BaseIndex + TriToAppend.B;
		OutTri.C = BaseIndex + TriToAppend.C;
	}
}

void FFlowVisLib::EmitLine(const TArray<FLineInfo>& InLines, float InThickness, FGeometry& OutGeometry) {
	using namespace UE::Geometry;
	
	for (const FLineInfo& LineInfo : InLines) {
		FVector3f Direction = (LineInfo.End - LineInfo.Start).GetSafeNormal();
		FVector3f Side = FVector3f::CrossProduct(Direction, FVector3f::UnitZ()) * InThickness;

		const int32 I00 = OutGeometry.Vertices.Add(CreateVertex(LineInfo.Start - Side, FVector2f(0, 0), {1, 1}, LineInfo.Color));
		const int32 I10 = OutGeometry.Vertices.Add(CreateVertex(LineInfo.Start + Side, FVector2f(1, 0), {1, 1}, LineInfo.Color));
		const int32 I11 = OutGeometry.Vertices.Add(CreateVertex(LineInfo.End + Side, FVector2f(1, 1), {1, 1}, LineInfo.Color));
		const int32 I01 = OutGeometry.Vertices.Add(CreateVertex(LineInfo.End - Side, FVector2f(0, 1), {1, 1}, LineInfo.Color));
			
		OutGeometry.Triangles.Add(FIndex3i(I00, I11, I10));
		OutGeometry.Triangles.Add(FIndex3i(I00, I01, I11));
	}
}

void FFlowVisLib::EmitQuad(const TArray<FQuadInfo>& InQuads, FGeometry& OutGeometry) {
	using namespace UE::Geometry;

	for (const FQuadInfo& Quad : InQuads) {
		FVector3f V00 = Quad.Location;
		FVector3f V10 = Quad.Location + FVector3f(Quad.Size.X, 0, 0);
		FVector3f V11 = Quad.Location + FVector3f(Quad.Size.X, Quad.Size.Y, 0);
		FVector3f V01 = Quad.Location + FVector3f(0, Quad.Size.Y, 0);
		
		const int32 I00 = OutGeometry.Vertices.Add(CreateVertex(V00, FVector2f(0, 0), Quad.UVScale, Quad.Color));
		const int32 I10 = OutGeometry.Vertices.Add(CreateVertex(V10, FVector2f(1, 0), Quad.UVScale, Quad.Color));
		const int32 I11 = OutGeometry.Vertices.Add(CreateVertex(V11, FVector2f(1, 1), Quad.UVScale, Quad.Color));
		const int32 I01 = OutGeometry.Vertices.Add(CreateVertex(V01, FVector2f(0, 1), Quad.UVScale, Quad.Color));
			
		OutGeometry.Triangles.Add(FIndex3i(I00, I11, I10));
		OutGeometry.Triangles.Add(FIndex3i(I00, I01, I11));
	}
}


void FFlowVisLib::EmitGrid(const FIntPoint& NumCells, float CellSize, FGeometry& OutGeometry) {
	using namespace UE::Geometry;

	constexpr float LineThickness = 20.0f;
	const FVector3f Color = FVector3f(0, 0, 0);
	TArray<FLineInfo> Lines;
	for (int y = 0; y <= NumCells.Y; y++) {
		Lines.Add({
			FVector3f(0, y, 0) * CellSize,
			FVector3f(NumCells.X, y, 0) * CellSize,
			Color
		});
	}

	for (int x = 0; x <= NumCells.X; x++) {
		Lines.Add({
			FVector3f(x, 0, 0) * CellSize,
			FVector3f(x, NumCells.Y, 0) * CellSize,
			Color
		});
	}
	
	EmitLine(Lines, LineThickness, OutGeometry);
}


