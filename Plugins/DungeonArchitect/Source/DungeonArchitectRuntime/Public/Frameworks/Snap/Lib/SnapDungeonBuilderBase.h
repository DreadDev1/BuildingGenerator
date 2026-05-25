//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"
#include "SnapDungeonBuilderBase.generated.h"

UCLASS(Abstract)
class USnapDungeonBuilderBase : public UDungeonBuilder {
	GENERATED_BODY()
public:
	virtual void DestroyDungeonImpl(UWorld* World) override;
};

