//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarker.h"
#include "Frameworks/PCG/DungeonPCGCommon.h"

#include "PCGComponent.h"
#include "DungeonPCGCache.generated.h"

class UPCGDungeonSGFTextureData;

/** Cache for each PCG component in the dungeon actor */
USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonPCGComponentCache {
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UPCGComponent> Owner;
	
	UPROPERTY()
	TArray<FDungeonMarkerInstance> InputWorldMarkers;
};


/** Global cache used by all the PCG components in the dungeon actor */
UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonPCGCache : public UObject {
	GENERATED_BODY()
public:
	void CreateComponentCache(UPCGComponent* InComponent);
	FDungeonPCGComponentCache* GetComponentCache(const UPCGComponent* InComponent);
	
	void Clear();
	
	
	
public:
	UPROPERTY()
	TObjectPtr<UPCGDungeonSGFTextureData> SDFPointData;

	UPROPERTY()
	EDungeonPCGDataLoadState SDFPointDataLoadState = EDungeonPCGDataLoadState::NotLoaded;
	
	
private:
	UPROPERTY()
	TArray<FDungeonPCGComponentCache> ComponentCaches;
};

