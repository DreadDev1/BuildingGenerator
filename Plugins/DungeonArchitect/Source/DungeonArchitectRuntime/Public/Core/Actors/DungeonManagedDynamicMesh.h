//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DynamicMeshActor.h"
#include "DungeonManagedDynamicMesh.generated.h"

UCLASS()
class ADungeonManagedDynamicMesh : public ADynamicMeshActor {
	GENERATED_BODY()
public:
	UPROPERTY()
	FGuid DungeonID{};
};

