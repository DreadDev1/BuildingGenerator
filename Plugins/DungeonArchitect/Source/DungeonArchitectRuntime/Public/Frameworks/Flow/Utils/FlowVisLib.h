//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"

class FFlowVisLib {
public:
	struct FDAVertexInfo
	{
		FVector3d Position { FVector3d::Zero() };
		FVector3f Normal { FVector3f::Zero() };
		FVector4f Color { FVector4f::Zero() };
		FVector2f UV0 { FVector2f::Zero() };
		FVector2f UV1 { FVector2f::Zero() };
	};
	
	struct FGeometry {
		TArray<FDAVertexInfo> Vertices;
		TArray<UE::Geometry::FIndex3i> Triangles;
	};
	
	struct FLineInfo {
		FVector3f Start = {};
		FVector3f End = {};
		FVector4f Color;
	};

	struct FQuadInfo {
		FVector3f Location = {};
		FVector2f Size = {};
		FVector2f UVScale = {};
		FVector4f Color;
	};

	static void EmitGrid(const FIntPoint& NumCells, float CellSize, FGeometry& OutGeometry);
	static void EmitLine(const TArray<FLineInfo>& InLines, float InThickness, FGeometry& OutGeometry);
	static void EmitQuad(const TArray<FQuadInfo>& InQuads, FGeometry& OutGeometry);
	static void AddGeometry(UE::Geometry::FDynamicMesh3& InMesh, const FGeometry& InGeometry);
	static FDAVertexInfo CreateVertex(const FVector3f& InPosition, const FVector2f& InUV, const FVector2f& InUVScale, const FVector4f& InColor, const FVector3f& InNormal = FVector3f::UnitZ());
	static FBox GetBounds(const FGeometry& InGeometry);
	static void TranslateGeometry(const FVector3d& Offset, FGeometry& Geometry);
	static void CalculateNormals(const TArray<UE::Geometry::FIndex3i>& Triangles, TArray<FFlowVisLib::FDAVertexInfo>& Vertices);
	static void MergeGeometries(const FGeometry& A, const FGeometry& B, FGeometry& OutMerged);
	static void AppendGeometry(const FGeometry& InGeometryToAppend, FGeometry& OutTarget);
};

