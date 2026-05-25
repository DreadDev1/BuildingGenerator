//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonModel.h"
#include "Frameworks/Snap/Lib/Serialization/SnapConnectionSerialization.h"
#include "Frameworks/Snap/Lib/Serialization/SnapModuleInstanceSerialization.h"
#include "SnapDungeonModelBase.generated.h"

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API USnapDungeonModelBase : public UDungeonModel {
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FSnapConnectionInstance> Connections;

	UPROPERTY()
	TArray<FSnapWallInstance> Walls;
    
	UPROPERTY()
	TArray<FSnapModuleInstanceSerializedData> ModuleInstances;

public:
	virtual void GenerateLayoutDataImpl(const UDungeonConfig* InConfig, struct FDungeonLayoutData& OutLayout) const override;

protected:
	void GetFloorIndexRange(float InFloorHeight, float InGlobalHeightOffset, int32& OutIndexMin, int32& OutIndexMax) const;
};

