//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Lib/Voronoi/VoronoiBeachLine.h"

namespace DA {
	class FVoronoiGenerator {
	public:
		struct FSettings {
			float UnboundedEdgeExtension = 100000;
		};
		static void Generate(FVoronoiGraph& InGraph, const TArray<FVector2d>& InSites, const FSettings& InSettings);
	
	private:
		FVoronoiGenerator(FVoronoiBeachLine& InBeachLine, FVoronoiEventQueue& InEventQueue, FVoronoiGraph& InGraph);
		void ExecuteEvents();
	
		void HandleSiteEvent(const FVoronoiEventPtr& Event);
		void HandleCircleEvent(const FVoronoiEventPtr& Event);
		void CreateCircleEvent(FVoronoiBeachLine::FNode* Left, FVoronoiBeachLine::FNode* Mid, FVoronoiBeachLine::FNode* Right);
		static FVector2d GetBisectorDir(const FVector2d& T, const FVector2d& U);

	private:
		FVoronoiBeachLine& BeachLine;
		FVoronoiEventQueue& EventQueue;
		FVoronoiGraph& Graph;
		bool bFirstEvent = true;
		float FirstSweepY{};
	};

	typedef TSharedPtr<FVoronoiGenerator> FVoronoiGeneratorPtr;

}

