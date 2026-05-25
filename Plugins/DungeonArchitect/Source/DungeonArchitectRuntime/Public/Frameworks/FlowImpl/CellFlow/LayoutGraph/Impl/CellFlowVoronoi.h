//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "CellFlowVoronoi.generated.h"

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDAFlowCellLeafNodeVoronoi : public UDAFlowCellLeafNode {
	GENERATED_BODY()
public:
	UPROPERTY()
	int32 SiteIndex{};

	UPROPERTY()
	double CenterX{};
	
	UPROPERTY()
	double CenterY{};

	UPROPERTY()
	float Area{};
	
	virtual FVector2d GetCenter() const override { return FVector2d{CenterX, CenterY}; }
	virtual float GetArea() const override { return Area; }
};

