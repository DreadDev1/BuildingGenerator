//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Markers/DungeonMarker.h"

#include "Core/Markers/DungeonMarkerTags.h"

void FDungeonMarkerInstanceTags::Append(const FDungeonMarkerInstanceTags& Other) {
	for (const FName OtherTag : Other.Tags) {
		Tags.AddUnique(OtherTag);
	}

	int32 Elevation{};
	bool bContainsElevation = FDungeonMarkerInstanceTagManagement::GetElevationFromGround(*this, Elevation);

	for (auto& Entry : Other.IntValues) {
		IntValues.FindOrAdd(Entry.Key) = Entry.Value;
	}
	for (auto& Entry : Other.FloatValues) {
		FloatValues.FindOrAdd(Entry.Key) = Entry.Value;
	}
	for (auto& Entry : Other.VectorValues) {
		VectorValues.FindOrAdd(Entry.Key) = Entry.Value;
	}

	if (bContainsElevation) {
		int32 NewElevation{};
		if (FDungeonMarkerInstanceTagManagement::GetElevationFromGround(*this, NewElevation)) {
			Elevation = FMath::Min(Elevation, NewElevation);
			FDungeonMarkerInstanceTagManagement::SetElevationFromGround(*this, Elevation);
		}
	}
}

