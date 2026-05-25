//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class AActor;

class FDungeonSceneProviderLib {
public:
	static bool GetNodeId(AActor* InActor, const FName& InDungeonTag, FGuid& OutNodeId);
    
	static const FGuid StaticInstancedMeshNodeId;
};

