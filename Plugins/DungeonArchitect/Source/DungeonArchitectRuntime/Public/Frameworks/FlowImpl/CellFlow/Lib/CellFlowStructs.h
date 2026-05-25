//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarkerNames.h"
#include "CellFlowStructs.generated.h"

//////////////////////// Cell Grid ////////////////////////
USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowGridEdgeInfo {
	GENERATED_BODY()

	UPROPERTY()
	FIntPoint Coord{};

	UPROPERTY()
	int TileGroup{INDEX_NONE};

	// Will be true if it's a door.   We'll check if a stair shows up here depending on the height difference
	UPROPERTY()
	int32 HeightZ{};

	UPROPERTY()
	bool bEnabled = true;
	
	// Will be true if it's a door.   We'll check if a stair shows up here depending on the height difference
	UPROPERTY()
	bool bConnection{};

	UPROPERTY()
	bool bContainsStair{};
	
	UPROPERTY()
	int32 EdgeIndex{INDEX_NONE};
	
	UPROPERTY()
	int TwinIndex{INDEX_NONE};
};


USTRUCT()
struct FCellFlowSizedMarkerDef {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Marker")
	FString MarkerName;
	
	UPROPERTY(EditAnywhere, Category="Marker")
	float Size = 400;

	UPROPERTY(EditAnywhere, Category="Marker")
	float SelectionWeight = 1.0f;

	/** Can this marker scale to fill up the remaining space along a line? */
	UPROPERTY(EditAnywhere, Category="Scaling")
	bool bAllowAutoScaling = false;

	UPROPERTY(EditAnywhere, Category="Scaling", meta=(EditCondition="bAllowAutoScaling"))
	float MinAutoScalingAllowed = 0.5f;

	/** Should we scale to fit only along the X-axis or scale evenly along XYZ */
	UPROPERTY(EditAnywhere, Category="Scaling", meta=(EditCondition="bAllowAutoScaling"))
	bool bAutoScaleUniformly = false;
	
	/** True if you want to spawn mesh that you don't want colliding with other meshes
	 * Set to true if you want to spawn buildings along the edge, and specify the expected depth taken up by the asset
	 * Set to false, if you want to spawn thin walls
	 */
	UPROPERTY(EditAnywhere, Category="Collision")
	bool bOccludes = false;

	UPROPERTY(EditAnywhere, Category="Collision", meta=(EditCondition="bOccludes"))
	float OcclusionDepth = 400.0f;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowGridMarkerSetup {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=CellFlow)
	float SelectionWeight{1};
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString GroundMarker = FCellFlowBuilderMarkers::GROUND;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString CeilingMarker = FCellFlowBuilderMarkers::CEILING;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString CeilingEdgeTrimMarker = FCellFlowBuilderMarkers::CEILING_EDGE_TRIM;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString EdgeWallMarker = FCellFlowBuilderMarkers::WALL;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString EdgeWallHalfMarker = FCellFlowBuilderMarkers::WALL_HALF;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString EdgeDoorWallHalfMarker = FCellFlowBuilderMarkers::DOOR_WALL_HALF;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString EdgeWallSeparatorMarker = FCellFlowBuilderMarkers::WALL_SEPARATOR;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString DoorMarker = FCellFlowBuilderMarkers::DOOR;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString DoorOneWayMarker = FCellFlowBuilderMarkers::DOOR_ONEWAY;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString StairMarker = FCellFlowBuilderMarkers::STAIR;
};


USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowGridMarkerSetupList {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	TArray<FCellFlowGridMarkerSetup> MarkerSetupList;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowVoronoiEdgeMarkerLayer {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=CellFlow)
	bool bBoundaryEdges = true;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	bool bSharedEdgesHigher = true;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	bool bSharedEdgesLower = true;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	bool bSharedEdgesSameHeight = true;

	UPROPERTY(EditAnywhere, Category=CellFlow, meta=(EditCondition="bSharedEdgesSameHeight"))
	bool bEnableTwoSidedSameHeightEdge = false;

	UPROPERTY(EditAnywhere, Category=CellFlow)
	TArray<FCellFlowSizedMarkerDef> EdgeMarkers = { { FCellFlowBuilderMarkers::WALL } };
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowVoronoiMarkerSetup {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=CellFlow)
	float SelectionWeight{1};
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	TArray<FCellFlowVoronoiEdgeMarkerLayer> EdgeLayers = { {} };

	UPROPERTY(EditAnywhere, Category=CellFlow)
	FString CornerMarker = FCellFlowBuilderMarkers::WALL_SEPARATOR;
	
	UPROPERTY(EditAnywhere, Category=CellFlow)
	TArray<FCellFlowSizedMarkerDef> DoorMarker = { { FCellFlowBuilderMarkers::DOOR, 400 } };

	UPROPERTY(EditAnywhere, Category=CellFlow)
	TArray<FCellFlowSizedMarkerDef> DoorOneWayMarker = { { FCellFlowBuilderMarkers::DOOR_ONEWAY, 400 } };

	UPROPERTY(EditAnywhere, Category=CellFlow)
	TArray<FCellFlowSizedMarkerDef> StairMarker = { { FCellFlowBuilderMarkers::STAIR, 400 } };
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowVoronoiMarkerSetupList {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	TArray<FCellFlowVoronoiMarkerSetup> MarkerSetupList;
};

namespace CellFlowConstants {
	const FName CustomParameter_InitialGridFilterName( TEXT("InitialGridFilter") );
	constexpr int CELL_EMPTY = -1;
	constexpr int CELL_INVALID = -2;
}