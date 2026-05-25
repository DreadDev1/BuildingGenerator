//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GridDungeonStairGenerator.generated.h"

class UGridDungeonModel;
class UGridDungeonBuilder;
namespace GridDungeonBuilderImpl {
	struct FCellHeightNode;
}

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UGridDungeonBuilderStairGeneratorBase : public UObject {
	GENERATED_BODY()
public:
	/**
	  The number of logical floor units the dungeon height can vary. This determines how high the dungeon's height
	  can vary (e.g. max 2 floors high).   Set this value depending on the stair meshes you designer has created. 
	  For e.g.,  if there are two stair meshes, one 200 units high (1 floor) and another 400 units high (2 floors), this value should be set to 2
	  If you do not want any stairs / height variations, set this value to 0
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	int32 MaxAllowedStairHeight = 1;

	/**
	  weak this value to increase / reduce the height variations (and stairs) in your dungeon.
	  A value close to 0 reduces the height variation and increases as you approach 1. 
	  Increasing this value to a higher level might create dungeons with no place for
	  proper stair placement since there is too much height variation.   
	  A value of 0.2 to 0.4 seems good
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	float HeightVariationProbability = 0.2f;
	
	/**
	  The generator would add stairs to make different areas of the dungeon accessible.
	  However, we do not want too many stairs. For e.g., before adding a stair in a 
	  particular elevated area, the generator would check if this area is already 
	  accessible from a nearby stair. If so, it would not add it.   
	  This tolerance parameter determines how far to look for an existing path
	  before we can add a stair.   Play with this parameter if you see too
	  many stairs close to each other, or too few
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	float StairConnectionTolerance = 6;
	
public:
	virtual void Generate(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder) const {}

protected:
	virtual void GenerateDungeonHeights(UGridDungeonModel* GridModel, UGridDungeonBuilder* GridBuilder, bool bFixHeights) const;

	/** Iteratively fixes the dungeon adjacent cell heights if they are too high up. Returns true if more iterative fixes are required */
	virtual bool FixDungeonCellHeights(UGridDungeonModel* GridModel, TMap<int32, GridDungeonBuilderImpl::FCellHeightNode>& CellHeightNodes,
		const TSet<TPair<int32, int32>>& ClampedAdjacentNodes = {}) const;
};

