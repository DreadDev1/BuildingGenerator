//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UDungeonThemeAsset;

/**
 * Upgrade an older asset's compiled data at runtime. To avoid upgrading this at runtime,
 * open up the theme in the theme editor and recompile to the latest version
 */
class DUNGEONARCHITECTRUNTIME_API FDungeonThemeCompiledGraphUpgrader {
public:
	static void Upgrade(UDungeonThemeAsset* InThemeAsset);
	
private:
	static void UpgradeToVersion2_MovedToCompiledData(UDungeonThemeAsset* InThemeAsset);
};
