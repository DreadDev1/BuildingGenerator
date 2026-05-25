//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Lib/Voronoi/Voronoi.h"

#include "Core/Utils/MathUtils.h"

namespace DA {
	
void FVoronoiGenerator::Generate(FVoronoiGraph& InGraph, const TArray<FVector2d>& InSites, const FSettings& InSettings) {
	FVoronoiBeachLine BeachLine;
	FVoronoiEventQueue EventQueue;
	InGraph.Init(InSites);
	for (const FVorSite& Site: InGraph.GetSites()) {
		EventQueue.HeapPush(FVoronoiEvent::CreateSiteEvent(&Site));
	}
	
	FVoronoiGenerator Generator(BeachLine, EventQueue, InGraph);
	Generator.ExecuteEvents();
	
	InGraph.Finalize(InSettings.UnboundedEdgeExtension);
}

FVoronoiGenerator::FVoronoiGenerator(FVoronoiBeachLine& InBeachLine, FVoronoiEventQueue& InEventQueue, FVoronoiGraph& InGraph)
		: BeachLine(InBeachLine)
		, EventQueue(InEventQueue)
		, Graph(InGraph) {
}

void FVoronoiGenerator::ExecuteEvents() {
	while (EventQueue.Num() > 0) {
		FVoronoiEventPtr Event;
		EventQueue.HeapPop(Event);

		if (!Event->bSiteEvent && !Event->CircleEvent.bValid) {
			continue;
		}

		if (Event->bSiteEvent) {
			HandleSiteEvent(Event);
		}
		else {
			HandleCircleEvent(Event);
		}
	}
	
}

void FVoronoiGenerator::HandleSiteEvent(const FVoronoiEventPtr& Event) {
	const FVector2d Location = Event->GetLocation();

	FVoronoiBeachLine::FNode* NewArc;

	if (bFirstEvent || Location.Y != FirstSweepY)
	{
		FVoronoiBeachLine::FNode* ArcAbove = BeachLine.FindArcAbove(Location);
		if (!BeachLine.IsNil(ArcAbove) && ArcAbove->LeafData.CircleEvent) {
			// Tag as a false alarm
			ArcAbove->LeafData.CircleEvent->bValid = false;
		}

		NewArc = BeachLine.SplitArc(ArcAbove, Event->SiteEvent.Site, Graph);
	}
	else {
		NewArc = BeachLine.SplitFirstHorizontalArc(Event->SiteEvent.Site, Graph);
	}

	if (bFirstEvent) {
		bFirstEvent = false;
		FirstSweepY = Location.Y;
	}

	// Check for circle event
	CreateCircleEvent(NewArc, NewArc->Next, NewArc->Next->Next);
	CreateCircleEvent(NewArc->Prev->Prev, NewArc->Prev, NewArc);
}

void FVoronoiGenerator::HandleCircleEvent(const FVoronoiEventPtr& Event) {
	if (!Event->CircleEvent.bValid) {
		// This has been tagged as a false alarm and should be ignored
		return;
	}

	// Remove the arc from the beach line structure
	FVoronoiBeachLine::FNode* ArcToRemove = Event->CircleEvent.DisappearingArc;
	FVoronoiBeachLine::FNode* PrevArc = ArcToRemove->Prev;
	FVoronoiBeachLine::FNode* NextArc = ArcToRemove->Next;

	if (!BeachLine.IsNil(ArcToRemove->Prev) && ArcToRemove->Prev->LeafData.CircleEvent) {
		ArcToRemove->Prev->LeafData.CircleEvent->bValid = false;
	}

	if (!BeachLine.IsNil(ArcToRemove->Next) && ArcToRemove->Next->LeafData.CircleEvent) {
		ArcToRemove->Next->LeafData.CircleEvent->bValid = false;
	}
	BeachLine.RemoveArc(ArcToRemove, Graph);
	Event->CircleEvent.DisappearingArc = {};

	CreateCircleEvent(PrevArc->Prev, PrevArc, NextArc);
	CreateCircleEvent(PrevArc, NextArc, NextArc->Next);
}

void FVoronoiGenerator::CreateCircleEvent(FVoronoiBeachLine::FNode* Left, FVoronoiBeachLine::FNode* Mid, FVoronoiBeachLine::FNode* Right) {
	if (BeachLine.IsNil(Left) || BeachLine.IsNil(Mid) || BeachLine.IsNil(Right)) {
		return;
	}

	check(Left->bLeafNode && Mid->bLeafNode && Right->bLeafNode);

	const FVector2d& A = Left->LeafData.SitePoint->Location;
	const FVector2d& B = Mid->LeafData.SitePoint->Location;
	const FVector2d& C = Right->LeafData.SitePoint->Location;

	auto BisectorsConverge = [](const FVector2d& A, const FVector2d& B, const FVector2d& C) {
		const FVector3d D = FVector3d(A - B, 0).Cross(FVector3d(B - C, 0));
		return D.Z < 0;
	};

	if (BisectorsConverge(A, B, C)) {
		FVector2d CircumCenter{};
		double Radius{};
		if (FDAMathUtils::CalcCircumCenter(A, B, C, CircumCenter, Radius)) {
			const FVector2d EventLocation = CircumCenter - FVector2d(0, Radius);
			const FVoronoiEventPtr EventPtr = FVoronoiEvent::CreateCircleEvent(EventLocation, CircumCenter, Mid);
			EventQueue.HeapPush(EventPtr);
		}
	}
}

FVector2d FVoronoiGenerator::GetBisectorDir(const FVector2d &T, const FVector2d &U) {
	const FVector2d Dir = (U - T).GetSafeNormal();
	return FVector2d{Dir.Y, -Dir.X};
}

}

