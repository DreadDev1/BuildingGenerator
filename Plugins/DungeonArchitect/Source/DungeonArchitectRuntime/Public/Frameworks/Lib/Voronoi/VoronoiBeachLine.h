//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

namespace DA {
	class FVoronoiBeachLine;
	struct FVoronoiEvent;
	struct FVoronoiCircleEvent;
	typedef TSharedPtr<FVoronoiEvent> FVoronoiEventPtr;
	typedef TArray<FVoronoiEventPtr> FVoronoiEventQueue;

	struct FVorSite {
		FVector2d Location{};
		int SiteIndex{};
	};

	struct FVorVertex {
		FVector2d Location{};
		int Index{};
	};

	struct FVorEdge {
		FVorEdge* Twin{};
		const FVorVertex* Origin{};
		const FVorSite* AdjacentSite{};
		double Angle{};
		FVector2d Direction{};
		int Index{};

		void UpdateAngle();
	};

	template<typename T>
	class TVorDataPool {
	public:
		~TVorDataPool() {
			Clear();
		}

		void Init(int InCapacity) {
			Clear();
			Capacity = InCapacity;
			Array = new T[Capacity];
			for (int i = 0; i < Capacity; i++) {
				Array[i].Index = i;
			}
		}

		inline T* AddNew() {
			check(Size + 1 <= Capacity);
			Size++;
			return &Array[Size - 1];
		}

		inline T* Get(size_t Index) const {
			check(Index < Size);
			return &Array[Index];
		}

		void Clear() {
			delete[] Array;
			Array = {};
			Capacity = {};
			Size = {};
		}

		inline size_t GetSize() const { return Size; }

		T* GetArray() const { return Array; }

	private:
		T* Array{};
		size_t Capacity{};
		size_t Size{};
	};

	class FVoronoiGraph {
	public:
		void Init(const TArray<FVector2d>& InSites);
		const FVorVertex* CreateVertex(const FVector2d& InLocation);
		void CreateEdgePair(FVorEdge*& OutEdgeLeft, FVorEdge*& OutEdgeRight);
		void Finalize(double UnboundedEdgeExtension);

		const TVorDataPool<FVorVertex>& GetVertices() const { return Vertices; }
		const TVorDataPool<FVorEdge>& GetEdges() const { return Edges; }
		const TArray<FVorSite>& GetSites() const { return Sites; }
		void CloneFrom(const FVoronoiGraph& Other);
	
		FORCEINLINE int GetNumSites() const { return Sites.Num(); }

	private:
		FVorEdge* CreateEdge();
	
	
	private:
		TArray<FVorSite> Sites;
		TVorDataPool<FVorVertex> Vertices;
		TVorDataPool<FVorEdge> Edges;
	};


	class FVoronoiBeachLine {
	public:
		~FVoronoiBeachLine();

		enum class ENodeColor : unsigned char {
			Black,
			Red
		};

		struct FLeafData {
			const FVorSite* SitePoint{};
			FVoronoiCircleEvent* CircleEvent{};
		};

		struct FBreakpointData {
			const FVorSite* SitePointLeft{};
			const FVorSite* SitePointRight{};
			FVorEdge* Edge{};
		};

		struct FNode {
			FNode* Parent{};
			FNode* Left{};
			FNode* Right{};
			ENodeColor Color{};

			FNode* Prev{};
			FNode* Next{};

			bool bLeafNode = true;
			union {
				FLeafData LeafData;
				FBreakpointData BreakpointData;
			};

			explicit FNode(ENodeColor InColor = ENodeColor::Black)
					: Color(InColor)
			{
			}
			bool IsRed() const { return Color == ENodeColor::Red; }
			void SetLeafData(const FVorSite* InSitePoint, FVoronoiCircleEvent* InCircleEvent) {
				bLeafNode = true;
				LeafData.SitePoint = InSitePoint;
				LeafData.CircleEvent = InCircleEvent;
			}
			void SetBreakpointData(const FVorSite* InSitePointLeft, const FVorSite* InSitePointRight) {
				bLeafNode = false;
				BreakpointData.SitePointLeft = InSitePointLeft;
				BreakpointData.SitePointRight = InSitePointRight;
			}
		};

