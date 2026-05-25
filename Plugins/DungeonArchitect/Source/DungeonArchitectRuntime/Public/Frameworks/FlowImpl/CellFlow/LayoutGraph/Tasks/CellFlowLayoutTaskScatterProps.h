//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTask.h"
#include "CellFlowLayoutTaskScatterProps.generated.h"

USTRUCT()
struct FCellFlowLayoutTaskScatterPropAssemblySettings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Dungeon")
	bool bEnabled{};
	
	UPROPERTY(EditAnywhere, Category = "Dungeon", meta=(EditCondition="bEnabled"))
	FString MarkerName{};
	
	UPROPERTY(EditAnywhere, Category = "Dungeon", meta=(EditCondition="bEnabled"))
	float Probability{};
};

USTRUCT()
struct FCellFlowLayoutTaskScatterPropSettings {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Marker Names")
	TArray<FString> EdgeMarkerNames { { "Wall", "Fence" } };

	UPROPERTY(EditAnywhere, Category = "Marker Names")
	TArray<FString> GroundMarkerNames { { "Ground" } };
	
	UPROPERTY(EditAnywhere, Category = "Marker Names")
	TArray<FString> DoorMarkerNames { { "Door" } };

	UPROPERTY(EditAnywhere, Category = "Marker Names")
	TArray<FString> StairMarkerNames { { "Stair" } };

	UPROPERTY(EditAnywhere, Category = "Marker Names")
	TArray<FString> TilesMarkersToAvoid { { "Enemy", "Key" } };

	UPROPERTY(EditAnywhere, Category = "Props")
	FCellFlowLayoutTaskScatterPropAssemblySettings Prop1x1{true, "Prop1x1", 1.0f};

	UPROPERTY(EditAnywhere, Category = "Props")
	FCellFlowLayoutTaskScatterPropAssemblySettings Prop1x2{false, "Prop1x2", 0.5f};

	UPROPERTY(EditAnywhere, Category = "Props")
	FCellFlowLayoutTaskScatterPropAssemblySettings Prop1x3{false, "Prop1x3", 0.2f};
};

UCLASS(Meta = (AbstractTask, Title = "Scatter Props (Grid)", Tooltip = "Scatter Props in free space around the scene", MenuPriority = 1500))
class DUNGEONARCHITECTRUNTIME_API UCellFlowLayoutTaskScatterProps : public UBaseFlowLayoutTask {
	GENERATED_BODY()
public:
	virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const override;

public:
	UPROPERTY(EditAnywhere, Category = "Scatter")
	FCellFlowLayoutTaskScatterPropSettings Settings;
	
};

