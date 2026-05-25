//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/Connection/SnapConnectionInfo.h"


USnapConnectionInfo::USnapConnectionInfo(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer), ConnectionCategory("default") {

    ThemeAsset = ObjectInitializer.CreateDefaultSubobject<UDungeonThemeAsset>(this, "ThemeAsset");
	//Version = static_cast<int32>(ESnapConnectionInfoVersion::LatestVersion);
}

