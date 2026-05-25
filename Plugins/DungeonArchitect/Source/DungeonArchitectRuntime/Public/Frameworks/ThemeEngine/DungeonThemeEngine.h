//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/MarkerGenerator/MarkerGenProcessor.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

class FDungeonSceneProvider;
struct FDungeonMarkerBuildData;
struct FDungeonMarkerInstance;
class ADungeonThemeOverrideVolume;
class UDungeonSelectorLogic;
class UDungeonTransformLogic;

struct FDungeonThemeEngineSettings {
    TArray<UDungeonThemeAsset*> Themes;
    TArray<FClusterThemeInfo> ClusteredThemes;
    TArray<ADungeonThemeOverrideVolume*> ThemeOverrideVolumes;
    TSharedPtr<FDungeonSceneProvider> SceneProvider;
	TSharedPtr<IMarkerGenProcessor> MarkerGenerator;
    TArray<UDungeonMarkerEmitter*> MarkerEmitters;
	TWeakObjectPtr<ADungeon> Dungeon;
	FGuid ChunkId;
	TWeakObjectPtr<ULevel> LevelOverride;
	bool bRoleAuthority = true;
};

struct FDungeonThemeEngineEventHandlers {
    TFunction<bool(const TArray<UDungeonSelectorLogic*>&, const FDungeonMarkerInstance&)> PerformSelectionLogic
            = [](const TArray<UDungeonSelectorLogic*>&, const FDungeonMarkerInstance&){ return false; };

	TFunction<bool(const TArray<UGenericDungeonSelectorLogic*>&, const FDungeonMarkerInstance&)> PerformGenericSelectionLogic
			= [](const TArray<UGenericDungeonSelectorLogic*>&, const FDungeonMarkerInstance&){ return false; };

	TFunction<FTransform(const TArray<UDungeonTransformLogic*>&, const FDungeonMarkerInstance&)> PerformTransformLogic
			= [](const TArray<UDungeonTransformLogic*>&, const FDungeonMarkerInstance&) { return FTransform::Identity; };

	TFunction<FTransform(const TArray<UProceduralDungeonTransformLogic*>&, const FDungeonMarkerInstance&)> PerformProceduralTransformLogic
		= [](const TArray<UProceduralDungeonTransformLogic*>&, const FDungeonMarkerInstance&) { return FTransform::Identity; };

    TFunction<void(TArray<FDungeonMarkerBuildData>&)> HandlePostMarkersEmit
            = [](TArray<FDungeonMarkerBuildData>&) {};

	TFunction<void(FDungeonSceneProvider&)> HandleEmitCustomSceneObjects
			= [](FDungeonSceneProvider&) {};
	
};

class DUNGEONARCHITECTRUNTIME_API FDungeonThemeEngine {
public:
    static void Apply(TArray<FDungeonMarkerInstance>& Markers, const FRandomStream& InRandom,
                const FDungeonThemeEngineSettings& InSettings, const FDungeonThemeEngineEventHandlers& EventHandlers);

};

