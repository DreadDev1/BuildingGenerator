//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/Grid/Stair/GridDungeonStairGenerator.h"
#include "GridDungeonStairGeneratorV2.generated.h"

UCLASS(DisplayName="Stair Generator V2")
class DUNGEONARCHITECTRUNTIME_API UGridDungeonBuilderStairGeneratorV2 : public UGridDungeonBuilderStairGeneratorBase {
	GENERATED_BODY()
public:
	/** Having stairs inside rooms would mis-align the room walls. Check this flag if you don't want it */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	bool bAvoidStairsInsideRooms = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	bool bAvoidSingleCellStairDeadEnds = true;
	
public:
	virtual void Generate(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder) const override;
	
private:
	virtual void ConnectStairs(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder, const FVector& GridToMeshScale) const;
};


