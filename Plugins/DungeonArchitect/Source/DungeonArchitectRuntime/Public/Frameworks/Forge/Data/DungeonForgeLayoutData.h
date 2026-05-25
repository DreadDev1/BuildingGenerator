//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "DungeonForgeLayoutData.generated.h"

UCLASS()
class UDungeonForgeLayoutData : public UDungeonForgeData {
	GENERATED_BODY()
public:
	UPROPERTY()
	FDungeonLayoutData Layout;

public:
#if WITH_EDITOR
	virtual void WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const override;
#endif // WITH_EDITOR
};

