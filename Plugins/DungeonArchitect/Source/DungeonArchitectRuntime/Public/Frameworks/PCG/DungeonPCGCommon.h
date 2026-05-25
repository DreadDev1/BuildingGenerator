//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonPCGCommon.generated.h"

UENUM()
enum class EDungeonPCGDataLoadState : uint8 {
	NotLoaded,
	Loading,
	Loaded
};