		void Clear();
		FORCEINLINE FNode* GetRoot() const { return RootNode; }
		FORCEINLINE bool IsNil(const FNode* InNode) const {
			return InNode == &NilNode || !InNode;
		}

		FNode* CreateNewNode();
		FNode* FindArcAbove(const FVector2d& InLocation);

		static double GetArcIntersection(const FVorSite* Left, const FVorSite* Right, double Directrix);
		void LinkPrevNext(FNode* Prev, FNode* Next) const;;
		FNode* SplitFirstHorizontalArc(const FVorSite* NewArc, FVoronoiGraph &Graph);
		FNode* SplitArc(FNode* x, const FVorSite* NewArc, FVoronoiGraph& Graph);
		void RemoveArc(FNode* NodeToRemove, FVoronoiGraph& Graph);
		void Remove(FNode* Node);
		FNode* GetMin(FNode* InNode) const;
		FNode* GetMax(FNode* InNode) const;
		FNode* GetSuccessor(FNode* Node) const;
		FNode* GetPredecessor(FNode* Node) const;

		struct FTraverseStat {
			int Depth {};
			FVector2d ParentPreviewPos {};
			FVector2d PreviewPos {};
		};

		typedef TFunction<void(FNode*)> FnTraverseVisit;
		typedef TFunction<void(FNode*, const FTraverseStat& InStat)> FnTraverseStatVisit;
		void Traverse(const FnTraverseVisit& InVisitFn) const;
		void Traverse(const FnTraverseStatVisit& InVisitFn) const;

	protected:
		void LeftRotate(FNode* x);
		void RightRotate(FNode* x);
		void RBInsertFixup(FNode* z);
		void Transplant(FNode* u, FNode* v);

		// Removes the node from the tree. NOTE: This doesn't delete the memory. The caller is responsible for memory management
		void DeleteImpl(FNode* z);
		void RBDeleteFixup(FNode* x);
		void TraverseSimple(FNode* InNode, const FnTraverseVisit& InVisitFn) const;
		void TraverseWithStat(FNode* InNode, const FTraverseStat& InStat, const FnTraverseStatVisit& InVisitFn) const;

	protected:
		FNode NilNode = FNode(ENodeColor::Black);
		FNode* RootNode = &NilNode;
	};

	struct FVoronoiBeachLineExtension {
		FString GetNodeString(FVoronoiBeachLine::FNode* InNode) const;
		void GetCustomLinks(const FVoronoiBeachLine& InBeachLine, TArray<TPair<typename FVoronoiBeachLine::FNode*, typename FVoronoiBeachLine::FNode*>>& CustomList) const;
	};

	struct FVoronoiSiteEvent {
		const FVorSite* Site;
	};

	struct FVoronoiCircleEvent {
		FVector2d Location;
		FVector2d CircumCenter;
		FVoronoiBeachLine::FNode* DisappearingArc; // Store pointer to the disappearing arc

		int _PreviewDisappearingArcSiteIndex{};

		bool bValid{ true };
	};


	struct FVoronoiEvent {
		FVoronoiEvent() {}
		bool bSiteEvent{};
		//union {
		FVoronoiSiteEvent SiteEvent;
		FVoronoiCircleEvent CircleEvent;
		//};
		static FVoronoiEventPtr CreateSiteEvent(const FVorSite* InSite);
		static FVoronoiEventPtr CreateCircleEvent(const FVector2d &InLocation, const FVector2d &InCircumCenter, FVoronoiBeachLine::FNode* InDisappearingArc);
		FVector2d GetLocation() const;
	};
	bool operator<(const FVoronoiEvent& A, const FVoronoiEvent& B);
	bool operator<(const FVoronoiEventPtr& A, const FVoronoiEventPtr& B);
}

