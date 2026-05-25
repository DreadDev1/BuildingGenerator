//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Utils/FlowVisLib.h"

namespace DA {
	namespace DCEL {
		struct FEdge;
	}
}

struct FCellFlowLevelMeshGenSettings;
class UDASceneDebugDataComponent;
class UFlowAbstractNode;
class UCellFlowLayoutGraph;
class UCellFlowVoronoiGraph;
class UDAFlowCellGraph;
class UDynamicMeshComponent;

class FCellFlowLevelMeshLib {
public:
	static void UploadGeometry(const FFlowVisLib::FGeometry& InGeometry, TObjectPtr<UDynamicMeshComponent> InComponent);
	static int GetGlobalCliffDepth(const UCellFlowLayoutGraph* LayoutGraph, const UDAFlowCellGraph* CellGraph, const TMap<FGuid, const UFlowAbstractNode*>& LayoutNodes);
	static void RenderStairs(const FVector3f& InLocation, const UE::Math::TQuat<float>& InRotation, const FVector3f& InScale, const FVector2f& InUVScale, const FVector4f& InColor,
			TFunction<int(const FFlowVisLib::FDAVertexInfo& InVertexInfo)> CreateVertex, TFunction<void(const UE::Geometry::FIndex3i& InTriangle)> CreateTriangle);
	static void RenderStairs(const FVector3f& InLocation, const UE::Math::TQuat<float>& InRotation, const FVector3f& InScale,
					const FVector4f& InColor, FFlowVisLib::FGeometry& OutSurfaceGeometry, const FVector2f& InUVScale = {1, 1});
	
	class FUniqueLineCollection {
	public:
		FUniqueLineCollection(const FVector3f InGridSize, const FVector4f& InColor, float InOffsetZ)
			: GridSize(InGridSize)
			  , Color(InColor)
			  , OffsetZ(InOffsetZ) {
		}

		FORCEINLINE void Add(const DA::DCEL::FEdge* InEdge, int LogicalZ) {
			Add(InEdge, LogicalZ, Color);
		}
		void Add(const DA::DCEL::FEdge* InEdge, int LogicalZ, const FVector4f& InColor);
		FORCEINLINE const TArray<FFlowVisLib::FLineInfo>& GetLines() const { return Lines; }
		FVector4f GetColor() const { return Color; }

	private:
		FVector3f GridSize{};
		FVector4f Color{};
		float OffsetZ{};
		TArray<FFlowVisLib::FLineInfo> Lines;
	};
};

