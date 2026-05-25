//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Lib/Geometry/DCEL.h"

#include "Core/Utils/MathUtils.h"
#include "Frameworks/Lib/Voronoi/VoronoiBeachLine.h"

namespace DA {
	
DCELGraph::~DCELGraph() {
	Clear();
}

void DCELGraph::Clear() {
	Query = {};

	for (const DCEL::FVertex* Vertex : Vertices) {
		delete Vertex;
	}
	Vertices.Reset();
	
	for (const DCEL::FEdge* Edge : Edges) {
		delete Edge;
	}
	Edges.Reset();

	for (const DCEL::FFace* Face : Faces) {
		delete Face;
	}
	Faces.Reset();
}

template<typename T>
class TArrayLookup {
public:
	TArrayLookup(const TArray<T*>& InArray) {
		for (int i = 0; i < InArray.Num(); i++) {
			if (InArray[i]) {
				int& IndexRef = IndexMap.FindOrAdd(InArray[i]);
				IndexRef = i;
			}
		}
	}

	bool GetIndex(T* Item, int& OutIndex) const {
		if (const int* IndexPtr = IndexMap.Find(Item)) {
			OutIndex = *IndexPtr;
			return true;
		}
		return false;
	}
	
	// Takes index from the lookup and returns the corresponding item from the specified array, null otherwise
	T* GetIndirect(const T* LookupItem, const TArray<T*>& IndirectArray) {
		if (const int* IndexPtr = IndexMap.Find(LookupItem)) {
			if (IndirectArray.IsValidIndex(*IndexPtr)) {
				return IndirectArray[*IndexPtr];
			}
		}
		return nullptr;
	}
	
private:
	TMap<T*, int> IndexMap;	
};
	
void DCELGraph::CloneFrom(const DA::DCELGraph& OtherGraph) {
	Clear();

	const TArray<DCEL::FVertex*>& OtherVertices = OtherGraph.GetVertices();
	const TArray<DCEL::FEdge*>& OtherEdges = OtherGraph.GetEdges();
	const TArray<DCEL::FFace*>& OtherFaces = OtherGraph.GetFaces();
	
	TArrayLookup<DCEL::FVertex> OtherVertexLookup(OtherVertices);
	TArrayLookup<DCEL::FEdge> OtherEdgeLookup(OtherEdges);
	TArrayLookup<DCEL::FFace> OtherFaceLookup(OtherFaces);

	// Allocate Memory
	Vertices.SetNum(OtherVertices.Num());
	for (int i = 0; i < Vertices.Num(); i++) {
		Vertices[i] = new DCEL::FVertex;
	}

	Edges.SetNum(OtherEdges.Num());
	for (int i = 0; i < Edges.Num(); i++) {
		Edges[i] = new DCEL::FEdge;
		Edges[i]->Index = i;
	}
	
	Faces.SetNum(OtherFaces.Num());
	for (int i = 0; i < Faces.Num(); i++) {
		Faces[i] = new DCEL::FFace;
	}

	auto IndirectEdge = [&](const DCEL::FEdge* OtherEdge) {
		return OtherEdgeLookup.GetIndirect(OtherEdge, Edges);
	};
	
	auto IndirectVertex = [&](const DCEL::FVertex* OtherVertex) {
		return OtherVertexLookup.GetIndirect(OtherVertex, Vertices);
	};
	
	auto IndirectFace = [&](const DCEL::FFace* OtherFace) {
		return OtherFaceLookup.GetIndirect(OtherFace, Faces);
	};
	
	// Initialize the vertices
	for (int i = 0; i < Vertices.Num(); i++) {
		DCEL::FVertex* Vertex = Vertices[i];
		const DCEL::FVertex* OtherVertex = OtherVertices[i];
		Vertex->Location = OtherVertex->Location;
		Vertex->Edge = IndirectEdge(OtherVertex->Edge);
		Vertex->bValid = OtherVertex->bValid;
	}

	// Initialize the faces
	for (int i = 0; i < Faces.Num(); i++) {
		DCEL::FFace* Face = Faces[i];
		const DCEL::FFace* OtherFace = OtherFaces[i];
		Face->Outer = IndirectEdge(OtherFace->Outer);
		Face->FaceId = OtherFace->FaceId;
		Face->bOpenFace = OtherFace->bOpenFace;
		Face->bValid = OtherFace->bValid;
	}

	// Initialize the edges
	for (int i = 0; i < Edges.Num(); i++) {
		DCEL::FEdge* Edge = Edges[i];
		const DCEL::FEdge* OtherEdge = OtherEdges[i];

		Edge->Origin = IndirectVertex(OtherEdge->Origin);
		Edge->Twin = IndirectEdge(OtherEdge->Twin);
		Edge->Next = IndirectEdge(OtherEdge->Next);
		Edge->Prev = IndirectEdge(OtherEdge->Prev);
		Edge->LeftFace = IndirectFace(OtherEdge->LeftFace);
		Edge->Angle = OtherEdge->Angle;
		Edge->bValid = OtherEdge->bValid;
	}

	// Update the Query
	for (int i = 0; i < Vertices.Num(); i++) {
		DCEL::FVertex* Vertex = Vertices[i];
		Query.RegisterVertex(Vertex->Location, Vertex);
	}
}


void DCELGraph::Generate(const FVoronoiGraph &VGraph) {
	Clear();

	const TVorDataPool<FVorVertex>& VVerts = VGraph.GetVertices();
	const TVorDataPool<FVorEdge>& VEdges = VGraph.GetEdges();
	if (VVerts.GetSize() == 0 || VEdges.GetSize() == 0) {
		return;
	}

	{
		//SCOPED_PROFILE(Init)
		Vertices.SetNum(VVerts.GetSize());
		Edges.SetNum(VEdges.GetSize());
		Faces.SetNum(VGraph.GetNumSites());

		int Index = 0;
		for (DCEL::FVertex*& Vert : Vertices) {
			Vert = new DCEL::FVertex;
			Vert->Location = VVerts.Get(Index)->Location;
			Vert->Edge = {};
			Index++;
			Query.RegisterVertex(Vert->Location, Vert);
		}

		Index = 0;
		for (DCEL::FEdge*& Edge : Edges) {
			Edge = new DCEL::FEdge;
			Edge->Index = Index;
			Index++;
		}
		
		Index = 0;
		for (DCEL::FFace*& Face : Faces) {
			Face = new DCEL::FFace;
			Face->FaceId = Index;
			Index++;
		}
		for (int i = 0; i < VEdges.GetSize(); i++) {
			const FVorEdge *VEdge = VEdges.Get(i);
			DCEL::FEdge* Edge = Edges[i];
			Edge->Origin = Vertices[VEdge->Origin->Index];
			Edge->Twin = Edges[VEdge->Twin->Index];
			Edge->Angle = VEdge->Angle;
			Edge->LeftFace = Faces[VEdge->AdjacentSite->SiteIndex];

			if (!Edge->Origin->Edge) {
				Edge->Origin->Edge = Edge;
			}

			if (!Edge->LeftFace->Outer) {
				Edge->LeftFace->Outer = Edge;
			}
		}
	}


	// Set the Edge Next / Prev for all the vertices
	{
		TMap<DCEL::FVertex*, TArray<DCEL::FEdge*>> OutgoingVertEdges;
		{
			//SCOPED_PROFILE(Gen Outgoing List)
			for (DCEL::FEdge* Edge: Edges) {
				TArray<DCEL::FEdge*>& OutgoingEdgeList = OutgoingVertEdges.FindOrAdd(Edge->Origin);
				OutgoingEdgeList.Add(Edge);
			}
		}

		{
			//SCOPED_PROFILE(Set Next / Prev)
			for (auto &Entry: OutgoingVertEdges) {
				DCEL::FVertex *Vertex = Entry.Key;
				TArray<DCEL::FEdge *> &OutgoingEdges = Entry.Value;
				if (OutgoingEdges.Num() > 0) {
					OutgoingEdges.Sort([](const DCEL::FEdge& A, const DCEL::FEdge& B) {
						return A.Angle < B.Angle;
					});
					
					Vertex->Edge = OutgoingEdges[0];
					const uint32 NumEdges = OutgoingEdges.Num();
					for (uint32 i = 0; i < NumEdges; i++) {
						DCEL::FEdge *Before = OutgoingEdges[(i - 1 + NumEdges) % NumEdges];
						DCEL::FEdge *Edge = OutgoingEdges[i];
						DCEL::FEdge *After = OutgoingEdges[(i + 1) % NumEdges];

						// AfterTwin <-> Edge
						// EdgeTwin <-> Before

						DCEL::FEdge *AfterTwin = After->Twin;
						AfterTwin->Next = Edge;
						Edge->Prev = AfterTwin;

						DCEL::FEdge *EdgeTwin = Edge->Twin;
						EdgeTwin->Next = Before;
						Before->Prev = EdgeTwin;
					}
				}
			}
		}
	}

	for (const DCEL::FEdge* Edge : Edges) {
		if (Edge->LeftFace->FaceId != Edge->Prev->LeftFace->FaceId) {
			Edge->LeftFace->bOpenFace = true;
		}
	}
}

void DCELGraph::Generate(const TArray<TPair<FVector2d, FVector2d>>& InEdges) {
	Clear();
	
	auto FnEdgeHash = [](DCEL::FVertex* VertexStart, DCEL::FVertex* VertexEnd) {
		return HashCombine(GetTypeHash(VertexStart->Location), GetTypeHash(VertexEnd->Location));
	};

	auto FnUpdateAngle = [](DCEL::FEdge *Edge) {
		Edge->Angle = FDAMathUtils::FindAngle(Edge->Twin->Origin->Location - Edge->Origin->Location);
	};

	// Create the edges and their twins
	TSet<uint32> CreatedEdges;
	for (const auto &Entry: InEdges) {
		FVector2d LocStart = Entry.Key;
		FVector2d LocEnd = Entry.Value;

		DCEL::FVertex *VertexStart = FindOrAddVertex(LocStart);
		DCEL::FVertex *VertexEnd = FindOrAddVertex(LocEnd);

		// Don't create duplicate edges
		{
			const uint32 EdgeHash = FnEdgeHash(VertexStart, VertexEnd);
			const uint32 TwinEdgeHash = FnEdgeHash(VertexEnd, VertexStart);
			if (CreatedEdges.Contains(EdgeHash) || CreatedEdges.Contains(TwinEdgeHash)) {
				continue;
			}
			CreatedEdges.Add(EdgeHash);
			CreatedEdges.Add(TwinEdgeHash);
		}

		DCEL::FEdge* Edge = CreateEdgePair();
		DCEL::FEdge* TwinEdge = Edge->Twin;

		Edge->Twin = TwinEdge;
		TwinEdge->Twin = Edge;

		Edge->Origin = VertexStart;
		TwinEdge->Origin = VertexEnd;

		FnUpdateAngle(Edge);
		FnUpdateAngle(TwinEdge);
	}

	// Reference the Next/Prev of the edges
	TMap<DCEL::FVertex*, TArray<DCEL::FEdge*>> OutgoingVertEdges;
	for (DCEL::FEdge* Edge: Edges) {
		TArray<DCEL::FEdge*>& EdgeList = OutgoingVertEdges.FindOrAdd(Edge->Origin); 
		EdgeList.Add(Edge);
	}

	for (auto &Entry: OutgoingVertEdges) {
		DCEL::FVertex* Vertex = Entry.Key;
		TArray<DCEL::FEdge*>& OutgoingEdges = Entry.Value;
		if (OutgoingEdges.Num() > 0) {
			OutgoingEdges.Sort([](const DCEL::FEdge& A, const DCEL::FEdge& B) -> bool {
				return A.Angle < B.Angle;
			});
			
			Vertex->Edge = OutgoingEdges[0];
			const int32 NumEdges = OutgoingEdges.Num();
			for (int32 i = 0; i < NumEdges; i++) {
				DCEL::FEdge *Before = OutgoingEdges[(i - 1 + NumEdges) % NumEdges];
				DCEL::FEdge *Edge = OutgoingEdges[i];
				DCEL::FEdge *After = OutgoingEdges[(i + 1) % NumEdges];

				// AfterTwin <-> Edge
				// EdgeTwin <-> Before

				DCEL::FEdge *AfterTwin = After->Twin;
				AfterTwin->Next = Edge;
				Edge->Prev = AfterTwin;

				DCEL::FEdge *EdgeTwin = Edge->Twin;
				EdgeTwin->Next = Before;
				Before->Prev = EdgeTwin;
			}
		}
	}
}

DCEL::FFace* DCELGraph::RegisterFace(DCEL::FEdge* InIncidentEdge, int InFaceId) {
	DCEL::FFace* Face = new DCEL::FFace;
	Face->Outer = InIncidentEdge;
	Face->FaceId = InFaceId;
	Faces.Add(Face);

	return Face;
}

void DCELGraph::Clip(const FVector4f &InBounds) {

}

DCEL::FVertex *DCELGraph::FindOrAddVertex(const FVector2d &InLocation) {
	if (!Query.ContainsVertex(InLocation)) {
		DCEL::FVertex *Vert = CreateNewVertex();
		Vert->Location = InLocation;
		Query.RegisterVertex(InLocation, Vert);
		return Vert;
	}
	return Query.GetVertex(InLocation);
}

DCEL::FVertex* DCELGraph::SplitEdge(DCEL::FEdge *Edge, const FVector2d &InLocation) {

	DCEL::FVertex* NewVertex = FindOrAddVertex(InLocation);

	DCEL::FEdge* EdgeTwin = Edge->Twin;

	//DATest::TestIntegrity(*this);
	DCEL::FEdge* NewEdge = CreateEdgePair();
	DCEL::FEdge* NewEdgeTwin = NewEdge->Twin;

	NewEdge->LeftFace = Edge->LeftFace;
	NewEdgeTwin->LeftFace = EdgeTwin->LeftFace;

	NewEdgeTwin->Origin = EdgeTwin->Origin;
	EdgeTwin->Origin = NewVertex;
	NewEdge->Origin = NewVertex;

	NewVertex->Edge = NewEdge;
	if (NewEdgeTwin->Origin->Edge == EdgeTwin) {
		NewEdgeTwin->Origin->Edge = NewEdgeTwin;
	}
	//DATest::TestIntegrity(*this);

	// Fix Prev/Next of NewEdge
	DCEL::FEdge* OrigEdgeNext = Edge->Next;
	DCEL::FEdge* OrigEdgeTwinPrev = EdgeTwin->Prev;

	if (Edge->Next != Edge->Twin) {
		LinkPrevNext(Edge, NewEdge);
		LinkPrevNext(NewEdge, OrigEdgeNext);

		LinkPrevNext(OrigEdgeTwinPrev, NewEdgeTwin);
		LinkPrevNext(NewEdgeTwin, EdgeTwin);
	}
	else {
		LinkPrevNext(Edge, NewEdge);
		LinkPrevNext(NewEdge, NewEdgeTwin);
		LinkPrevNext(NewEdgeTwin, EdgeTwin);
	}

	NewEdge->Angle = Edge->Angle;
	NewEdgeTwin->Angle = EdgeTwin->Angle;


	return NewVertex;
}

DCEL::FEdge *DCELGraph::CreateEdgePair() {
	DCEL::FEdge* Edge = CreateNewEdge();
	DCEL::FEdge* EdgeTwin = CreateNewEdge();
	Edge->Twin = EdgeTwin;
	EdgeTwin->Twin = Edge;
	Edge->Next = Edge->Prev = EdgeTwin;
	EdgeTwin->Next = EdgeTwin->Prev = Edge;

	return Edge;
}

void DCELGraph::AttachEdgeToVertex(DCEL::FEdge* InEdge, DCEL::FVertex* InVertex) {
	// Set the edge origin to the vertex to attach to
	InEdge->Origin = InVertex;

	// Update the Angle of the edge and its twin
	if (InEdge->Twin->Origin) {
		FVector2d Dir = (InEdge->Twin->Origin->Location - InEdge->Origin->Location).GetSafeNormal();
		InEdge->Angle = FDAMathUtils::FindAngle(Dir);
		InEdge->Twin->Angle = InEdge->Angle + PI;

		constexpr double PI2 = PI * 2;
		if (InEdge->Twin->Angle >= PI2) {
			InEdge->Twin->Angle -= PI2;
		}
	}

	if (!InVertex->Edge) {
		InVertex->Edge = InEdge;
		return;
	}

	// Insert the edge in the vertex list correctly (update the next/prev correctly)
	{
		TArray<DCEL::FEdge*> VertEdges;
		DCEL::FEdge *VertEdge = InVertex->Edge;
		do {
			VertEdges.Add(VertEdge);
			VertEdge = VertEdge->Twin->Next;
		} while (VertEdge != InVertex->Edge);

		// TODO: Optimize Me.  The list is already sorted, but not rotated correctly.  Rotate instead of sort by staring from the lowest angle
		VertEdges.Sort([](const DCEL::FEdge& A, const DCEL::FEdge& B) {
			return A.Angle < B.Angle;
		});

		int InsertIdx;
		for (InsertIdx = 0; InsertIdx < VertEdges.Num(); InsertIdx++) {
			DCEL::FEdge* E = VertEdges[InsertIdx];
			if (E->Angle >= InEdge->Angle) {
				break;
			}
		}
		InsertIdx = (InsertIdx - 1 + VertEdges.Num()) % VertEdges.Num();

		DCEL::FEdge* EdgeNext = VertEdges[InsertIdx];
		DCEL::FEdge* EdgePrev = EdgeNext->Prev;   // Enters the new vertex, not the same as the vertex's previous edge (that would be the twin of this)

		LinkPrevNext(EdgePrev, InEdge);
		LinkPrevNext(InEdge->Twin, EdgeNext);
	}

	/*
	if (!InEdge->LeftFace) {
		InEdge->LeftFace = InEdge->Prev->LeftFace
				? InEdge->Prev->LeftFace
				: InEdge->Next->LeftFace;
	}
	 */
}

void DCELGraph::LinkPrevNext(DCEL::FEdge *A, DCEL::FEdge *B) {
	A->Next = B;
	B->Prev = A;
}

namespace {
	template<typename T>
	void RemoveItem(TArray<T*>& List, T*& Item) {
		Item->bValid = false;
		//List.RemoveSingleSwap(Item);
		//delete Item;
		//Item = {};
	}
}

void DCELGraph::RemoveVertex(DCEL::FVertex* InVertex) {
	if (!InVertex) {
		return;
	}

	TArray<DCEL::FEdge*> EdgesToRemove;
	if (InVertex->Edge) {
		DCEL::FEdge* Edge = InVertex->Edge;
		do {
			EdgesToRemove.Add(Edge);
			Edge = Edge->Twin->Next;
		} while (Edge != InVertex->Edge);
	}

	auto ChangeOriginVertexRef = [](DCEL::FEdge* Edge) {
		if (Edge->Origin && Edge->Origin->Edge == Edge) {
			Edge->Origin->Edge = Edge->Twin->Next;
			if (Edge->Origin->Edge == Edge) {
				Edge->Origin->Edge = {};
			}
		}
	};

	auto ChangeFaceRef = [](DCEL::FEdge* EdgeToDelete, DCEL::FEdge* NewRef) {
		if (EdgeToDelete->LeftFace && EdgeToDelete->LeftFace->Outer == EdgeToDelete) {
			EdgeToDelete->LeftFace->Outer = NewRef;
			if (EdgeToDelete->LeftFace->Outer == EdgeToDelete || EdgeToDelete->LeftFace->Outer == EdgeToDelete->Twin) {
				EdgeToDelete->LeftFace->Outer = {};
			}
		}
	};


	for (DCEL::FEdge* EdgeToRemove : EdgesToRemove) {
		// Unlink from the other end
		DCEL::FEdge* Edge = EdgeToRemove->Twin;
		DCEL::FEdge* Prev = Edge->Prev;
		DCEL::FEdge* Next = EdgeToRemove->Next;
		LinkPrevNext(Prev, Next);

		ChangeFaceRef(EdgeToRemove, EdgeToRemove->Next);
		ChangeFaceRef(EdgeToRemove->Twin, EdgeToRemove->Twin->Prev);
		ChangeOriginVertexRef(Edge);

		// Delete the vertex
		RemoveItem(Edges, EdgeToRemove);
		RemoveItem(Edges, Edge);
	}

	// Delete the vertex
	Query.UnregisterVertex(InVertex);
	RemoveItem(Vertices, InVertex);
}

void DCELGraph::RelaxPoints(TArray<FVector2d>& InFacePoints) const {
	for (DCEL::FFace* Face : Faces) {
		if (!Face || !Face->Outer || Face->FaceId >= InFacePoints.Num()) {
			continue;
		}

		FVector2d Sum = InFacePoints[Face->FaceId];
		int Count = 1;
		DCEL::TraverseFaceEdges(Face->Outer, [&Sum, &Count](const DCEL::FEdge* Edge) {
			Sum += Edge->Origin->Location;
			Count++;
		});
		const FVector2d RelaxedSiteLocation = Sum / Count;
		InFacePoints[Face->FaceId] = RelaxedSiteLocation;
	}
}

DCEL::FEdge* DCELGraph::CreateNewEdge() {
	DCEL::FEdge* Edge = new DCEL::FEdge;
	const int Index = Edges.Add(Edge);
	Edge->Index = Index;
	return Edge;
}

DCEL::FVertex* DCELGraph::CreateNewVertex() {
	DCEL::FVertex* Vertex = new DCEL::FVertex;
	Vertices.Add(Vertex);
	return Vertex;
}

DCEL::FVertex* DCELGraphQuery::GetVertex(const FVector2d& InLocation) const {
	DCEL::FVertex* const* SearchResult = VertexTable.Find(InLocation);
	return SearchResult ? *SearchResult : nullptr;
}

void DCEL::TraverseFaceEdges(FEdge* StartEdge, TFunction<void(FEdge* InEdge)> Visit) {
	if (!StartEdge) return;
	FEdge* TraverseEdge = StartEdge;
	TSet<FEdge*> Visited;
	
	do {
		Visit(TraverseEdge);
		Visited.Add(TraverseEdge);
		
		TraverseEdge = TraverseEdge->Next;
		if (Visited.Contains(TraverseEdge)) {
			break;
		}
	} while (TraverseEdge != StartEdge);
}

void DCEL::TraverseFaceEdges(const FEdge* StartEdge, TFunction<void(const FEdge* InEdge)> Visit) {
	if (!StartEdge) return;
	const FEdge* TraverseEdge = StartEdge;
	TSet<const FEdge*> Visited;
	
	do {
		Visit(TraverseEdge);
		Visited.Add(TraverseEdge);
		
		TraverseEdge = TraverseEdge->Next;
		if (Visited.Contains(TraverseEdge)) {
			break;
		}
	} while (TraverseEdge != StartEdge);
}

bool DCELGraphQuery::ContainsVertex(const FVector2d& InLocation) const {
	return VertexTable.Contains(InLocation);
}

void DCELGraphQuery::RegisterVertex(const FVector2d &InLocation, DCEL::FVertex* InVertex) {
	DCEL::FVertex*& VertexRef = VertexTable.FindOrAdd(InLocation);
	VertexRef = InVertex;
}

void DCELGraphQuery::UnregisterVertex(const DCEL::FVertex *InVertex) {
	VertexTable.Remove(InVertex->Location);
}

void DCELGraphClipper::ClipBoundary(DCELGraph &Graph, const FVector2d &Min, const FVector2d &Max) {
	EdgesToIgnoreDuringSplit.Reset();

	{
		const FVector2d BottomRight = { Max.X, Min.Y };
		const FVector2d TopLeft = { Min.X, Max.Y };
		
		SplitGraph(Graph, Min, BottomRight);
		SplitGraph(Graph, BottomRight, Max);
		SplitGraph(Graph, Max, TopLeft);
		SplitGraph(Graph, TopLeft, Min);
	}
	
	ClipVertices(Graph, Min, Max);
	FixupFaces(Graph);
}

void DCELGraphClipper::ClipVertices(DCELGraph &Graph, const FVector2d &Min, const FVector2d &Max) {
	auto InsideBounds = [&Min, &Max](const FVector2d& P) {
		constexpr double EPSILON = 1e-7;

		return P.X >= Min.X - EPSILON
		       && P.X <= Max.X + EPSILON
		       && P.Y >= Min.Y - EPSILON
		       && P.Y <= Max.Y + EPSILON;
	};

	TArray<DCEL::FVertex*> VerticesToRemove;
	for (DCEL::FVertex* Vertex: Graph.GetVertices()) {
		if (!Vertex->bValid) continue;
		if (!InsideBounds(Vertex->Location)) {
			VerticesToRemove.Add(Vertex);
		}
	}

	{
		//SCOPED_PROFILE(CV - Remove Verts);
		for (DCEL::FVertex *VertexToRemove: VerticesToRemove) {
			Graph.RemoveVertex(VertexToRemove);
		}
	}
}

void DCELGraphClipper::FixupFaces(DCELGraph &Graph) {
	const TArray<DCEL::FFace*>& Faces = Graph.GetFaces();
	for (DCEL::FEdge* Edge : Graph.GetEdges()) {
		if (!Edge->bValid) continue;
		if (Edge->LeftFace) {
			Edge->LeftFace->Outer = Edge;
		}
	}

	TSet<DCEL::FEdge*> Visited;
	for (DCEL::FEdge* Edge : Graph.GetEdges()) {
		if (!Edge->bValid) continue;
		if (Visited.Contains(Edge)) {
			continue;
		}
		Visited.Add(Edge);
		if (Edge->LeftFace) {
			DCEL::FEdge* StartEdge = Edge;
			DCEL::FEdge* TraverseEdge = StartEdge->Next;
			while (TraverseEdge != StartEdge) {
				if (!TraverseEdge->LeftFace) {
					TraverseEdge->LeftFace = StartEdge->LeftFace;
				}
				Visited.Add(TraverseEdge);
				TraverseEdge = TraverseEdge->Next;
			}
		}
	}
}

void DCELGraphClipper::SplitGraph(DCELGraph &Graph, const FVector2d &L0, const FVector2d &L1) {
	auto ConnectVertices = [&Graph](DCEL::FVertex* PrevVert, DCEL::FVertex* NextVert) {
		DCEL::FEdge* NewEdge = Graph.CreateEdgePair();
		NewEdge->Origin = PrevVert;
		NewEdge->Twin->Origin = NextVert;
		Graph.AttachEdgeToVertex(NewEdge, PrevVert);
		Graph.AttachEdgeToVertex(NewEdge->Twin, NextVert);
		return NewEdge;
	};

	struct IntersectionInfo {
		DCEL::FEdge* Edge{};
		double SrcT{};
		double EdgeT{};
	};


	TArray<IntersectionInfo> EdgeIntersects;
	TSet<DCEL::FEdge*> Visited;
	for (DCEL::FEdge* Edge: Graph.GetEdges()) {
		if (!Edge->bValid) continue;
		if (Visited.Contains(Edge) || EdgesToIgnoreDuringSplit.Contains(Edge)) {
			continue;
		}

		//Visited.insert(&Edge);
		Visited.Add(Edge->Twin);

		const FVector2d& A0 = L0;
		const FVector2d& A1 = L1;

		const FVector2d& B0 = Edge->Origin->Location;
		const FVector2d& B1 = Edge->Twin->Origin->Location;

		double TA{}, TB{};
		if (FDAMathUtils::RayRayIntersection(A0, A1 - A0, B0, B1 - B0, TA, TB)) {
			IntersectionInfo Intersection{ Edge, TA, TB };
			EdgeIntersects.Add(MoveTemp(Intersection));
		}
	}

	EdgeIntersects.Sort([](const IntersectionInfo& A, const IntersectionInfo& B) {
		return A.SrcT < B.SrcT;
	});

	TArray<DCEL::FEdge*> BoundaryEdges;
	auto HandleConnectVerts = [this, &BoundaryEdges, &ConnectVertices](DCEL::FVertex* PrevVert, DCEL::FVertex* NextVert) {
		DCEL::FEdge* NewEdge = ConnectVertices(PrevVert, NextVert);
		EdgesToIgnoreDuringSplit.Add(NewEdge);
		EdgesToIgnoreDuringSplit.Add(NewEdge->Twin);
		BoundaryEdges.Add(NewEdge);
		NewEdge->Twin->LeftFace = {};
		NewEdge->Twin->Prev->LeftFace = {};
	};


	DCEL::FVertex* PrevVert = Graph.FindOrAddVertex(L0);
	for (const IntersectionInfo& Intersection : EdgeIntersects) {
		const FVector2d& Start = Intersection.Edge->Origin->Location;
		const FVector2d& End = Intersection.Edge->Twin->Origin->Location;
		FVector2d IntersectionPoint = Start + (End - Start) * Intersection.EdgeT;
		DCEL::FVertex* NewVertex = Graph.SplitEdge(Intersection.Edge, IntersectionPoint);
		HandleConnectVerts(PrevVert, NewVertex);
		PrevVert = NewVertex;
	}

	DCEL::FVertex* LastVert = Graph.FindOrAddVertex(L1);
	HandleConnectVerts(PrevVert, LastVert);
}


//////////// Hashing code ////////////

uint32 DCELGraphQuery::FEdgeHash::operator()(const DCEL::FVertex* VertexStart, const DCEL::FVertex* VertexEnd) const {
	return HashCombine(GetTypeHash(VertexStart->Location), GetTypeHash(VertexEnd->Location));
}

	
}

