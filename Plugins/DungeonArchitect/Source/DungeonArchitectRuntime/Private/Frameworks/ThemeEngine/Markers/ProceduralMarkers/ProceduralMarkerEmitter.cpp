//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerEmitter.h"

#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerLib.h"

void UBoxBoundaryMarkerEmitter::EmitMarkers(const FVector& GridSize, const FIntVector& CoordMin, const FIntVector& CoordMax,
                                            const FTransform& DungeonTransform, const TFunction<void(const FString&, const FTransform&)> FnEmitMarker) const {
	const int HeightZ = (HeightFunction == EProcMarkerGenSimpleHeightFunction::LowestPoint)
			 ? CoordMin.Z
			 : CoordMax.Z;

	if (bEmitEdges) {
		TArray<FTransform> Transforms;
		DA::FProceduralMarkerLib::GetBoxBoundaryEdgeTransforms(GridSize, CoordMin, CoordMax, HeightZ, SizeMultiplier, Transforms);
		for (const FTransform& EdgeTransform : Transforms) {
			FnEmitMarker(EdgeMarkerName, EdgeTransform * DungeonTransform);
		}
	}

	if (bEmitEdgeSeparators) {
		TArray<FTransform> Transforms;
		DA::FProceduralMarkerLib::GetBoxBoundaryEdgeSeparatorTransforms(GridSize, CoordMin, CoordMax, HeightZ, SizeMultiplier, Transforms);
		for (const FTransform& EdgeTransform : Transforms) {
			FnEmitMarker(EdgeSeparatorMarkerName, EdgeTransform * DungeonTransform);
		}
	}

	if (bEmitCorners) {
		TArray<FTransform> Transforms;
		DA::FProceduralMarkerLib::GetBoxBoundaryCornerTransforms(GridSize, CoordMin, CoordMax, HeightZ, SizeMultiplier, Transforms);
		for (const FTransform& EdgeTransform : Transforms) {
			FnEmitMarker(CornerMarkerName, EdgeTransform * DungeonTransform);
		}
	}

	if (bEmitGroundTiles) {
		TArray<FTransform> Transforms;
		DA::FProceduralMarkerLib::GetBoxBoundaryGroundTransforms(GridSize, CoordMin, CoordMax, HeightZ, SizeMultiplier, Transforms);
		for (const FTransform& EdgeTransform : Transforms) {
			FnEmitMarker(GroundMarkerName, EdgeTransform * DungeonTransform);
		}
	}
}

/*
bool UBoundaryMarkerEmitter::Execute(IMarkerGenProcessor& InProcessor, const FRandomStream& InRandom, TArray<FDAMarkerInfo>& InOutMarkerList) {
	static const FName AssetPathEdge = TEXT("/DungeonArchitect/Core/ProceduralMarkerEmitters/Patterns/_Internal_Theme_BoundaryEdge");
	static const FName AssetPathCorner = TEXT("/DungeonArchitect/Core/ProceduralMarkerEmitters/Patterns/_Internal_Theme_BoundaryCorner");
	static const FName AssetPathTile = TEXT("/DungeonArchitect/Core/ProceduralMarkerEmitters/Patterns/_Internal_Theme_BoundaryTile");
	const UDungeonThemeAsset* PatternThemeAsset = Cast<UDungeonThemeAsset>(StaticLoadObject(UDungeonThemeAsset::StaticClass(), nullptr, TEXT("/DungeonArchitect/Core/ProceduralMarkerEmitters/Patterns/_Internal_Theme_BoundaryEdge")));

	
	TArray<FDAMarkerInfo> NewMarkers;
	if (!InProcessor.Process(MarkerGenLayer, InOutMarkerList, InRandom, NewMarkers)) {
		return false;
	}
	InOutMarkerList = NewMarkers;
	return true;
}
*/

