//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Lib/Voronoi/VoronoiBeachLine.h"

#include "Core/Utils/MathUtils.h"

namespace DA {

bool operator<(const FVoronoiEvent &A, const FVoronoiEvent &B) {
	const FVector2d& LocationA = A.bSiteEvent ? A.SiteEvent.Site->Location : A.CircleEvent.Location;
	const FVector2d& LocationB = B.bSiteEvent ? B.SiteEvent.Site->Location : B.CircleEvent.Location;

	if (LocationA.Y == LocationB.Y) {
		return LocationA.X < LocationB.X;
	}

	return LocationA.Y > LocationB.Y;
}

bool operator<(const FVoronoiEventPtr &A, const FVoronoiEventPtr &B) {
	return *A < *B;
}

FVoronoiEventPtr FVoronoiEvent::CreateCircleEvent(const FVector2d &InLocation, const FVector2d &InCircumCenter, FVoronoiBeachLine::FNode* InDisappearingArc) {
	check(InDisappearingArc);

	FVoronoiEventPtr Event = MakeShared<FVoronoiEvent>();
	memset(Event.Get(), 0, sizeof(Event));
	Event->bSiteEvent = false;
	Event->CircleEvent.Location = InLocation;
	Event->CircleEvent.CircumCenter = InCircumCenter;
	Event->CircleEvent.DisappearingArc = InDisappearingArc;
	Event->CircleEvent._PreviewDisappearingArcSiteIndex = InDisappearingArc->LeafData.SitePoint->SiteIndex;

	InDisappearingArc->LeafData.CircleEvent = &Event->CircleEvent;

	return Event;
}

FVoronoiEventPtr FVoronoiEvent::CreateSiteEvent(const FVorSite *InSite) {
	FVoronoiEventPtr Event = MakeShared<FVoronoiEvent>();
	memset(Event.Get(), 0, sizeof(Event));
	Event->bSiteEvent = true;
	Event->SiteEvent.Site = InSite;
	return Event;
}

FVector2d FVoronoiEvent::GetLocation() const {
	return bSiteEvent ? SiteEvent.Site->Location : CircleEvent.Location;
}


FString FVoronoiBeachLineExtension::GetNodeString(FVoronoiBeachLine::FNode* InNode) const {
	if (InNode->bLeafNode && InNode->LeafData.SitePoint) {
		return FString::FromInt(InNode->LeafData.SitePoint->SiteIndex);
	}
	else if (!InNode->bLeafNode && InNode->BreakpointData.SitePointLeft && InNode->BreakpointData.SitePointRight) {
		const FString Left = FString::FromInt(InNode->BreakpointData.SitePointLeft->SiteIndex);
		const FString Right = FString::FromInt(InNode->BreakpointData.SitePointRight->SiteIndex);
		return "<" + Left + "," + Right + ">";
	}
	else {
		return "Nil";
	}
}

void FVoronoiBeachLineExtension::GetCustomLinks(const FVoronoiBeachLine& InBeachLine, TArray<TPair<typename FVoronoiBeachLine::FNode*, typename FVoronoiBeachLine::FNode*>>& CustomList) const {
	FVoronoiBeachLine::FNode* MinNode = InBeachLine.GetMin(InBeachLine.GetRoot());
	FVoronoiBeachLine::FNode* Node = MinNode;
	while (!InBeachLine.IsNil(Node) && !InBeachLine.IsNil(Node->Next)) {
		CustomList.Add({Node, Node->Next});
		Node = Node->Next;
	}
}

void FVoronoiBeachLine::RemoveArc(FVoronoiBeachLine::FNode *NodeToRemove, FVoronoiGraph &Graph) {
	if (IsNil(NodeToRemove)) {
		return;
	}
	check(NodeToRemove->bLeafNode && NodeToRemove->LeafData.CircleEvent);
	check(!IsNil(NodeToRemove->Parent));

	FVector2d VertexLocation = NodeToRemove->LeafData.CircleEvent->CircumCenter;
	const FVorVertex* NewVertex = Graph.CreateVertex(VertexLocation);

	FNode* Sibling{};

	if (NodeToRemove->Parent->Left == NodeToRemove) {
		// Node is Left of the parent
		Sibling = NodeToRemove->Next;
		Sibling->Prev = NodeToRemove->Prev;
	} else {
		// Node is right of the parent
		Sibling = NodeToRemove->Prev;
		Sibling->Next = NodeToRemove->Next;
	}

	LinkPrevNext(NodeToRemove->Prev, NodeToRemove->Next);

	// #1 Attach the direct parent to the vertex
	NodeToRemove->Parent->BreakpointData.Edge->Origin = NewVertex;

	const FVorSite* SiblingSite = Sibling->LeafData.SitePoint;

	FNode* Node = NodeToRemove->Parent;
	check(!IsNil(Node->Parent));

	FVorEdge* EdgeLeft{};
	FVorEdge* EdgeRight{};
	Graph.CreateEdgePair(EdgeLeft, EdgeRight);

	FNode* ParentToFix{};
	if (NodeToRemove->Parent->Left == NodeToRemove) {
		ParentToFix = GetPredecessor(NodeToRemove);
		ParentToFix->BreakpointData.Edge->Origin = NewVertex;

		ParentToFix->BreakpointData.SitePointRight = SiblingSite;
		ParentToFix->BreakpointData.Edge = EdgeLeft;
		EdgeRight->Origin = NewVertex;

		EdgeLeft->AdjacentSite = ParentToFix->BreakpointData.SitePointLeft;
		EdgeRight->AdjacentSite = ParentToFix->BreakpointData.SitePointRight;
	}
	else {
		ParentToFix = GetSuccessor(NodeToRemove);
		ParentToFix->BreakpointData.Edge->Origin = NewVertex;

		ParentToFix->BreakpointData.SitePointLeft = SiblingSite;
		ParentToFix->BreakpointData.Edge = EdgeRight;
		EdgeLeft->Origin = NewVertex;

		EdgeLeft->AdjacentSite = ParentToFix->BreakpointData.SitePointRight;
		EdgeRight->AdjacentSite = ParentToFix->BreakpointData.SitePointLeft;
	}

	EdgeLeft->UpdateAngle();

	// Get rid of the node and it's parent.  the sibling will take the parent's place
	{
		FNode* ParentToRemove = NodeToRemove->Parent;
		DeleteImpl(NodeToRemove);
		delete NodeToRemove;

		DeleteImpl(ParentToRemove);
		delete ParentToRemove;
		ParentToRemove = {};
	}
}

void FVoronoiBeachLine::Remove(FNode* Node) {
	if (!IsNil(Node)) {
		LinkPrevNext(Node->Prev, Node->Next);
		DeleteImpl(Node);
		delete Node;
		Node = &NilNode;
	}
}

FVoronoiBeachLine::FNode* FVoronoiBeachLine::GetMin(FNode* InNode) const {
	if (!IsNil(InNode)) {
		while (!IsNil(InNode->Left)) {
			InNode = InNode->Left;
		}
	}
	return InNode;
}

FVoronoiBeachLine::FNode* FVoronoiBeachLine::GetMax(FNode* InNode) const {
	while (!IsNil(InNode->Right)) {
		InNode = InNode->Right;
	}
	return InNode;
}

FVoronoiBeachLine::FNode* FVoronoiBeachLine::GetSuccessor(FNode* Node) const {
	if (!IsNil(Node->Right)) {
		return GetMin(Node->Right);
	}

	FNode* Parent = Node->Parent;
	while (!IsNil(Parent) && Node == Parent->Right) {
		Node = Parent;
		Parent = Node->Parent;
	}
	return Parent;
}

FVoronoiBeachLine::FNode* FVoronoiBeachLine::GetPredecessor(FNode* Node) const {
	if (!IsNil(Node->Left)) {
		return GetMax(Node->Left);
	}

	FNode* Parent = Node->Parent;
	while (!IsNil(Parent) && Node == Parent->Left) {
		Node = Parent;
		Parent = Node->Parent;
	}
	return Parent;
}

void FVoronoiBeachLine::Traverse(const FnTraverseVisit& InVisitFn) const {
	TraverseSimple(RootNode, InVisitFn);
}

void FVoronoiBeachLine::Traverse(const FnTraverseStatVisit& InVisitFn) const {
	TraverseWithStat(RootNode, {}, InVisitFn);
}

void FVoronoiBeachLine::LeftRotate(FNode* x) {
	FNode* y = x->Right;
	x->Right = y->Left;
	if (!IsNil(y->Left)) {
		y->Left->Parent = x;
	}
	y->Parent = x->Parent;
	if (IsNil(x->Parent)) {
		RootNode = y;
	}
	else if (x == x->Parent->Left) {
		x->Parent->Left = y;
	}
	else {
		x->Parent->Right = y;
	}
	y->Left = x;
	x->Parent = y;
}

void FVoronoiBeachLine::RightRotate(FNode* x) {
	FNode* y = x->Left;
	x->Left = y->Right;
	if (!IsNil(y->Right)) {
		y->Right->Parent = x;
	}
	y->Parent = x->Parent;
	if (IsNil(x->Parent)) {
		RootNode = y;
	}
	else if (x == x->Parent->Right) {
		x->Parent->Right = y;
	}
	else {
		x->Parent->Left = y;
	}
	y->Right = x;
	x->Parent = y;
}

void FVoronoiBeachLine::RBInsertFixup(FNode* z) {
	while (z->Parent->Color == ENodeColor::Red) {
		if (z->Parent == z->Parent->Parent->Left) {
			FNode* y = z->Parent->Parent->Right;     // Uncle node
			if (y->Color == ENodeColor::Red) {
				z->Parent->Color = ENodeColor::Black;
				y->Color = ENodeColor::Black;
				z->Parent->Parent->Color = ENodeColor::Red;
				z = z->Parent->Parent;
			}
			else {
				if (z == z->Parent->Right) {
					z = z->Parent;
					LeftRotate(z);
				}
				z->Parent->Color = ENodeColor::Black;
				z->Parent->Parent->Color = ENodeColor::Red;
				RightRotate(z->Parent->Parent);
			}
		}
		else {
			// Mirror the code
			FNode* y = z->Parent->Parent->Left;     // Uncle node
			if (y->Color == ENodeColor::Red) {
				z->Parent->Color = ENodeColor::Black;
				y->Color = ENodeColor::Black;
				z->Parent->Parent->Color = ENodeColor::Red;
				z = z->Parent->Parent;
			}
			else {
				if (z == z->Parent->Left) {
					z = z->Parent;
					RightRotate(z);
				}
				z->Parent->Color = ENodeColor::Black;
				z->Parent->Parent->Color = ENodeColor::Red;
				LeftRotate(z->Parent->Parent);
			}
		}
	}
	RootNode->Color = ENodeColor::Black;
}

void FVoronoiBeachLine::Transplant(FNode* u, FNode* v) {
	if (IsNil(u->Parent)) {
		RootNode = v;
	}
	else if (u == u->Parent->Left) {
		u->Parent->Left = v;
	}
	else {
		u->Parent->Right = v;
	}

	v->Parent = u->Parent;
}

void FVoronoiBeachLine::DeleteImpl(FNode* z) {
	FNode* x = &NilNode;
	FNode* y = z;
	ENodeColor YOrigColor = y->Color;

	if (IsNil(z->Left)) {
		x = z->Right;
		Transplant(z, z->Right);
	}
	else if (IsNil(z->Right)) {
		x = z->Left;
		Transplant(z, z->Left);
	}
	else {
		y = GetMin(z->Right);
		YOrigColor = y->Color;
		x = y->Right;
		if (y->Parent == z) {
			x->Parent = y;
		}
		else {
			Transplant(y, y->Right);
			y->Right = z->Right;
			y->Right->Parent = y;
		}

		Transplant(z, y);
		y->Left = z->Left;
		y->Left->Parent = y;
		y->Color = z->Color;
	}

	if (YOrigColor == ENodeColor::Black) {
		RBDeleteFixup(x);
	}
}

void FVoronoiBeachLine::RBDeleteFixup(FNode* x) {
	FNode* w{};
	while (x != RootNode && x->Color == ENodeColor::Black) {
		if (x == x->Parent->Left) {
			w = x->Parent->Right;
			if (w->Color == ENodeColor::Red) {
				w->Color = ENodeColor::Black;
				x->Parent->Color = ENodeColor::Red;
				LeftRotate(x->Parent);
				w = x->Parent->Right;
			}
			else if (w->Left->Color == ENodeColor::Black && w->Right->Color == ENodeColor::Black) {
				w->Color = ENodeColor::Red;
				x = x->Parent;
			}
			else if (w->Right->Color == ENodeColor::Black) {
				w->Left->Color = ENodeColor::Black;
				w->Color = ENodeColor::Red;
				RightRotate(w);
				w = x->Parent->Right;
			}
			else {
				w->Color = x->Parent->Color;
				x->Parent->Color = ENodeColor::Black;
				w->Right->Color = ENodeColor::Black;
				LeftRotate(x->Parent);
				x = RootNode;
			}
		}
		else {
			// Mirror the code
			w = x->Parent->Left;
			if (w->Color == ENodeColor::Red) {
				w->Color = ENodeColor::Black;
				x->Parent->Color = ENodeColor::Red;
				RightRotate(x->Parent);
				w = x->Parent->Left;
			}
			else if (w->Right->Color == ENodeColor::Black && w->Left->Color == ENodeColor::Black) {
				w->Color = ENodeColor::Red;
				x = x->Parent;
			}
			else if (w->Left->Color == ENodeColor::Black) {
				w->Right->Color = ENodeColor::Black;
				w->Color = ENodeColor::Red;
				LeftRotate(w);
				w = x->Parent->Left;
			}
			else {
				w->Color = x->Parent->Color;
				x->Parent->Color = ENodeColor::Black;
				w->Left->Color = ENodeColor::Black;
				RightRotate(x->Parent);
				x = RootNode;
			}
		}
	}
	x->Color = ENodeColor::Black;
}

void FVoronoiBeachLine::TraverseSimple(FNode* InNode, const FnTraverseVisit& InVisitFn) const {
	if (!IsNil(InNode)) {
		TraverseSimple(InNode->Left, InVisitFn);
		InVisitFn(InNode);
		TraverseSimple(InNode->Right, InVisitFn);
	}
}

void FVoronoiBeachLine::TraverseWithStat(FNode* InNode, const FTraverseStat& InStat, const FnTraverseStatVisit& InVisitFn) const {
	if (IsNil(InNode)) {
		return;
	}

	FTraverseStat Stat = InStat;
	FTraverseStat StatLeft{ InStat.Depth + 1, InStat.PreviewPos };
	FTraverseStat StatRight{ InStat.Depth + 1, InStat.PreviewPos };

	if (InStat.Depth == 0) {
		Stat.ParentPreviewPos = Stat.PreviewPos;
	}
	StatLeft.PreviewPos = {
		InStat.PreviewPos.X * 2,
		static_cast<double>(InStat.Depth + 1)
	};
	StatRight.PreviewPos = {
		InStat.PreviewPos.X * 2 + 1,
		static_cast<double>(InStat.Depth + 1)
	};

	TraverseWithStat(InNode->Left, StatLeft, InVisitFn);
	InVisitFn(InNode, InStat);
	TraverseWithStat(InNode->Right, StatRight, InVisitFn);
}

FVoronoiBeachLine::FNode *FVoronoiBeachLine::SplitArc(FVoronoiBeachLine::FNode *x, const FVorSite *NewArc, FVoronoiGraph &Graph) {
	if (IsNil(x)) {
		FNode* NewNode = CreateNewNode();
		NewNode->SetLeafData(NewArc, {});
		RootNode = NewNode;
		RBInsertFixup(NewNode);
		return NewNode;
	}

	check(x->bLeafNode);
	check(IsNil(x->Left) && IsNil(x->Right));

	const FVorSite* TopArc = x->LeafData.SitePoint;

	FNode* OrigPrev = x->Prev;
	FNode* OrigNext = x->Next;
	x->Prev = &NilNode;
	x->Next = &NilNode;

	FNode* LeafLeft = CreateNewNode();
	LeafLeft->SetLeafData(TopArc, {});

	FNode* LeafMid = CreateNewNode();
	LeafMid->SetLeafData(NewArc, {});

	FNode* LeafRight = CreateNewNode();
	LeafRight->SetLeafData(TopArc, {});

	FNode* InterLeftMid = x;
	FNode* InterMidRight = CreateNewNode();

	InterLeftMid->SetBreakpointData(TopArc, NewArc);
	InterMidRight->SetBreakpointData(NewArc, TopArc);

	InterLeftMid->Left = LeafLeft;
	InterLeftMid->Right = InterMidRight;
	InterMidRight->Left = LeafMid;
	InterMidRight->Right = LeafRight;

	LeafLeft->Parent = InterLeftMid;
	InterMidRight->Parent = InterLeftMid;
	LeafMid->Parent = InterMidRight;
	LeafRight->Parent = InterMidRight;

	LinkPrevNext(OrigPrev, LeafLeft);
	LinkPrevNext(LeafLeft, LeafMid);
	LinkPrevNext(LeafMid, LeafRight);
	LinkPrevNext(LeafRight, OrigNext);

	// Build the graph edges
	FVorEdge* EdgeTopArc{};
	FVorEdge* EdgeNewArc{};
	Graph.CreateEdgePair(EdgeTopArc, EdgeNewArc);

	EdgeTopArc->AdjacentSite = TopArc;
	EdgeNewArc->AdjacentSite = NewArc;

	EdgeTopArc->UpdateAngle();

	InterLeftMid->BreakpointData.Edge = EdgeTopArc;
	InterMidRight->BreakpointData.Edge = EdgeNewArc;

	RBInsertFixup(InterMidRight);
	RBInsertFixup(LeafLeft);
	RBInsertFixup(LeafMid);
	RBInsertFixup(LeafRight);

	return LeafMid;
}

void FVoronoiBeachLine::Clear() {
	TArray<FNode*> NodesToDelete;
	Traverse([&](FNode* InNode) {
		NodesToDelete.Add(InNode);
	});

	for (FNode* Node : NodesToDelete) {
		delete Node;
	}

	NodesToDelete.Reset();
	RootNode = &NilNode;
}

FVoronoiBeachLine::FNode* FVoronoiBeachLine::CreateNewNode() {
	FNode* NewNode = new FNode();
	memset(NewNode, 0, sizeof(FNode));
	NewNode->Parent = &NilNode;
	NewNode->Left = &NilNode;
	NewNode->Right = &NilNode;

	NewNode->Prev = &NilNode;
	NewNode->Next = &NilNode;
	NewNode->Color = ENodeColor::Red;

	return NewNode;
}

FVoronoiBeachLine::FNode* FVoronoiBeachLine::FindArcAbove(const FVector2d& InLocation) {
	if (IsNil(RootNode)) {
		return &NilNode;
	}

	const double X = InLocation.X;
	const double Directrix = InLocation.Y;

	FNode* Node = RootNode;
	while (!IsNil(Node) && !Node->bLeafNode) {
		check(!(IsNil(Node->Left) && IsNil(Node->Right)));

		const FVorSite* SiteLeft = Node->BreakpointData.SitePointLeft;
		const FVorSite* SiteRight = Node->BreakpointData.SitePointRight;

		double IntersectionX = GetArcIntersection(SiteLeft, SiteRight, Directrix);
		Node = (X <= IntersectionX) ? Node->Left : Node->Right;
	}

	return Node;
}

double FVoronoiBeachLine::GetArcIntersection(const FVorSite* Left, const FVorSite* Right, double Directrix) {
	static constexpr double EPSILON = 1e-7;
	const bool bLeftOnSweepLine = FMath::Abs(Left->Location.Y - Directrix) < EPSILON;
	const bool bRightOnSweepLine = FMath::Abs(Right->Location.Y - Directrix) < EPSILON;

	if (bLeftOnSweepLine && bRightOnSweepLine) {
		return (Left->Location.X + Right->Location.X) * 0.5f;
	}

	if (bLeftOnSweepLine) {
		return Left->Location.X;
	}
	
	if (bRightOnSweepLine) {
		return Right->Location.X;
	}

	const double c = Directrix;
	auto CalcA = [c](const FVector2d& Pos) {
		return 0.5f / (Pos.Y - c);
	};

	auto CalcB = [c](const FVector2d& v) {
		return -v.X / (v.Y - c);
	};

	auto CalcC = [c](const FVector2d& v) {
		const double a = v.X;
		const double b = v.Y;
		return 0.5f * (a * a + b * b - c * c) / (b - c);
	};

	double A = CalcA(Left->Location) - CalcA(Right->Location);
	double B = CalcB(Left->Location) - CalcB(Right->Location);
	double C = CalcC(Left->Location) - CalcC(Right->Location);

	auto CalcIntersection = [=]() {
		if (A == 0) {
			return -C / B;
		}
		else {
			double S2 = B * B - 4 * A * C;
			if (S2 < 0) {
				S2 = B * B + 4 * A * C;
			}
			check(S2 >= 0);

			const double S = sqrt(S2);
			const double A2 = 2 * A;
			return (-B + S) / A2;
		}
	};

	return CalcIntersection();
}

void FVoronoiBeachLine::LinkPrevNext(FNode* Prev, FNode* Next) const {
	if (!IsNil(Prev)) {
		Prev->Next = Next;
	}
	if (!IsNil(Next)) {
		Next->Prev = Prev;
	}
}

FVoronoiBeachLine::FNode *FVoronoiBeachLine::SplitFirstHorizontalArc(const FVorSite *NewArc, FVoronoiGraph& Graph) {
	FNode* MaxNode = GetMax(RootNode);
	check(MaxNode->bLeafNode);
	check(IsNil(MaxNode->Next));

	const FVorSite* LeftArc = MaxNode->LeafData.SitePoint;
	const FVorSite* RightArc = NewArc;

	FNode* OrigPrev = MaxNode->Prev;
	MaxNode->Prev = &NilNode;

	MaxNode->SetBreakpointData(LeftArc, RightArc);

	FNode* LeftNode = CreateNewNode();
	LeftNode->SetLeafData(LeftArc, {});      // TODO: Handle the circle event

	FNode* RightNode = CreateNewNode();
	RightNode->SetLeafData(RightArc, {});

	// Link the Left / Right child nodes
	MaxNode->Left = LeftNode;
	MaxNode->Right = RightNode;
	LeftNode->Parent = MaxNode;
	RightNode->Parent = MaxNode;

	// Fix Prev / Next
	LinkPrevNext(OrigPrev, LeftNode);
	LinkPrevNext(LeftNode, RightNode);

	// Build the graph edges
	FVorEdge* EdgeLeft{};
	FVorEdge* EdgeRight{};
	Graph.CreateEdgePair(EdgeLeft, EdgeRight);

	MaxNode->BreakpointData.Edge = EdgeLeft;

	EdgeLeft->AdjacentSite = LeftArc;
	EdgeRight->AdjacentSite = RightArc;

	EdgeLeft->UpdateAngle();

	RBInsertFixup(LeftNode);
	RBInsertFixup(RightNode);

	return RightNode;
}

void FVorEdge::UpdateAngle() {
	check(AdjacentSite && Twin && Twin->AdjacentSite);
	const FVector2d LocSiteSrc = AdjacentSite->Location;
	const FVector2d LocSiteDst = Twin->AdjacentSite->Location;
	const FVector2d SiteDir = (LocSiteDst - LocSiteSrc).GetSafeNormal();

	const FVector2d BisectorDir = { SiteDir.Y, -SiteDir.X };

	Angle = FDAMathUtils::FindAngle(-BisectorDir);
	Direction = BisectorDir;

	constexpr double PI2 = PI * 2;

	Twin->Angle = Angle + PI;
	if (Twin->Angle > PI2) {
		Twin->Angle -= PI2;
	}
	Twin->Direction = -BisectorDir;
}

void FVoronoiGraph::Init(const TArray<FVector2d>& InSites) {
	const int32 NumSites = InSites.Num();
	Sites.SetNum(NumSites);
	for (int i = 0; i < NumSites; i++) {
		Sites[i].SiteIndex = i;
		Sites[i].Location = InSites[i];
	}

	Vertices.Init(NumSites * 3);
	Edges.Init(NumSites * 6);   // Requires at most 3n edges. Since these are half edges, we allocated 6x
}

const FVorVertex* FVoronoiGraph::CreateVertex(const FVector2d& InLocation) {
	FVorVertex* NewVertex = Vertices.AddNew();
	NewVertex->Location = InLocation;
	return NewVertex;
}

void FVoronoiGraph::CreateEdgePair(FVorEdge*& OutEdgeLeft, FVorEdge*& OutEdgeRight) {
	OutEdgeLeft = CreateEdge();
	OutEdgeRight = CreateEdge();
	OutEdgeLeft->Twin = OutEdgeRight;
	OutEdgeRight->Twin = OutEdgeLeft;
}

void FVoronoiGraph::CloneFrom(const FVoronoiGraph& Other) {
	// Setup the sites
	TArray<FVector2d> SiteLocations;
	{
		const TArray<FVorSite>& OtherSites = Other.GetSites();
		const int32 NumSites = OtherSites.Num();
		SiteLocations.SetNum(NumSites);
		
		for (int i = 0; i < NumSites; i++) {
			SiteLocations[i] = OtherSites[i].Location;
		}
	}
	
	Init(SiteLocations);
	// Setup the vertices
	{
		const TVorDataPool<FVorVertex>& OtherVertices = Other.GetVertices();
		const int NumVertices = OtherVertices.GetSize();
		for (int i = 0; i < NumVertices; i++) {
			const FVorVertex* OtherVertex = OtherVertices.Get(i);
			CreateVertex(OtherVertex ? OtherVertex->Location : FVector2d::ZeroVector);
		}
	}

	// Setup the edges
	{
		const TVorDataPool<FVorEdge>& OtherEdges = Other.GetEdges();
		for (int i = 0; i < OtherEdges.GetSize(); i++) {
			CreateEdge();
		}

		for (int i = 0; i < OtherEdges.GetSize(); i++) {
			FVorEdge* Edge = Edges.Get(i);
			const FVorEdge* OtherEdge = OtherEdges.Get(i);

			Edge->Twin = Edges.Get(OtherEdge->Twin->Index);
			Edge->Origin = Vertices.Get(OtherEdge->Origin->Index);
			Edge->AdjacentSite = &Sites[OtherEdge->AdjacentSite->SiteIndex];
			Edge->Angle = OtherEdge->Angle;
			Edge->Direction = OtherEdge->Direction;
			Edge->Index = i;
			check(Edge->Index == OtherEdge->Index);
		}
	}
}

FVorEdge* FVoronoiGraph::CreateEdge() {
	return Edges.AddNew();
}

void FVoronoiGraph::Finalize(double UnboundedEdgeExtension) {
	auto CreateVert = [this, UnboundedEdgeExtension](FVorEdge* Edge, const FVector2d& StartLocation) {
		const FVector2d VertexLocation = StartLocation + Edge->Direction * UnboundedEdgeExtension;
		const FVorVertex* Vertex = CreateVertex(VertexLocation);
		Edge->Origin = Vertex;
	};

	for (int i = 0; i < Edges.GetSize(); i++) {
		FVorEdge* Edge = Edges.Get(i);
		if (!Edge->Origin) {
			const FVector2d BisectorMid = (Edge->AdjacentSite->Location + Edge->Twin->AdjacentSite->Location) * 0.5f;
			if (!Edge->Twin->Origin) {
				CreateVert(Edge, BisectorMid);
				CreateVert(Edge->Twin, BisectorMid);
			}
			else {
				FVector2d StartLocation = Edge->Twin->Origin->Location;
				const double Dot = Edge->Direction.Dot(StartLocation - BisectorMid);
				if (Dot < 0) {
					StartLocation = BisectorMid;
				}
				CreateVert(Edge, StartLocation);
			}
		}
	}
}

FVoronoiBeachLine::~FVoronoiBeachLine() {
	Clear();
}


}

