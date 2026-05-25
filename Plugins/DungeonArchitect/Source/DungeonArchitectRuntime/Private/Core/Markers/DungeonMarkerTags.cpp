//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Markers/DungeonMarkerTags.h"


#define DA_MARKER_MGMT_PROPERTY_IMPL(PropertyName, TagName) \
const FName FDungeonMarkerInstanceTagManagement::TAG_NAME_##PropertyName = TagName;	


DA_MARKER_MGMT_PROPERTY_IMPL(ElevationFromGround, TEXT("Room.Elevation.Current"))
DA_MARKER_MGMT_PROPERTY_IMPL(CeilingElevation, TEXT("Room.Elevation.Ceiling"))

int32 UDungeonMarkerInstanceTagLib::GetDungeonMarkerIntTag(const FDungeonMarkerInstance& Instance, EDungeonMarkerIntTag Tag, int32 DefaultValue) {
	if (const int* ValuePtr = Instance.Tags.IntValues.Find(ToTagName(Tag))) {
		return *ValuePtr;
	}
	return DefaultValue;
}

