//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"
#include "GridFlowItem.generated.h"

UENUM()
enum class EGridFlowTilemapItemPlacementMode {
	Random,
	NearEdges,
	Center
};

USTRUCT()
struct FGridFlowTilemapItemPlacementSettings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Item Placement")
	EGridFlowTilemapItemPlacementMode PlacementMode = EGridFlowTilemapItemPlacementMode::Random;

	UPROPERTY(EditAnywhere, Category="Item Placement")
	bool bAvoidPlacingNearDoors = true;
};


UCLASS()
class UGridFlowGraphItem : public UFlowGraphItem {
	GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, Category = "Dungeon")
	FGridFlowTilemapItemPlacementSettings PlacementSettings;
};

