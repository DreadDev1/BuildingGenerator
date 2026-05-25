//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/DungeonPCGCache.h"


void UDungeonPCGCache::Clear() {
	SDFPointData = nullptr;
	SDFPointDataLoadState = EDungeonPCGDataLoadState::NotLoaded;
	ComponentCaches.Reset();
}

FDungeonPCGComponentCache* UDungeonPCGCache::GetComponentCache(const UPCGComponent* InComponent) {
	for (FDungeonPCGComponentCache& Cache : ComponentCaches) {
		if (Cache.Owner == InComponent) {
			return &Cache;
		}
	}
	return nullptr;
}

void UDungeonPCGCache::CreateComponentCache(UPCGComponent* InComponent) {
	FDungeonPCGComponentCache& ComponentCache = ComponentCaches.AddDefaulted_GetRef();
	ComponentCache.Owner = InComponent;
}

