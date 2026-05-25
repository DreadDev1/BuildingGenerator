//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeatmapColorRamp.generated.h"

class UCurveLinearColor;

UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UHeatmapColorRamp : public UDataAsset
{
	GENERATED_BODY()

public:
	UHeatmapColorRamp();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heatmap")
	TObjectPtr<UCurveLinearColor> ColorRamp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heatmap")
	float WrapDistance;

	UFUNCTION(BlueprintCallable, Category = "Heatmap")
	FLinearColor GetColorForDistance(float Distance) const;
};

