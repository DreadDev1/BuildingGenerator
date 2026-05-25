//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NonLatentCurveTimeline.generated.h"

class UCurveFloat;

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FNonLatentCurveTimeline {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	TObjectPtr<UCurveFloat> Curve = nullptr;;

	UPROPERTY()
	float Time = 0;
};

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UNonLatentCurveTimelineBlueprintFunctionLib : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void TickNonLatentTimelineForward(float DeltaSeconds, UPARAM(ref) FNonLatentCurveTimeline& Timeline);
	
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void TickNonLatentTimelineBackward(float DeltaSeconds, UPARAM(ref) FNonLatentCurveTimeline& Timeline);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon Architect")
	static float GetNonLatentTimelineValue(const FNonLatentCurveTimeline& Timeline);

private:
	static void TickImpl(FNonLatentCurveTimeline& Timeline, float DeltaSeconds);
	
};

