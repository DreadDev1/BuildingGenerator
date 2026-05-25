//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Settings/DungeonArchitectProjectUserSettings.h"


UDungeonArchitectProjectUserSettings::UDungeonArchitectProjectUserSettings()
{
	
}

#if WITH_EDITOR 
void UDungeonArchitectProjectUserSettings::SetThemeEdModeActorHighlightSaturation(float InValue) {
	if (ThemeEdModeActorHighlightSaturation != InValue) {
		ThemeEdModeActorHighlightSaturation = InValue;
		SaveConfig();
	}
}

float UDungeonArchitectProjectUserSettings::GetThemeEdModeActorHighlightSaturation() const {
	return ThemeEdModeActorHighlightSaturation;
}

void UDungeonArchitectProjectUserSettings::SetThemeEdModeAutoLayoutGraph(bool bInValue) {
	if (bInValue != bThemeEdModeAutoLayoutGraph) {
		bThemeEdModeAutoLayoutGraph = bInValue;
		SaveConfig();
	}
}

bool UDungeonArchitectProjectUserSettings::GetThemeEdModeAutoLayoutGraph() const {
	return bThemeEdModeAutoLayoutGraph;
}
#endif  // WITH_EDITOR