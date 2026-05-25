//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DungeonArchitectProjectUserSettings.generated.h"

UCLASS(config=EditorPerProjectUserSettings, Meta=(DisplayName="Dungeon Architect"))
class DUNGEONARCHITECTRUNTIME_API UDungeonArchitectProjectUserSettings : public UDeveloperSettings {
	GENERATED_BODY()
public:
	UDungeonArchitectProjectUserSettings();
	
#if WITH_EDITORONLY_DATA

	UPROPERTY(config, EditAnywhere, Category=DungeonArchitect)
	float ThemeEdModeActorHighlightSaturation = 0.85f;

	UPROPERTY(config, EditAnywhere, Category=DungeonArchitect)
	bool bThemeEdModeAutoLayoutGraph = false;
	
#endif // WITH_EDITORONLY_DATA

public:
#if WITH_EDITOR 
	void SetThemeEdModeActorHighlightSaturation(float InValue);
	float GetThemeEdModeActorHighlightSaturation() const;
	
	void SetThemeEdModeAutoLayoutGraph(bool bInValue);
	bool GetThemeEdModeAutoLayoutGraph() const;
#endif // WITH_EDITOR
	
};

