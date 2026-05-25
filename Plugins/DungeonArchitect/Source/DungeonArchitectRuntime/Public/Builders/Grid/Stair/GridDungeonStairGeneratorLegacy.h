//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/Grid/Stair/GridDungeonStairGenerator.h"
#include "GridDungeonStairGeneratorLegacy.generated.h"

UCLASS(DisplayName="Stair Generator Legacy")
class DUNGEONARCHITECTRUNTIME_API UGridDungeonBuilderStairGeneratorLegacy : public UGridDungeonBuilderStairGeneratorBase {
	GENERATED_BODY()
	
public:
	virtual void Generate(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder) const override;
	
private:
	virtual void ConnectStairs(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder, const FVector& GridToMeshScale, int32 WeightThreshold) const;
};

