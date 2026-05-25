//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FPCGContext;
class UDungeonCanvasComponent;
struct FDungeonMarkerInstance;
class UPCGPointData;

class DUNGEONARCHITECTRUNTIME_API FDungeonPCGLib {
public:
	static void WriteMarkersToPCGPointData(const TArray<FDungeonMarkerInstance>& InMarkers, UPCGPointData* InPointData);
	static void ReadMarkersFromPCGPointData(const UPCGPointData* PointData, TArray<FDungeonMarkerInstance>& OutMarkers);
	static UDungeonCanvasComponent* GetCanvasComponent(const FPCGContext* InContext);
	
};

