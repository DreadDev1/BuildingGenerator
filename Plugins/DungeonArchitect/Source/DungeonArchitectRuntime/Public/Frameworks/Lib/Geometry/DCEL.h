//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

namespace DA {
	class FVoronoiGraph;
	namespace DCEL {
		struct FEdge;
		struct FVertex {
			FVector2d Location{};
			FEdge* Edge{};
			bool bValid{true};
		};

		struct FFace {
			FEdge* Outer{};
			int FaceId{};
			bool bOpenFace{false};
			bool bValid{true};
		};

		struct FEdge {
			FVertex* Origin{};
			FEdge* Twin{};
			FEdge* Next{};
			FEdge* Prev{};
			FFace* LeftFace{};
			int32 Index{INDEX_NONE};
			double Angle{};
			bool bValid{true};
		};

		void TraverseFaceEdges(FEdge* StartEdge, TFunction<void(FEdge* InEdge)> Visit);
		void TraverseFaceEdges(const FEdge* StartEdge, TFunction<void(const FEdge* InEdge)> Visit);
		FORCEINLINE double GetEdgeLength(const FEdge* InEdge) {
			return (InEdge->Origin->Location - InEdge->Twin->Origin->Location).Size();
		}
		FORCEINLINE double GetEdgeLengthSq(const FEdge* InEdge) {
			return (InEdge->Origin->Location - InEdge->Twin->Origin->Location).SizeSquared();
		}
	};

	class DCELGraphQuery {
	public:
		bool ContainsVertex(const FVector2d& InLocation) const;
		DCEL::FVertex* GetVertex(const FVector2d& InLocation) const;

		void RegisterVertex(const FVector2d& InLocation, DCEL::FVertex* InVertex);
		void UnregisterVertex(const DCEL::FVertex *InVertex);

	private:
		struct FEdgeHash {
			uint32 operator()(const DCEL::FVertex* VertexStart, const DCEL::FVertex* VertexEnd) const;
		};

	private:
		TMap<FVector2d, DCEL::FVertex*> VertexTable;
	};

	class DCELGraph {
	public:
		~DCELGraph();
		void Generate(const TArray<TPair<FVector2d, FVector2d>>& InEdges);
		void Generate(const FVoronoiGraph& VGraph);
		void Clip(const FVector4f& InBounds);
		void Clear();
		void CloneFrom(const DA::DCELGraph& OtherGraph);

		const TArray<DCEL::FVertex*>& GetVertices() const { return Vertices; }
		const TArray<DCEL::FEdge*>& GetEdges() const { return Edges; }
		const TArray<DCEL::FFace*>& GetFaces() const { return Faces; }

		TArray<DCEL::FVertex*>& GetVertices() { return Vertices; }
		TArray<DCEL::FEdge*>& GetEdges() { return Edges; }
		TArray<DCEL::FFace*>& GetFaces() { return Faces; }

		const DCELGraphQuery& GetQuery() const { return Query; }

		DCEL::FFace* RegisterFace(DCEL::FEdge* InIncidentEdge, int InFaceId);
		DCEL::FEdge* CreateEdgePair();
		DCEL::FVertex* CreateNewVertex();

		DCEL::FVertex* FindOrAddVertex(const FVector2d &InLocation);
		DCEL::FVertex* SplitEdge(DCEL::FEdge* Edge, const FVector2d &InLocation);

		void AttachEdgeToVertex(DCEL::FEdge *InEdge, DCEL::FVertex *InVertex);
		void RemoveVertex(DCEL::FVertex* EdgeToDelete);

		void RelaxPoints(TArray<FVector2d>& InFacePoints) const;

	private:
		DCEL::FEdge* CreateNewEdge();
		void LinkPrevNext(DCEL::FEdge* A, DCEL::FEdge* B);

	private:
		TArray<DCEL::FVertex*> Vertices;
		TArray<DCEL::FEdge*> Edges;
		TArray<DCEL::FFace*> Faces;

		DCELGraphQuery Query;
	};


	class DCELGraphClipper {
	public:
		void ClipBoundary(DCELGraph& Graph, const FVector2d& Min, const FVector2d& Max);

	private:
		void ClipVertices(DCELGraph& Graph, const FVector2d& Min, const FVector2d& Max);
		void FixupFaces(DCELGraph& Graph);
		void SplitGraph(DCELGraph& Graph, const FVector2d& L0, const FVector2d& L1);

	private:
		TSet<DCEL::FEdge*> EdgesToIgnoreDuringSplit;
	};
}

